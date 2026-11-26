#include "chat_client.hpp"
#include "../common/utils.hpp"
#include "../common/exception.hpp"

#include <iostream>
#include <cstring>
#include <poll.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

using namespace std;

ChatClient::ChatClient(string host, int port)
    : host_(move(host)), port_(port) {}

ChatClient::~ChatClient() {
    if (sockfd_ >= 0) {
        ::close(sockfd_);
        sockfd_ = -1;
    }
}

void ChatClient::connect_to_server() {
    sockfd_ = ::socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd_ < 0) throw SocketCreationException("Failed to create TCP client socket");

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(static_cast<uint16_t>(port_));
    if (::inet_pton(AF_INET, host_.c_str(), &server_addr.sin_addr) <= 0) {
        throw ConnectionException("Invalid server address: " + host_);
    }

    if (::connect(sockfd_, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr)) < 0) {
        throw ConnectionException("Failed to connect to chat server at " + host_ + ":" + to_string(port_));
    }
}

void ChatClient::process_received_lines(string& recv_buf) {
    while (true) {
        size_t nl_pos = recv_buf.find('\n');
        if (nl_pos == string::npos) break;

        string line = recv_buf.substr(0, nl_pos);
        recv_buf.erase(0, nl_pos + 1);

        if (!line.empty() && line.back() == '\r') line.pop_back();
        cout << line << "\n" << flush;
    }
}

void ChatClient::handle_server_incoming(string& recv_buf) {
    char buf[2048];
    ssize_t n = ::recv(sockfd_, buf, sizeof(buf), 0);
    if (n <= 0) throw NetworkException("Connection closed by server");

    recv_buf.append(buf, n);
    process_received_lines(recv_buf);
}

void ChatClient::handle_user_input() {
    string user_input;
    if (!getline(cin, user_input)) {
        string quit_cmd = "QUIT\n";
        Utils::send_all(sockfd_, quit_cmd.data(), quit_cmd.size());
        throw NetworkException("Standard input closed");
    }
    user_input += "\n";
    Utils::send_all(sockfd_, user_input.data(), user_input.size());
}

void ChatClient::run() {
    connect_to_server();
    pollfd pfds[2]{{STDIN_FILENO, POLLIN, 0}, {sockfd_, POLLIN, 0}};
    string recv_buf;

    try {
        while (true) {
            int ret = ::poll(pfds, 2, -1);
            if (ret < 0) {
                if (errno == EINTR) continue;
                break;
            }
            if (pfds[1].revents & (POLLIN | POLLERR | POLLHUP | POLLNVAL)) {
                handle_server_incoming(recv_buf);
            }
            if (pfds[0].revents & POLLIN) {
                handle_user_input();
            }
        }
    } catch (const NetworkException& e) {}
}
