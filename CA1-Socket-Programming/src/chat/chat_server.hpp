#ifndef SRC_CHAT_CHAT_SERVER_HPP
#define SRC_CHAT_CHAT_SERVER_HPP

#include "room_manager.hpp"
#include "protocol.hpp"

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class ChatServer {
  public:
    explicit ChatServer(int port);
    ~ChatServer();

    void start();
    void stop();

  private:
    struct ClientSession {
        int fd = -1;
        std::string nick;
        bool is_registered = false;
        std::string recv_buffer;
        bool discarding_oversized_line = false;
        std::unordered_set<std::string> joined_rooms;
    };

    int port_;
    int listen_fd_ = -1;
    bool is_running_ = false;

    RoomManager room_manager_;
    std::unordered_map<int, ClientSession> clients_;
    std::unordered_map<std::string, int> nick_to_fd_;

    void setup_listen_socket();
    void accept_new_client();
    void handle_client_activity(int client_fd);
    bool extract_next_line(ClientSession& client, std::string& out_line);
    void handle_disconnect(int client_fd);

    void process_raw_line(int client_fd, const std::string& line);
    void execute_command(int client_fd, const ChatProtocol::Command& cmd);

    void handle_nick(int client_fd, const ChatProtocol::Command& cmd);
    void handle_join(int client_fd, const ChatProtocol::Command& cmd);
    void handle_part(int client_fd, const ChatProtocol::Command& cmd);
    void handle_msg(int client_fd, const ChatProtocol::Command& cmd);
    void handle_privmsg(int client_fd, const ChatProtocol::Command& cmd);
    void handle_list(int client_fd, const ChatProtocol::Command& cmd);
    void handle_who(int client_fd, const ChatProtocol::Command& cmd);
    void handle_ping(int client_fd, const ChatProtocol::Command& cmd);
    void handle_quit(int client_fd, const ChatProtocol::Command& cmd);

    void send_line(int client_fd, const std::string& line);
    void broadcast_to_room_except(const std::string& room_name, const std::string& message, int except_fd);
};

#endif // SRC_CHAT_CHAT_SERVER_HPP
