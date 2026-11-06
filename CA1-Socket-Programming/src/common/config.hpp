#ifndef SRC_COMMON_CONFIG_HPP
#define SRC_COMMON_CONFIG_HPP

#include <string>
#include <cstdint>

namespace Config {

namespace Common {
    constexpr int DEFAULT_BUFFER_SIZE = 1024;
    constexpr int MAX_LINE_LENGTH = 1024;
} // namespace Common

namespace Pinger {
    constexpr int DEFAULT_PORT = 9000;
    constexpr int DEFAULT_COUNT = 10;
    constexpr int DEFAULT_TIMEOUT_MS = 1000;
    constexpr int DEFAULT_INTERVAL_MS = 0;
    constexpr double DEFAULT_LOSS_RATE = 0.0;
    constexpr int MAX_DATAGRAM_SIZE = 512;
    constexpr int MAX_TOKEN_LEN = 64;
    constexpr int MAX_SEQ_DIGITS = 10;
} // namespace Pinger

namespace Chat {
    constexpr int DEFAULT_PORT = 8080;
    constexpr int MAX_LINE_BYTES = 1024;
    constexpr int MAX_NICK_LEN = 16;
    constexpr int MAX_ROOM_LEN = 32;

    const std::string GREETING = "HELLO UT-CHAT/1.0\n";
    const std::string PONG = "PONG\n";
    const std::string OK_BYE = "OK BYE\n";
} // namespace Chat

namespace Ftp {
    constexpr int DEFAULT_PORT = 2121;
    constexpr int MAX_FILENAME_LEN = 64;
    constexpr int MODE_B_BLOCK_SIZE = 1024;
    constexpr uint8_t DESCRIPTOR_DATA = 0x00;
    constexpr uint8_t DESCRIPTOR_EOF = 0x40;

    const std::string GREETING = "220 UT-FTP ready\r\n";
} // namespace Ftp

} // namespace Config

#endif // SRC_COMMON_CONFIG_HPP
