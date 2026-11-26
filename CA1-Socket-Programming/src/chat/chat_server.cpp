#include "chat_server.hpp"
#include "../common/config.hpp"
#include "../common/utils.hpp"
#include "../common/exception.hpp"

#include <iostream>
#include <algorithm>
#include <cstring>
#include <poll.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>

using namespace std;

ChatServer::ChatServer(int port) : port_(port) {}

ChatServer::~ChatServer() {
    stop();
}

void ChatServer::setup_listen_socket() {
    ::signal(SIGPIPE, SIG_IGN);
    listen_fd_ = ::socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd_ < 0) throw SocketCreationException("Failed to create TCP listen socket");

    int opt = 1;
    ::setsockopt(listen_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(static_cast<uint16_t>(port_));

    if (::bind(listen_fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        ::close(listen_fd_);
        listen_fd_ = -1;
        throw BindException("Failed to bind TCP listen socket to port " + to_string(port_));
    }
    if (::listen(listen_fd_, 64) < 0) throw NetworkException("Failed to listen on TCP socket");

    int flags = ::fcntl(listen_fd_, F_GETFL, 0);
    ::fcntl(listen_fd_, F_SETFL, flags | O_NONBLOCK);
}

void ChatServer::send_line(int client_fd, const string& line) {
    Utils::send_all(client_fd, line.data(), line.size());
}

void ChatServer::broadcast_to_room_except(const string& room_name, const string& message, int except_fd) {
    auto members = room_manager_.get_members_except(room_name, except_fd);
    for (int m_fd : members) {
        send_line(m_fd, message);
    }
}

void ChatServer::accept_new_client() {
    sockaddr_in client_addr{};
    socklen_t client_len = sizeof(client_addr);
    int client_fd = ::accept(listen_fd_, reinterpret_cast<sockaddr*>(&client_addr), &client_len);
    if (client_fd >= 0) {
        int flags = ::fcntl(client_fd, F_GETFL, 0);
        ::fcntl(client_fd, F_SETFL, flags | O_NONBLOCK);

        ClientSession session;
        session.fd = client_fd;
        clients_[client_fd] = session;
        send_line(client_fd, Config::Chat::GREETING);
    }
}

void ChatServer::handle_disconnect(int client_fd) {
    auto it = clients_.find(client_fd);
    if (it == clients_.end()) return;

    ClientSession& client = it->second;
    if (client.is_registered) {
        auto left_rooms = room_manager_.remove_client_from_all(client_fd);
        for (const auto& room_name : left_rooms) {
            broadcast_to_room_except(room_name, "EVENT LEAVE " + room_name + " " + client.nick + "\n", client_fd);
        }
        nick_to_fd_.erase(client.nick);
    }
    ::close(client_fd);
    clients_.erase(it);
}

void ChatServer::handle_nick(int client_fd, const ChatProtocol::Command& cmd) {
    ClientSession& client = clients_[client_fd];
    if (cmd.arg1.empty() || cmd.arg1.find(' ') != string::npos || !Utils::is_valid_nick(cmd.arg1)) {
        send_line(client_fd, ChatProtocol::make_error_response(ChatProtocol::ErrorCode::BAD_SYNTAX, "BAD_SYNTAX"));
        return;
    }
    if (client.is_registered) {
        send_line(client_fd, ChatProtocol::make_error_response(ChatProtocol::ErrorCode::ALREADY_REGISTERED, "ALREADY_REGISTERED"));
        return;
    }
    if (nick_to_fd_.find(cmd.arg1) != nick_to_fd_.end()) {
        send_line(client_fd, ChatProtocol::make_error_response(ChatProtocol::ErrorCode::NICK_IN_USE, "NICK_IN_USE"));
        return;
    }
    client.nick = cmd.arg1;
    client.is_registered = true;
    nick_to_fd_[cmd.arg1] = client_fd;
    send_line(client_fd, "OK NICK " + cmd.arg1 + "\n");
}

void ChatServer::handle_join(int client_fd, const ChatProtocol::Command& cmd) {
    ClientSession& client = clients_[client_fd];
    if (cmd.arg1.empty() || cmd.arg1.find(' ') != string::npos || !Utils::is_valid_room(cmd.arg1)) {
        send_line(client_fd, ChatProtocol::make_error_response(ChatProtocol::ErrorCode::BAD_SYNTAX, "BAD_SYNTAX"));
        return;
    }
    if (room_manager_.is_member(client_fd, cmd.arg1)) {
        send_line(client_fd, ChatProtocol::make_error_response(ChatProtocol::ErrorCode::ALREADY_IN_ROOM, "ALREADY_IN_ROOM"));
        return;
    }
    room_manager_.add_member(client_fd, cmd.arg1);
    client.joined_rooms.insert(cmd.arg1);
    send_line(client_fd, "OK JOIN " + cmd.arg1 + "\n");
    broadcast_to_room_except(cmd.arg1, "EVENT JOIN " + cmd.arg1 + " " + client.nick + "\n", client_fd);
}

void ChatServer::handle_part(int client_fd, const ChatProtocol::Command& cmd) {
    ClientSession& client = clients_[client_fd];
    if (cmd.arg1.empty() || cmd.arg1.find(' ') != string::npos || !Utils::is_valid_room(cmd.arg1)) {
        send_line(client_fd, ChatProtocol::make_error_response(ChatProtocol::ErrorCode::BAD_SYNTAX, "BAD_SYNTAX"));
        return;
    }
    if (!room_manager_.is_member(client_fd, cmd.arg1)) {
        send_line(client_fd, ChatProtocol::make_error_response(ChatProtocol::ErrorCode::NOT_IN_ROOM, "NOT_IN_ROOM"));
        return;
    }
    room_manager_.remove_member(client_fd, cmd.arg1);
    client.joined_rooms.erase(cmd.arg1);
    send_line(client_fd, "OK PART " + cmd.arg1 + "\n");
    broadcast_to_room_except(cmd.arg1, "EVENT LEAVE " + cmd.arg1 + " " + client.nick + "\n", client_fd);
}

void ChatServer::handle_msg(int client_fd, const ChatProtocol::Command& cmd) {
    ClientSession& client = clients_[client_fd];
    if (!Utils::is_valid_room(cmd.arg1) || cmd.arg2.empty()) {
        send_line(client_fd, ChatProtocol::make_error_response(ChatProtocol::ErrorCode::BAD_SYNTAX, "BAD_SYNTAX"));
        return;
    }
    if (!room_manager_.is_member(client_fd, cmd.arg1)) {
        send_line(client_fd, ChatProtocol::make_error_response(ChatProtocol::ErrorCode::NOT_IN_ROOM, "NOT_IN_ROOM"));
        return;
    }
    uint64_t msg_id = room_manager_.next_message_id(cmd.arg1);
    send_line(client_fd, "OK MSG " + cmd.arg1 + " " + to_string(msg_id) + "\n");
    broadcast_to_room_except(cmd.arg1, "FROM " + cmd.arg1 + " " + client.nick + " " +
                             to_string(msg_id) + " " + cmd.arg2 + "\n", client_fd);
}

void ChatServer::handle_privmsg(int client_fd, const ChatProtocol::Command& cmd) {
    ClientSession& client = clients_[client_fd];
    if (!Utils::is_valid_nick(cmd.arg1) || cmd.arg2.empty()) {
        send_line(client_fd, ChatProtocol::make_error_response(ChatProtocol::ErrorCode::BAD_SYNTAX, "BAD_SYNTAX"));
        return;
    }
    auto it = nick_to_fd_.find(cmd.arg1);
    if (it == nick_to_fd_.end()) {
        send_line(client_fd, ChatProtocol::make_error_response(ChatProtocol::ErrorCode::NO_SUCH_NICK, "NO_SUCH_NICK"));
        return;
    }
    send_line(client_fd, "OK PRIVMSG " + cmd.arg1 + "\n");
    send_line(it->second, "PRIV " + client.nick + " " + cmd.arg2 + "\n");
}

void ChatServer::handle_list(int client_fd, const ChatProtocol::Command& cmd) {
    if (!cmd.arg1.empty()) {
        send_line(client_fd, ChatProtocol::make_error_response(ChatProtocol::ErrorCode::BAD_SYNTAX, "BAD_SYNTAX"));
        return;
    }
    auto sorted_rooms = room_manager_.get_all_rooms_sorted();
    string resp = "OK LIST " + to_string(sorted_rooms.size()) + "\n";
    for (const auto& r : sorted_rooms) {
        resp += "ROOM " + r.first + " " + to_string(r.second) + "\n";
    }
    send_line(client_fd, resp);
}

void ChatServer::handle_who(int client_fd, const ChatProtocol::Command& cmd) {
    if (cmd.arg1.empty() || cmd.arg1.find(' ') != string::npos || !Utils::is_valid_room(cmd.arg1)) {
        send_line(client_fd, ChatProtocol::make_error_response(ChatProtocol::ErrorCode::BAD_SYNTAX, "BAD_SYNTAX"));
        return;
    }
    if (!room_manager_.room_exists(cmd.arg1)) {
        send_line(client_fd, ChatProtocol::make_error_response(ChatProtocol::ErrorCode::NO_SUCH_ROOM, "NO_SUCH_ROOM"));
        return;
    }
    auto member_fds = room_manager_.get_members(cmd.arg1);
    vector<string> nicks;
    for (int m_fd : member_fds) {
        auto it = clients_.find(m_fd);
        if (it != clients_.end() && it->second.is_registered) nicks.push_back(it->second.nick);
    }
    sort(nicks.begin(), nicks.end());

    string resp = "OK WHO " + cmd.arg1 + " " + to_string(nicks.size()) + "\n";
    for (const auto& nick : nicks) resp += "USER " + nick + "\n";
    send_line(client_fd, resp);
}

void ChatServer::handle_ping(int client_fd, const ChatProtocol::Command& cmd) {
    if (!cmd.arg1.empty()) {
        send_line(client_fd, ChatProtocol::make_error_response(ChatProtocol::ErrorCode::BAD_SYNTAX, "BAD_SYNTAX"));
        return;
    }
    send_line(client_fd, Config::Chat::PONG);
}

void ChatServer::handle_quit(int client_fd, const ChatProtocol::Command& cmd) {
    if (!cmd.arg1.empty()) {
        send_line(client_fd, ChatProtocol::make_error_response(ChatProtocol::ErrorCode::BAD_SYNTAX, "BAD_SYNTAX"));
        return;
    }
    send_line(client_fd, Config::Chat::OK_BYE);
    handle_disconnect(client_fd);
}

void ChatServer::execute_command(int client_fd, const ChatProtocol::Command& cmd) {
    ClientSession& client = clients_[client_fd];
    if (cmd.type == ChatProtocol::CommandType::UNKNOWN) {
        send_line(client_fd, ChatProtocol::make_error_response(ChatProtocol::ErrorCode::UNKNOWN_COMMAND, "UNKNOWN_COMMAND"));
        return;
    }
    if (!client.is_registered && cmd.type != ChatProtocol::CommandType::NICK &&
        cmd.type != ChatProtocol::CommandType::PING && cmd.type != ChatProtocol::CommandType::QUIT) {
        send_line(client_fd, ChatProtocol::make_error_response(ChatProtocol::ErrorCode::NOT_REGISTERED, "NOT_REGISTERED"));
        return;
    }

    if (cmd.type == ChatProtocol::CommandType::NICK) handle_nick(client_fd, cmd);
    else if (cmd.type == ChatProtocol::CommandType::JOIN) handle_join(client_fd, cmd);
    else if (cmd.type == ChatProtocol::CommandType::PART) handle_part(client_fd, cmd);
    else if (cmd.type == ChatProtocol::CommandType::MSG) handle_msg(client_fd, cmd);
    else if (cmd.type == ChatProtocol::CommandType::PRIVMSG) handle_privmsg(client_fd, cmd);
    else if (cmd.type == ChatProtocol::CommandType::LIST) handle_list(client_fd, cmd);
    else if (cmd.type == ChatProtocol::CommandType::WHO) handle_who(client_fd, cmd);
    else if (cmd.type == ChatProtocol::CommandType::PING) handle_ping(client_fd, cmd);
    else if (cmd.type == ChatProtocol::CommandType::QUIT) handle_quit(client_fd, cmd);
}

void ChatServer::process_raw_line(int client_fd, const string& line) {
    if (line.empty()) return;
    if (line.size() > static_cast<size_t>(Config::Chat::MAX_LINE_BYTES)) {
        send_line(client_fd, ChatProtocol::make_error_response(ChatProtocol::ErrorCode::LINE_TOO_LONG, "LINE_TOO_LONG"));
        return;
    }
    execute_command(client_fd, ChatProtocol::parse_command(line));
}

bool ChatServer::extract_next_line(ClientSession& client, string& out_line) {
    size_t nl_pos = client.recv_buffer.find('\n');
    if (nl_pos == string::npos) {
        if (!client.discarding_oversized_line && client.recv_buffer.size() > 1024) {
            send_line(client.fd, ChatProtocol::make_error_response(ChatProtocol::ErrorCode::LINE_TOO_LONG, "LINE_TOO_LONG"));
            client.discarding_oversized_line = true;
        }
        if (client.discarding_oversized_line) client.recv_buffer.clear();
        return false;
    }

    out_line = client.recv_buffer.substr(0, nl_pos);
    client.recv_buffer.erase(0, nl_pos + 1);
    if (client.discarding_oversized_line) {
        client.discarding_oversized_line = false;
        return extract_next_line(client, out_line);
    }
    if (!out_line.empty() && out_line.back() == '\r') out_line.pop_back();
    return true;
}

void ChatServer::handle_client_activity(int client_fd) {
    char buf[2048];
    ssize_t n = ::recv(client_fd, buf, sizeof(buf), 0);
    if (n <= 0) {
        handle_disconnect(client_fd);
        return;
    }

    ClientSession& client = clients_[client_fd];
    client.recv_buffer.append(buf, n);

    string line;
    while (extract_next_line(client, line)) {
        process_raw_line(client_fd, line);
        if (clients_.find(client_fd) == clients_.end()) break;
    }
}

void ChatServer::start() {
    setup_listen_socket();
    is_running_ = true;
    cout << "[chat-server] Listening on TCP port " << port_ << "...\n";

    while (is_running_) {
        vector<pollfd> pfds{{listen_fd_, POLLIN, 0}};
        for (const auto& pair : clients_) pfds.push_back({pair.first, POLLIN, 0});

        int ret = ::poll(pfds.data(), static_cast<nfds_t>(pfds.size()), -1);
        if (ret < 0) {
            if (errno == EINTR) continue;
            break;
        }
        if (pfds[0].revents & POLLIN) accept_new_client();

        for (size_t i = 1; i < pfds.size(); ++i) {
            if (pfds[i].revents & (POLLIN | POLLERR | POLLHUP | POLLNVAL)) {
                handle_client_activity(pfds[i].fd);
            }
        }
    }
}

void ChatServer::stop() {
    is_running_ = false;
    for (auto& pair : clients_) ::close(pair.first);
    clients_.clear();
    if (listen_fd_ >= 0) {
        ::close(listen_fd_);
        listen_fd_ = -1;
    }
}
