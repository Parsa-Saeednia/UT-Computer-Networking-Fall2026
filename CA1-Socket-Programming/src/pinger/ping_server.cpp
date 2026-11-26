#include "ping_server.hpp"
#include "../common/config.hpp"
#include "../common/utils.hpp"
#include "../common/exception.hpp"

#include <iostream>
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

using namespace std;

PingServer::PingServer(int port, double loss_rate, unsigned int seed, bool has_seed)
    : port_(port), loss_rate_(loss_rate), seed_(seed), has_seed_(has_seed) {
    if (loss_rate_ < 0.0) loss_rate_ = 0.0;
    if (loss_rate_ > 1.0) loss_rate_ = 1.0;

    if (has_seed_) {
        srand(seed_);
    } else {
        srand(static_cast<unsigned int>(time(nullptr)));
    }
}

PingServer::~PingServer() {
    stop();
}

bool PingServer::setup_socket() {
    sockfd_ = ::socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd_ < 0) {
        throw SocketCreationException("Failed to create UDP socket");
    }

    int opt = 1;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(static_cast<uint16_t>(port_));

    if (::bind(sockfd_, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr)) < 0) {
        ::close(sockfd_);
        sockfd_ = -1;
        throw BindException("Failed to bind UDP socket to port " + to_string(port_));
    }
    return true;
}

bool PingServer::should_drop_packet() const {
    double r = static_cast<double>(rand()) / (static_cast<double>(RAND_MAX) + 1.0);
    return r < loss_rate_;
}

void PingServer::send_reply(const string& message, const sockaddr_in& client_addr, socklen_t addr_len) {
    ::sendto(sockfd_, message.data(), message.size(), 0,
             reinterpret_cast<const sockaddr*>(&client_addr), addr_len);
}

bool PingServer::validate_ascii(const string& raw) const {
    for (char c : raw) {
        if (static_cast<unsigned char>(c) > 127) return false;
    }
    return true;
}

bool PingServer::parse_ping_fields(const string& raw, string& seq, string& token) const {
    size_t first_space = raw.find(' ');
    if (first_space == string::npos || first_space == 0) return false;

    string cmd = raw.substr(0, first_space);
    if (cmd != "PING") return false;

    size_t second_space = raw.find(' ', first_space + 1);
    if (second_space == string::npos) return false;

    seq = raw.substr(first_space + 1, second_space - (first_space + 1));
    token = raw.substr(second_space + 1);

    if (token.find(' ') != string::npos) return false;
    return Utils::is_valid_ping_seq(seq) && Utils::is_valid_ping_token(token);
}

void PingServer::handle_valid_ping(const string& seq, const string& token,
                                   const sockaddr_in& client_addr, socklen_t addr_len) {
    if (should_drop_packet()) return;
    string pong_msg = "PONG " + seq + " " + token + "\n";
    send_reply(pong_msg, client_addr, addr_len);
}

void PingServer::process_incoming_packet() {
    char buffer[1024];
    sockaddr_in client_addr{};
    socklen_t addr_len = sizeof(client_addr);

    ssize_t n = ::recvfrom(sockfd_, buffer, sizeof(buffer), 0,
                           reinterpret_cast<sockaddr*>(&client_addr), &addr_len);
    if (n < 0 || n > Config::Pinger::MAX_DATAGRAM_SIZE) {
        if (n > Config::Pinger::MAX_DATAGRAM_SIZE) send_reply("ERR BAD_REQUEST\n", client_addr, addr_len);
        return;
    }

    string raw(buffer, n);
    if (!raw.empty() && raw.back() == '\n') raw.pop_back();
    if (!raw.empty() && raw.back() == '\r') raw.pop_back();

    string seq, token;
    if (!validate_ascii(raw) || !parse_ping_fields(raw, seq, token)) {
        send_reply("ERR BAD_REQUEST\n", client_addr, addr_len);
        return;
    }

    handle_valid_ping(seq, token, client_addr, addr_len);
}

void PingServer::start() {
    setup_socket();
    is_running_ = true;
    cout << "[ping-server] Listening on UDP port " << port_ << " with loss = " << loss_rate_ << "...\n";
    while (is_running_) {
        process_incoming_packet();
    }
}

void PingServer::stop() {
    is_running_ = false;
    if (sockfd_ >= 0) {
        ::close(sockfd_);
        sockfd_ = -1;
    }
}
