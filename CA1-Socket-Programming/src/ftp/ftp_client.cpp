#include "ftp_client.hpp"
#include "mode_b.hpp"
#include "../common/config.hpp"
#include "../common/utils.hpp"
#include "../common/exception.hpp"

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <cstring>
#include <cctype>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

using namespace std;

FtpClient::FtpClient(string host, int port)
    : host_(move(host)), port_(port) {}

FtpClient::~FtpClient() {
    if (pending_data_fd_ >= 0) ::close(pending_data_fd_);
    if (ctrl_fd_ >= 0) ::close(ctrl_fd_);
}

void FtpClient::connect_control() {
    ctrl_fd_ = ::socket(AF_INET, SOCK_STREAM, 0);
    if (ctrl_fd_ < 0) throw SocketCreationException("Failed to create FTP control socket");

    sockaddr_in server_addr{AF_INET, htons(static_cast<uint16_t>(port_)), {}, {}};
    if (::inet_pton(AF_INET, host_.c_str(), &server_addr.sin_addr) <= 0) {
        throw ConnectionException("Invalid host address: " + host_);
    }
    if (::connect(ctrl_fd_, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr)) < 0) {
        throw ConnectionException("Failed to connect to FTP server at " + host_ + ":" + to_string(port_));
    }
}

string FtpClient::read_control_line() {
    while (true) {
        size_t nl_pos = ctrl_recv_buf_.find('\n');
        if (nl_pos != string::npos) {
            string line = ctrl_recv_buf_.substr(0, nl_pos);
            ctrl_recv_buf_.erase(0, nl_pos + 1);
            if (!line.empty() && line.back() == '\r') line.pop_back();
            return line;
        }

        char buf[1024];
        ssize_t n = ::recv(ctrl_fd_, buf, sizeof(buf), 0);
        if (n <= 0) return "";
        ctrl_recv_buf_.append(buf, n);
    }
}

void FtpClient::send_control_line(const string& line) {
    string out = line + "\r\n";
    Utils::send_all(ctrl_fd_, out.data(), out.size());
}

bool FtpClient::request_pasv(string& out_ip, int& out_port) {
    send_control_line("PASV");
    string reply = read_control_line();
    cout << reply << "\n";
    if (reply.rfind("227", 0) != 0) return false;

    size_t lp = reply.find('('), rp = reply.find(')', lp);
    if (lp == string::npos || rp == string::npos) return false;

    stringstream ss(reply.substr(lp + 1, rp - (lp + 1)));
    int h1, h2, h3, h4, p1, p2;
    char c;
    if (!(ss >> h1 >> c >> h2 >> c >> h3 >> c >> h4 >> c >> p1 >> c >> p2)) return false;

    out_ip = to_string(h1) + "." + to_string(h2) + "." + to_string(h3) + "." + to_string(h4);
    out_port = p1 * 256 + p2;
    return true;
}

int FtpClient::connect_data(const string& ip, int port) {
    int dfd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (dfd < 0) return -1;

    sockaddr_in addr{AF_INET, htons(static_cast<uint16_t>(port)), {}, {}};
    ::inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);
    if (::connect(dfd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        ::close(dfd);
        return -1;
    }
    return dfd;
}

int FtpClient::ensure_data_connection() {
    if (pending_data_fd_ >= 0) {
        int dfd = pending_data_fd_;
        pending_data_fd_ = -1;
        return dfd;
    }
    string ip;
    int port = 0;
    if (!request_pasv(ip, port)) return -1;
    return connect_data(ip, port);
}

void FtpClient::read_and_print_mode_b_data(int dfd) {
    uint8_t desc = 0;
    vector<char> payload;
    while (ModeB::read_block(dfd, desc, payload)) {
        if (!payload.empty()) cout.write(payload.data(), payload.size());
        if (desc == ModeB::DESCRIPTOR_EOF) break;
    }
}

void FtpClient::handle_list() {
    int dfd = ensure_data_connection();
    if (dfd < 0) {
        cerr << "Cannot establish data connection for LIST\n";
        return;
    }
    send_control_line("LIST");
    string resp = read_control_line();
    cout << resp << "\n";
    if (resp.rfind("150", 0) == 0) {
        read_and_print_mode_b_data(dfd);
        cout << read_control_line() << "\n";
    }
    ::close(dfd);
}

void FtpClient::read_and_write_mode_b_file(int dfd, const string& filename) {
    ofstream ofs(filename, ios::binary | ios::trunc);
    uint8_t desc = 0;
    vector<char> payload;
    while (ModeB::read_block(dfd, desc, payload)) {
        if (!payload.empty() && ofs) ofs.write(payload.data(), payload.size());
        if (desc == ModeB::DESCRIPTOR_EOF) break;
    }
    ofs.close();
}

void FtpClient::handle_retr(const string& filename) {
    if (filename.empty()) { cerr << "Local error: RETR requires a filename\n"; return; }
    int dfd = ensure_data_connection();
    if (dfd < 0) { cerr << "Cannot establish data connection for RETR\n"; return; }

    send_control_line("RETR " + filename);
    string resp = read_control_line();
    cout << resp << "\n";
    if (resp.rfind("150", 0) == 0) {
        read_and_write_mode_b_file(dfd, filename);
        cout << read_control_line() << "\n";
    }
    ::close(dfd);
}

void FtpClient::handle_stor(const string& filename) {
    if (filename.empty()) { cerr << "Local error: STOR requires a filename\n"; return; }
    ifstream ifs(filename, ios::binary);
    if (!ifs) { cerr << "Local error: cannot open local file " << filename << "\n"; return; }
    vector<char> data((istreambuf_iterator<char>(ifs)), istreambuf_iterator<char>());
    ifs.close();

    int dfd = ensure_data_connection();
    if (dfd < 0) { cerr << "Cannot establish data connection for STOR\n"; return; }
    send_control_line("STOR " + filename);
    string resp = read_control_line();
    cout << resp << "\n";
    if (resp.rfind("150", 0) == 0) {
        ModeB::send_data_stream(dfd, data.data(), data.size());
        cout << read_control_line() << "\n";
    }
    ::close(dfd);
}

bool FtpClient::process_user_command(const string& trimmed) {
    auto [cmd, args] = Utils::split_first(trimmed, ' ');
    string upper_cmd = cmd;
    for (char& c : upper_cmd) c = static_cast<char>(toupper(static_cast<unsigned char>(c)));

    if (upper_cmd == "QUIT") {
        send_control_line("QUIT");
        cout << read_control_line() << "\n";
        return false;
    }
    if (upper_cmd == "PASV") {
        string ip; int port = 0;
        if (request_pasv(ip, port)) {
            if (pending_data_fd_ >= 0) ::close(pending_data_fd_);
            pending_data_fd_ = connect_data(ip, port);
        }
    } else if (upper_cmd == "LIST") handle_list();
    else if (upper_cmd == "RETR") handle_retr(args);
    else if (upper_cmd == "STOR") handle_stor(args);
    else {
        send_control_line(trimmed);
        cout << read_control_line() << "\n";
    }
    return true;
}

void FtpClient::run() {
    connect_control();
    cout << read_control_line() << "\n";
    string input;
    while (getline(cin, input)) {
        string trimmed = Utils::trim(input);
        if (trimmed.empty()) continue;
        if (!process_user_command(trimmed)) break;
    }
}
