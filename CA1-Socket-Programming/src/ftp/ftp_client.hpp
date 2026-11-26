#ifndef SRC_FTP_FTP_CLIENT_HPP
#define SRC_FTP_FTP_CLIENT_HPP

#include <string>

class FtpClient {
  public:
    FtpClient(std::string host, int port);
    ~FtpClient();

    void run();

  private:
    std::string host_;
    int port_;
    int ctrl_fd_ = -1;
    int pending_data_fd_ = -1;
    std::string ctrl_recv_buf_;

    void connect_control();
    std::string read_control_line();
    void send_control_line(const std::string& line);

    bool request_pasv(std::string& out_ip, int& out_port);
    int connect_data(const std::string& ip, int port);
    int ensure_data_connection();

    void read_and_print_mode_b_data(int dfd);
    void read_and_write_mode_b_file(int dfd, const std::string& filename);

    void handle_list();
    void handle_retr(const std::string& filename);
    void handle_stor(const std::string& filename);
    bool process_user_command(const std::string& trimmed);
};

#endif // SRC_FTP_FTP_CLIENT_HPP
