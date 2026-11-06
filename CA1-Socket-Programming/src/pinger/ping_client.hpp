#ifndef SRC_PINGER_PING_CLIENT_HPP
#define SRC_PINGER_PING_CLIENT_HPP

#include <string>
#include <vector>
#include <chrono>
#include <netinet/in.h>

class PingClient {
  public:
    PingClient(std::string host, int port, int count, int timeout_ms, int interval_ms);
    ~PingClient();

    void run();

  private:
    std::string host_;
    int port_;
    int count_;
    int timeout_ms_;
    int interval_ms_;
    int sockfd_ = -1;

    bool setup_socket(sockaddr_in& server_addr);
    bool send_single_ping(int seq, const std::string& token, const sockaddr_in& server_addr);
    bool check_pong_packet(int seq, const char* buf, ssize_t len,
                           std::chrono::steady_clock::time_point send_time, double& out_rtt);
    bool wait_for_pong(int seq, std::chrono::steady_clock::time_point send_time, double& out_rtt);
    void print_summary(int sent, int received, const std::vector<double>& rtt_list) const;
    void ping_cycle(int seq, const sockaddr_in& server_addr, int& sent_count,
                    int& received_count, std::vector<double>& rtt_list);
};

#endif // SRC_PINGER_PING_CLIENT_HPP
