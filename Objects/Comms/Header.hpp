#include <cstdint>

namespace smh
{
#pragma pack(push, 1)
    struct MessageHeader
    {
        uint8_t version;       // Protocol version
        uint8_t flags;         // Bit flags (see above)
        uint8_t source_uid;    // Sender UID
        uint8_t dest_uid;      // Receiver UID
        uint8_t message_type;  // GET, POST, etc.
        uint16_t payload_size; // Actual size of payload
        uint16_t padding_size; // Size of padding after payload
    };
#pragma pack(pop)
}