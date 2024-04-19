#ifndef LOGCABIN_PROTOCOL_FLAIRPROTOCOL_H
#define LOGCABIN_PROTOCOL_FLAIRPROTOCOL_H

#include <cinttypes>
#include <ostream>
#include <cstdint>

namespace LogCabin {
namespace Protocol {
namespace FlairProtocol{

constexpr uint16_t FLAIR_PORT = 23233;

constexpr uint8_t OP_WRITE_REQUEST = 0;
constexpr uint8_t OP_READ_REQUEST = 1;
constexpr uint8_t OP_WRITE_REPLY = 2;
constexpr uint8_t OP_READ_REPLY = 3;
constexpr uint8_t OP_WRITE_FAILED = 100;
constexpr uint8_t OP_READ_FAILED = 101;
constexpr uint8_t OP_UNKNOWN = 255;

constexpr uint64_t FLAIR_KEY_SIZE = 16;

struct FlairProtocol {
    uint8_t from_leader;
    uint8_t opcode;
    char key[FLAIR_KEY_SIZE];
    uint64_t seq;
    uint32_t sid;
    uint64_t log_idx;
    uint8_t cflwrs;
};

void fromBigEndian(FlairProtocol& flair_hdr);
void toBigEndian(FlairProtocol& flair_hdr);

}
}
}

#endif