#ifndef SRC_FTP_FTP_SERVER_HPP
#define SRC_FTP_FTP_SERVER_HPP

#include "ftp_session.hpp"

#include <string>
#include <unordered_map>

class FtpServer {
  public:
    FtpServer(int port, std::string root_dir);
    ~FtpServer();

    void start();
    void stop();

  private:
    int port_;
    std::string root_dir_;
    int listen_fd_ = -1;
    bool is_running_ = false;

    struct ClientData {
        FtpSession session;
        std::string recv_buf;
    };

    std::unordered_map<int, ClientData> clients_;

    void setup_listen_socket();
    void accept_client();
    void handle_client(int cfd);
    void close_client(int cfd);
    void process_received_lines(int cfd, ClientData& cd);
};

#endif // SRC_FTP_FTP_SERVER_HPP
