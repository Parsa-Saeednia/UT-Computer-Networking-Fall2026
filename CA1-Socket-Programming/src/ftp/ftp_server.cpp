#include "ftp_server.hpp"
#include "../common/config.hpp"
#include "../common/utils.hpp"
#include "../common/exception.hpp"

#include <iostream>
#include <vector>
#include <cstring>
#include <poll.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>

using namespace std;

FtpServer::FtpServer(int port, string root_dir)
    : port_(port), root_dir_(move(root_dir)) {}

FtpServer::~FtpServer() {
    stop();
}

void FtpServer::setup_listen_socket() {
    ::signal(SIGPIPE, SIG_IGN);
    listen_fd_ = ::socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd_ < 0) throw SocketCreationException("Failed to create FTP listen socket");

    int opt = 1;
    ::setsockopt(listen_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{AF_INET, htons(static_cast<uint16_t>(port_)), {INADDR_ANY}, {}};
    if (::bind(listen_fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        ::close(listen_fd_);
        listen_fd_ = -1;
        throw BindException("Failed to bind FTP socket to port " + to_string(port_));
    }
    if (::listen(listen_fd_, 64) < 0) throw NetworkException("Failed to listen on FTP socket");

    int flags = ::fcntl(listen_fd_, F_GETFL, 0);
    ::fcntl(listen_fd_, F_SETFL, flags | O_NONBLOCK);
}

void FtpServer::accept_client() {
    sockaddr_in addr{};
    socklen_t len = sizeof(addr);
    int cfd = ::accept(listen_fd_, reinterpret_cast<sockaddr*>(&addr), &len);
    if (cfd >= 0) {
        int flags = ::fcntl(cfd, F_GETFL, 0);
        ::fcntl(cfd, F_SETFL, flags | O_NONBLOCK);

        ClientData cd{FtpSession(cfd, root_dir_), ""};
        clients_.emplace(cfd, move(cd));
        Utils::send_all(cfd, Config::Ftp::GREETING.data(), Config::Ftp::GREETING.size());
    }
}

void FtpServer::close_client(int cfd) {
    auto it = clients_.find(cfd);
    if (it != clients_.end()) {
        ::close(cfd);
        clients_.erase(it);
    }
}

void FtpServer::process_received_lines(int cfd, ClientData& cd) {
    while (true) {
        size_t nl_pos = cd.recv_buf.find('\n');
        if (nl_pos == string::npos) break;

        string line = cd.recv_buf.substr(0, nl_pos);
        cd.recv_buf.erase(0, nl_pos + 1);

        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (!line.empty()) cd.session.process_command(line);

        if (cd.session.is_closed()) {
            close_client(cfd);
            break;
        }
    }
}

void FtpServer::handle_client(int cfd) {
    char buf[2048];
    ssize_t n = ::recv(cfd, buf, sizeof(buf), 0);
    if (n <= 0) {
        close_client(cfd);
        return;
    }

    ClientData& cd = clients_.at(cfd);
    cd.recv_buf.append(buf, n);
    process_received_lines(cfd, cd);
}

void FtpServer::start() {
    setup_listen_socket();
    is_running_ = true;
    cout << "[ftp-server] Listening on TCP port " << port_ << " with root directory: " << root_dir_ << "...\n";

    while (is_running_) {
        vector<pollfd> pfds{{listen_fd_, POLLIN, 0}};
        for (const auto& pair : clients_) pfds.push_back({pair.first, POLLIN, 0});

        int ret = ::poll(pfds.data(), static_cast<nfds_t>(pfds.size()), -1);
        if (ret < 0) {
            if (errno == EINTR) continue;
            break;
        }
        if (pfds[0].revents & POLLIN) accept_client();

        for (size_t i = 1; i < pfds.size(); ++i) {
            if (pfds[i].revents & (POLLIN | POLLERR | POLLHUP | POLLNVAL)) {
                handle_client(pfds[i].fd);
            }
        }
    }
}

void FtpServer::stop() {
    is_running_ = false;
    for (auto& pair : clients_) ::close(pair.first);
    clients_.clear();
    if (listen_fd_ >= 0) {
        ::close(listen_fd_);
        listen_fd_ = -1;
    }
}
