#ifndef SRC_FTP_MODE_B_HPP
#define SRC_FTP_MODE_B_HPP

#include <cstdint>
#include <vector>
#include <cstddef>

namespace ModeB {

constexpr uint8_t DESCRIPTOR_DATA = 0x00;
constexpr uint8_t DESCRIPTOR_EOF  = 0x40;
constexpr size_t CHUNK_SIZE = 1024;

void send_data_stream(int data_fd, const char* data, size_t size);

bool read_block(int data_fd, uint8_t& out_descriptor, std::vector<char>& out_payload);

} // namespace ModeB

#endif // SRC_FTP_MODE_B_HPP
