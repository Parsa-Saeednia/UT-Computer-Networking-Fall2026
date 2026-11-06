#ifndef SRC_PINGER_PING_SERVER_HPP
#define SRC_PINGER_PING_SERVER_HPP

#include <string>
#include <netinet/in.h>

class PingServer {
  public:
    PingServer(int port, double loss_rate, unsigned int seed = 0, bool has_seed = false);
    ~PingServer();

    void start();
    void stop();

  private:
    int port_;
    double loss_rate_;
    unsigned int seed_;
    bool has_seed_;
    int sockfd_ = -1;
    bool is_running_ = false;

    bool setup_socket();
    void process_incoming_packet();
    bool should_drop_packet() const;
    void send_reply(const std::string& message, const sockaddr_in& client_addr, socklen_t addr_len);
    bool validate_ascii(const std::string& raw) const;
    bool parse_ping_fields(const std::string& raw, std::string& seq, std::string& token) const;
    void handle_valid_ping(const std::string& seq, const std::string& token,
                           const sockaddr_in& client_addr, socklen_t addr_len);
};

#endif // SRC_PINGER_PING_SERVER_HPP
