#include "mode_b.hpp"
#include "../common/utils.hpp"

using namespace std;

namespace ModeB {

void send_data_stream(int data_fd, const char* data, size_t size) {
    if (size == 0) {
        uint8_t eof_block[3] = {DESCRIPTOR_EOF, 0x00, 0x00};
        Utils::send_all(data_fd, eof_block, 3);
        return;
    }

    size_t offset = 0;
    while (offset < size) {
        size_t remaining = size - offset;
        bool is_last = (remaining <= CHUNK_SIZE);
        size_t chunk_len = is_last ? remaining : CHUNK_SIZE;
        uint8_t desc = is_last ? DESCRIPTOR_EOF : DESCRIPTOR_DATA;

        uint8_t hdr[3]{desc, 0, 0};
        Utils::write_uint16_be(hdr + 1, static_cast<uint16_t>(chunk_len));

        Utils::send_all(data_fd, hdr, 3);
        Utils::send_all(data_fd, data + offset, chunk_len);
        offset += chunk_len;
    }
}

bool read_block(int data_fd, uint8_t& out_descriptor, vector<char>& out_payload) {
    uint8_t hdr[3];
    if (Utils::recv_exact(data_fd, hdr, 3) < 3) return false;

    out_descriptor = hdr[0];
    uint16_t count = Utils::read_uint16_be(hdr + 1);

    out_payload.resize(count);
    if (count > 0 && Utils::recv_exact(data_fd, out_payload.data(), count) < count) {
        return false;
    }
    return true;
}

} // namespace ModeB
