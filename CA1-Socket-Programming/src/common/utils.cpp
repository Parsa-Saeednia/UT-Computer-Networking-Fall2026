#include "utils.hpp"

#include <cctype>
#include <cstring>
#include <errno.h>
#include <sys/socket.h>

using namespace std;

namespace Utils {

string trim(const string& str) {
    size_t start = 0;
    while (start < str.size() && (str[start] == ' ' || str[start] == '\t' || str[start] == '\r' || str[start] == '\n')) {
        ++start;
    }
    size_t end = str.size();
    while (end > start && (str[end - 1] == ' ' || str[end - 1] == '\t' || str[end - 1] == '\r' || str[end - 1] == '\n')) {
        --end;
    }
    return str.substr(start, end - start);
}

pair<string, string> split_first(const string& str, char delimiter) {
    size_t pos = str.find(delimiter);
    if (pos == string::npos) {
        return {str, ""};
    }
    return {str.substr(0, pos), str.substr(pos + 1)};
}

bool is_valid_nick(const string& nick) {
    if (nick.empty() || nick.size() > 16) return false;
    for (char c : nick) {
        if (!isalnum(static_cast<unsigned char>(c)) && c != '_') {
            return false;
        }
    }
    return true;
}

bool is_valid_room(const string& room) {
    if (room.size() < 2 || room.size() > 32 || room[0] != '#') return false;
    for (size_t i = 1; i < room.size(); ++i) {
        char c = room[i];
        if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_' || c == '-')) {
            return false;
        }
    }
    return true;
}

bool is_valid_ftp_filename(const string& filename) {
    if (filename.empty() || filename.size() > 64) return false;
    if (filename.find('/') != string::npos || filename.find('\\') != string::npos) return false;
    if (filename == "." || filename == "..") return false;
    for (char c : filename) {
        if (!isalnum(static_cast<unsigned char>(c)) && c != '.' && c != '_' && c != '-') {
            return false;
        }
    }
    return true;
}

bool is_valid_ping_token(const string& token) {
    if (token.empty() || token.size() > 64) return false;
    for (char c : token) {
        unsigned char uc = static_cast<unsigned char>(c);
        if (uc < 33 || uc > 126) return false;
    }
    return true;
}

bool is_valid_ping_seq(const string& seq_str) {
    if (seq_str.empty() || seq_str.size() > 10) return false;
    for (char c : seq_str) {
        if (!isdigit(static_cast<unsigned char>(c))) return false;
    }
    return true;
}

uint16_t read_uint16_be(const uint8_t* buffer) {
    return (static_cast<uint16_t>(buffer[0]) << 8) | static_cast<uint16_t>(buffer[1]);
}

void write_uint16_be(uint8_t* buffer, uint16_t value) {
    buffer[0] = static_cast<uint8_t>((value >> 8) & 0xFF);
    buffer[1] = static_cast<uint8_t>(value & 0xFF);
}

ssize_t send_all(int sockfd, const void* buffer, size_t length) {
    const char* ptr = static_cast<const char*>(buffer);
    size_t total_sent = 0;
    while (total_sent < length) {
        ssize_t sent = ::send(sockfd, ptr + total_sent, length - total_sent, 0);
        if (sent < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        if (sent == 0) break;
        total_sent += sent;
    }
    return total_sent;
}

ssize_t recv_exact(int sockfd, void* buffer, size_t length) {
    char* ptr = static_cast<char*>(buffer);
    size_t total_recv = 0;
    while (total_recv < length) {
        ssize_t recvd = ::recv(sockfd, ptr + total_recv, length - total_recv, 0);
        if (recvd < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        if (recvd == 0) break;
        total_recv += recvd;
    }
    return total_recv;
}

string CommandLineParser::normalize_flag(const string& flag) {
    if (flag.rfind("--", 0) == 0) return flag.substr(2);
    if (flag.rfind("-", 0) == 0) return flag.substr(1);
    return flag;
}

CommandLineParser::CommandLineParser(int argc, char* argv[]) {
    for (int i = 1; i < argc; ++i) {
        string arg = argv[i];
        if (arg.rfind("-", 0) == 0) {
            string key = normalize_flag(arg);
            if (i + 1 < argc && argv[i + 1][0] != '-') {
                flags_[key] = argv[++i];
            } else {
                flags_[key] = "true";
            }
        }
    }
}

bool CommandLineParser::has(const string& flag) const {
    return flags_.find(normalize_flag(flag)) != flags_.end();
}

string CommandLineParser::get_string(const string& flag, const string& default_value) const {
    auto it = flags_.find(normalize_flag(flag));
    if (it != flags_.end()) return it->second;
    return default_value;
}

int CommandLineParser::get_int(const string& flag, int default_value) const {
    auto it = flags_.find(normalize_flag(flag));
    if (it != flags_.end()) {
        try {
            return stoi(it->second);
        } catch (...) {
            return default_value;
        }
    }
    return default_value;
}

double CommandLineParser::get_double(const string& flag, double default_value) const {
    auto it = flags_.find(normalize_flag(flag));
    if (it != flags_.end()) {
        try {
            return stod(it->second);
        } catch (...) {
            return default_value;
        }
    }
    return default_value;
}

} // namespace Utils
