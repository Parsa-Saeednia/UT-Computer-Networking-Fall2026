#ifndef SRC_CHAT_CHAT_CLIENT_HPP
#define SRC_CHAT_CHAT_CLIENT_HPP

#include <string>

class ChatClient {
  public:
    ChatClient(std::string host, int port);
    ~ChatClient();

    void run();

  private:
    std::string host_;
    int port_;
    int sockfd_ = -1;

    void connect_to_server();
    void handle_server_incoming(std::string& recv_buf);
    void handle_user_input();
    void process_received_lines(std::string& recv_buf);
};

#endif // SRC_CHAT_CHAT_CLIENT_HPP
