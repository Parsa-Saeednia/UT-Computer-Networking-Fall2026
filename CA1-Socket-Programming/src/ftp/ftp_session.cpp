#include "ftp_session.hpp"
#include "mode_b.hpp"
#include "../common/config.hpp"
#include "../common/utils.hpp"

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <cstring>
#include <cctype>
#include <poll.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

using namespace std;

FtpSession::FtpSession(int ctrl_fd, string root_dir)
    : ctrl_fd_(ctrl_fd), root_dir_(move(root_dir)) {}

FtpSession::~FtpSession() {
    close_data();
}

FtpSession::FtpSession(FtpSession&& other) noexcept
    : ctrl_fd_(other.ctrl_fd_),
      root_dir_(move(other.root_dir_)),
      user_name_(move(other.user_name_)),
      pass_expected_(other.pass_expected_),
      logged_in_(other.logged_in_),
      mode_b_enabled_(other.mode_b_enabled_),
      is_closed_(other.is_closed_),
      pasv_listen_fd_(other.pasv_listen_fd_),
      data_fd_(other.data_fd_),
      pasv_active_(other.pasv_active_) {
    other.ctrl_fd_ = -1;
    other.pasv_listen_fd_ = -1;
    other.data_fd_ = -1;
}

FtpSession& FtpSession::operator=(FtpSession&& other) noexcept {
    if (this != &other) {
        close_data();
        ctrl_fd_ = other.ctrl_fd_;
        root_dir_ = move(other.root_dir_);
        user_name_ = move(other.user_name_);
        pass_expected_ = other.pass_expected_;
        logged_in_ = other.logged_in_;
        mode_b_enabled_ = other.mode_b_enabled_;
        is_closed_ = other.is_closed_;
        pasv_listen_fd_ = other.pasv_listen_fd_;
        data_fd_ = other.data_fd_;
        pasv_active_ = other.pasv_active_;
        other.ctrl_fd_ = -1;
        other.pasv_listen_fd_ = -1;
        other.data_fd_ = -1;
    }
    return *this;
}

void FtpSession::close_data() {
    if (data_fd_ >= 0) {
        ::close(data_fd_);
        data_fd_ = -1;
    }
    if (pasv_listen_fd_ >= 0) {
        ::close(pasv_listen_fd_);
        pasv_listen_fd_ = -1;
    }
    pasv_active_ = false;
}

void FtpSession::send_reply(const string& reply) {
    if (ctrl_fd_ >= 0) Utils::send_all(ctrl_fd_, reply.data(), reply.size());
}

bool FtpSession::ensure_data_connected() {
    if (data_fd_ >= 0) return true;
    if (pasv_listen_fd_ < 0) return false;

    pollfd pfd{pasv_listen_fd_, POLLIN, 0};
    int ret = ::poll(&pfd, 1, 5000);
    if (ret > 0 && (pfd.revents & POLLIN)) {
        sockaddr_in client_addr{};
        socklen_t len = sizeof(client_addr);
        int accepted = ::accept(pasv_listen_fd_, reinterpret_cast<sockaddr*>(&client_addr), &len);
        if (accepted >= 0) {
            data_fd_ = accepted;
            ::close(pasv_listen_fd_);
            pasv_listen_fd_ = -1;
            return true;
        }
    }
    return false;
}

string FtpSession::format_pasv_response(uint16_t port) const {
    sockaddr_in ctrl_addr{};
    socklen_t ctrl_len = sizeof(ctrl_addr);
    ::getsockname(ctrl_fd_, reinterpret_cast<sockaddr*>(&ctrl_addr), &ctrl_len);
    uint32_t ip = ntohl(ctrl_addr.sin_addr.s_addr);
    int h1 = (ip >> 24) & 0xFF, h2 = (ip >> 16) & 0xFF, h3 = (ip >> 8) & 0xFF, h4 = ip & 0xFF;
    if (h1 == 0 && h2 == 0 && h3 == 0 && h4 == 0) {
        h1 = 127; h2 = 0; h3 = 0; h4 = 1;
    }
    return "227 Entering Passive Mode (" + to_string(h1) + "," + to_string(h2) + "," +
           to_string(h3) + "," + to_string(h4) + "," +
           to_string((port >> 8) & 0xFF) + "," + to_string(port & 0xFF) + ")\r\n";
}

void FtpSession::handle_user(const string& args) {
    if (args.empty()) {
        send_reply("501 SYNTAX_ERROR_PARAMS\r\n");
        return;
    }
    user_name_ = args;
    pass_expected_ = true;
    logged_in_ = false;
    send_reply("331 Password required\r\n");
}

void FtpSession::handle_pass(const string&) {
    if (!pass_expected_) {
        send_reply("503 BAD_SEQUENCE\r\n");
        return;
    }
    pass_expected_ = false;
    logged_in_ = true;
    send_reply("230 Logged in\r\n");
}

void FtpSession::handle_mode(const string& args) {
    if (!logged_in_) {
        send_reply("530 NOT_LOGGED_IN\r\n");
        return;
    }
    if (args.empty()) {
        send_reply("501 SYNTAX_ERROR_PARAMS\r\n");
        return;
    }
    if (args == "B" || args == "b") {
        mode_b_enabled_ = true;
        send_reply("200 OK\r\n");
    } else {
        send_reply("504 COMMAND_NOT_IMPLEMENTED\r\n");
    }
}

void FtpSession::handle_pasv() {
    if (!logged_in_) {
        send_reply("530 NOT_LOGGED_IN\r\n");
        return;
    }
    close_data();
    pasv_listen_fd_ = ::socket(AF_INET, SOCK_STREAM, 0);
    if (pasv_listen_fd_ < 0) {
        send_reply("425 Cannot open data connection\r\n");
        return;
    }
    int opt = 1;
    ::setsockopt(pasv_listen_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    sockaddr_in addr{AF_INET, 0, {INADDR_ANY}, {}};
    if (::bind(pasv_listen_fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0 ||
        ::listen(pasv_listen_fd_, 1) < 0) {
        close_data();
        send_reply("425 Cannot open data connection\r\n");
        return;
    }
    socklen_t len = sizeof(addr);
    ::getsockname(pasv_listen_fd_, reinterpret_cast<sockaddr*>(&addr), &len);
    pasv_active_ = true;
    send_reply(format_pasv_response(ntohs(addr.sin_port)));
}

string FtpSession::build_directory_listing() const {
    DIR* dir = ::opendir(root_dir_.c_str());
    string listing;
    if (dir) {
        struct dirent* entry;
        while ((entry = ::readdir(dir)) != nullptr) {
            string fname = entry->d_name;
            if (fname == "." || fname == ".." || fname.rfind(".upload_tmp_", 0) == 0) continue;
            string full_path = root_dir_ + "/" + fname;
            struct stat st;
            if (::stat(full_path.c_str(), &st) == 0 && S_ISREG(st.st_mode)) {
                listing += fname + " " + to_string(st.st_size) + "\n";
            }
        }
        ::closedir(dir);
    }
    return listing;
}

void FtpSession::handle_list(const string& args) {
    if (!logged_in_) {
        send_reply("530 NOT_LOGGED_IN\r\n");
        return;
    }
    if (!args.empty()) {
        send_reply("501 SYNTAX_ERROR_PARAMS\r\n");
        return;
    }
    if (!mode_b_enabled_) {
        send_reply("503 BAD_SEQUENCE\r\n");
        return;
    }
    if (!pasv_active_ || !ensure_data_connected()) {
        send_reply("425 CANT_OPEN_DATA_CONNECTION\r\n");
        close_data();
        return;
    }
    send_reply("150 Opening data connection\r\n");
    string listing = build_directory_listing();
    ModeB::send_data_stream(data_fd_, listing.data(), listing.size());
    close_data();
    send_reply("226 Transfer complete\r\n");
}

bool FtpSession::validate_transfer_preconditions(const string& filename) {
    if (!logged_in_) { send_reply("530 NOT_LOGGED_IN\r\n"); return false; }
    if (filename.empty()) { send_reply("501 SYNTAX_ERROR_PARAMS\r\n"); return false; }
    if (!Utils::is_valid_ftp_filename(filename)) { send_reply("553 NAME_NOT_ALLOWED\r\n"); return false; }
    if (!mode_b_enabled_) { send_reply("503 BAD_SEQUENCE\r\n"); return false; }
    if (!pasv_active_) { send_reply("425 CANT_OPEN_DATA_CONNECTION\r\n"); return false; }
    return true;
}

void FtpSession::handle_retr(const string& args) {
    if (!validate_transfer_preconditions(args)) return;

    string file_path = root_dir_ + "/" + args;
    struct stat st;
    if (::stat(file_path.c_str(), &st) != 0 || !S_ISREG(st.st_mode)) {
        send_reply("550 FILE_NOT_FOUND\r\n");
        close_data();
        return;
    }
    if (!ensure_data_connected()) {
        send_reply("425 CANT_OPEN_DATA_CONNECTION\r\n");
        close_data();
        return;
    }
    send_reply("150 Opening data connection\r\n");
    ifstream ifs(file_path, ios::binary);
    vector<char> file_data((istreambuf_iterator<char>(ifs)), istreambuf_iterator<char>());
    ifs.close();
    ModeB::send_data_stream(data_fd_, file_data.data(), file_data.size());
    close_data();
    send_reply("226 Transfer complete\r\n");
}

bool FtpSession::receive_stor_file(int data_fd, const string& tmp_name) {
    ofstream ofs(tmp_name, ios::binary | ios::trunc);
    if (!ofs) return false;

    uint8_t desc = 0;
    vector<char> payload;
    bool success = false;
    while (ModeB::read_block(data_fd, desc, payload)) {
        if (desc != ModeB::DESCRIPTOR_DATA && desc != ModeB::DESCRIPTOR_EOF) break;
        if (!payload.empty()) ofs.write(payload.data(), payload.size());
        if (desc == ModeB::DESCRIPTOR_EOF) {
            success = true;
            break;
        }
    }
    ofs.close();
    return success;
}

void FtpSession::execute_stor_finalize(const string& tmp_name, const string& final_name, bool success) {
    if (success) {
        string final_dest = root_dir_ + "/" + final_name;
        ::rename(tmp_name.c_str(), final_dest.c_str());
        send_reply("226 Transfer complete\r\n");
    } else {
        ::unlink(tmp_name.c_str());
        send_reply("426 CONNECTION_CLOSED\r\n");
    }
}

void FtpSession::handle_stor(const string& args) {
    if (!validate_transfer_preconditions(args)) return;
    if (!ensure_data_connected()) {
        send_reply("425 CANT_OPEN_DATA_CONNECTION\r\n");
        close_data();
        return;
    }
    send_reply("150 Opening data connection\r\n");
    string tmp_name = root_dir_ + "/.upload_tmp_" + to_string(::getpid()) + "_" + args;
    bool success = receive_stor_file(data_fd_, tmp_name);
    close_data();
    execute_stor_finalize(tmp_name, args, success);
}

void FtpSession::handle_quit() {
    send_reply("221 Goodbye\r\n");
    is_closed_ = true;
}

void FtpSession::process_command(const string& line) {
    auto [cmd, args] = Utils::split_first(line, ' ');
    string upper_cmd = cmd;
    for (char& c : upper_cmd) c = static_cast<char>(toupper(static_cast<unsigned char>(c)));

    if (upper_cmd == "USER") handle_user(args);
    else if (upper_cmd == "PASS") handle_pass(args);
    else if (upper_cmd == "MODE") handle_mode(args);
    else if (upper_cmd == "PASV") handle_pasv();
    else if (upper_cmd == "LIST") handle_list(args);
    else if (upper_cmd == "RETR") handle_retr(args);
    else if (upper_cmd == "STOR") handle_stor(args);
    else if (upper_cmd == "QUIT") handle_quit();
    else send_reply("500 COMMAND_UNKNOWN\r\n");
}
