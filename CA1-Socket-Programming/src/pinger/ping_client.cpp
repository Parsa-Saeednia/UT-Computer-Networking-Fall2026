#include "ping_client.hpp"
#include "../common/config.hpp"
#include "../common/utils.hpp"
#include "../common/exception.hpp"

#include <iostream>
#include <iomanip>
#include <thread>
#include <algorithm>
#include <numeric>
#include <sstream>
#include <cstring>
#include <poll.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

using namespace std;

PingClient::PingClient(string host, int port, int count, int timeout_ms, int interval_ms)
    : host_(move(host)), port_(port), count_(count),
      timeout_ms_(timeout_ms), interval_ms_(interval_ms) {}

PingClient::~PingClient() {
    if (sockfd_ >= 0) {
        ::close(sockfd_);
        sockfd_ = -1;
    }
}

bool PingClient::setup_socket(sockaddr_in& server_addr) {
    sockfd_ = ::socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd_ < 0) {
        throw SocketCreationException("Failed to create UDP client socket");
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(static_cast<uint16_t>(port_));
    if (::inet_pton(AF_INET, host_.c_str(), &server_addr.sin_addr) <= 0) {
        throw ConnectionException("Invalid server IP address: " + host_);
    }
    return true;
}

bool PingClient::send_single_ping(int seq, const string& token, const sockaddr_in& server_addr) {
    string ping_msg = "PING " + to_string(seq) + " " + token + "\n";
    ssize_t sent = ::sendto(sockfd_, ping_msg.data(), ping_msg.size(), 0,
                            reinterpret_cast<const sockaddr*>(&server_addr), sizeof(server_addr));
    return sent >= 0;
}

bool PingClient::check_pong_packet(int seq, const char* buf, ssize_t len,
                                   chrono::steady_clock::time_point send_time, double& out_rtt) {
    auto recv_time = chrono::steady_clock::now();
    string raw(buf, len);
    if (!raw.empty() && raw.back() == '\n') raw.pop_back();
    if (!raw.empty() && raw.back() == '\r') raw.pop_back();

    stringstream ss(raw);
    string p_cmd, p_seq, p_token;
    if (ss >> p_cmd >> p_seq >> p_token && p_cmd == "PONG") {
        try {
            if (stoi(p_seq) == seq) {
                chrono::duration<double, milli> diff = recv_time - send_time;
                out_rtt = diff.count();
                return true;
            }
        } catch (...) {}
    }
    return false;
}

bool PingClient::wait_for_pong(int seq, chrono::steady_clock::time_point send_time, double& out_rtt) {
    while (true) {
        auto now = chrono::steady_clock::now();
        auto elapsed = chrono::duration_cast<chrono::milliseconds>(now - send_time).count();
        if (elapsed >= timeout_ms_) break;

        pollfd pfd{sockfd_, POLLIN, 0};
        int ret = ::poll(&pfd, 1, static_cast<int>(timeout_ms_ - elapsed));
        if (ret <= 0) break;

        if (pfd.revents & POLLIN) {
            char buf[1024];
            sockaddr_in from{};
            socklen_t from_len = sizeof(from);
            ssize_t n = ::recvfrom(sockfd_, buf, sizeof(buf), 0,
                                   reinterpret_cast<sockaddr*>(&from), &from_len);
            if (n > 0 && check_pong_packet(seq, buf, n, send_time, out_rtt)) {
                return true;
            }
        }
    }
    return false;
}

void PingClient::ping_cycle(int seq, const sockaddr_in& server_addr, int& sent_count,
                            int& received_count, vector<double>& rtt_list) {
    string token = "token_" + to_string(seq);
    auto send_time = chrono::steady_clock::now();
    if (send_single_ping(seq, token, server_addr)) sent_count++;

    double measured_rtt = 0.0;
    if (wait_for_pong(seq, send_time, measured_rtt)) {
        received_count++;
        rtt_list.push_back(measured_rtt);
        cout << fixed << setprecision(3) << "PONG seq=" << seq << " rtt=" << measured_rtt << " ms\n";
    } else {
        cout << "TIMEOUT seq=" << seq << "\n";
    }

    if (seq < count_ && interval_ms_ > 0) {
        this_thread::sleep_for(chrono::milliseconds(interval_ms_));
    }
}

void PingClient::print_summary(int sent, int received, const vector<double>& rtt_list) const {
    double loss_percent = (sent > 0) ? (((sent - received) * 100.0) / sent) : 0.0;
    cout << "--- ping statistics ---\n";
    cout << "sent=" << sent << " received=" << received
         << " loss=" << fixed << setprecision(1) << loss_percent << "%\n";

    if (!rtt_list.empty()) {
        double min_rtt = *min_element(rtt_list.begin(), rtt_list.end());
        double max_rtt = *max_element(rtt_list.begin(), rtt_list.end());
        double avg_rtt = accumulate(rtt_list.begin(), rtt_list.end(), 0.0) / rtt_list.size();
        cout << fixed << setprecision(3) << "rtt min/avg/max = "
             << min_rtt << "/" << avg_rtt << "/" << max_rtt << " ms\n";
    } else {
        cout << "rtt min/avg/max = -/-/- ms\n";
    }
}

void PingClient::run() {
    sockaddr_in server_addr{};
    setup_socket(server_addr);

    int sent_count = 0;
    int received_count = 0;
    vector<double> rtt_list;

    for (int seq = 1; seq <= count_; ++seq) {
        ping_cycle(seq, server_addr, sent_count, received_count, rtt_list);
    }

    print_summary(sent_count, received_count, rtt_list);
}
