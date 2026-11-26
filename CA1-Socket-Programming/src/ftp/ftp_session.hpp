#ifndef SRC_FTP_FTP_SESSION_HPP
#define SRC_FTP_FTP_SESSION_HPP

#include <string>
#include <cstdint>

class FtpSession {
  public:
    explicit FtpSession(int ctrl_fd, std::string root_dir);
    ~FtpSession();

    FtpSession(const FtpSession&) = delete;
    FtpSession& operator=(const FtpSession&) = delete;

    FtpSession(FtpSession&& other) noexcept;
    FtpSession& operator=(FtpSession&& other) noexcept;

    void process_command(const std::string& line);
    void close_data();

    bool is_closed() const { return is_closed_; }

  private:
    int ctrl_fd_ = -1;
    std::string root_dir_;

    std::string user_name_;
    bool pass_expected_ = false;
    bool logged_in_ = false;
    bool mode_b_enabled_ = false;
    bool is_closed_ = false;

    int pasv_listen_fd_ = -1;
    int data_fd_ = -1;
    bool pasv_active_ = false;

    void send_reply(const std::string& reply);
    bool ensure_data_connected();
    std::string format_pasv_response(uint16_t port) const;
    bool validate_transfer_preconditions(const std::string& filename);
    std::string build_directory_listing() const;
    bool receive_stor_file(int data_fd, const std::string& tmp_name);
    void execute_stor_finalize(const std::string& tmp_name, const std::string& final_name, bool success);

    void handle_user(const std::string& args);
    void handle_pass(const std::string& args);
    void handle_mode(const std::string& args);
    void handle_pasv();
    void handle_list(const std::string& args);
    void handle_retr(const std::string& args);
    void handle_stor(const std::string& args);
    void handle_quit();
};

#endif // SRC_FTP_FTP_SESSION_HPP
