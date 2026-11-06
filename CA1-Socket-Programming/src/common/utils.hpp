#ifndef SRC_COMMON_UTILS_HPP
#define SRC_COMMON_UTILS_HPP

#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>
#include <unistd.h>

namespace Utils {

std::string trim(const std::string& str);

std::pair<std::string, std::string> split_first(const std::string& str, char delimiter);

bool is_valid_nick(const std::string& nick);

bool is_valid_room(const std::string& room);

bool is_valid_ftp_filename(const std::string& filename);

bool is_valid_ping_token(const std::string& token);

bool is_valid_ping_seq(const std::string& seq_str);

uint16_t read_uint16_be(const uint8_t* buffer);

void write_uint16_be(uint8_t* buffer, uint16_t value);

ssize_t send_all(int sockfd, const void* buffer, size_t length);

ssize_t recv_exact(int sockfd, void* buffer, size_t length);

class CommandLineParser {
  public:
    CommandLineParser(int argc, char* argv[]);

    bool has(const std::string& flag) const;
    std::string get_string(const std::string& flag, const std::string& default_value = "") const;
    int get_int(const std::string& flag, int default_value) const;
    double get_double(const std::string& flag, double default_value) const;

  private:
    std::unordered_map<std::string, std::string> flags_;
    static std::string normalize_flag(const std::string& flag);
};

} // namespace Utils

#endif // SRC_COMMON_UTILS_HPP
