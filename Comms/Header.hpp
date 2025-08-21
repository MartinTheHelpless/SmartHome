#pragma once

#include <cstdint>
#include <cstring>
#include "../Utils/Defines.hpp"

namespace smh
{
#pragma pack(push, 1)
    struct MessageHeader
    {
        uint8_t version;           // Protocol version
        uint8_t flags;             // Bit flags (see above)
        uint8_t source_uid;        // Sender UID
        uint8_t dest_uid;          // Receiver UID
        Smh_Msg_Type message_type; // GET, POST, etc.
        uint16_t payload_size;     // Actual size of payload
    };
#pragma pack(pop)

    static MessageHeader create_header(uint8_t source, uint8_t dest, Smh_Msg_Type msg_type, uint8_t flags = SMH_FLAG_NONE,
                                       uint16_t pld_size = 0, uint8_t protocol_version = CURRENT_PROTOCOL_VERSION)
    {
        MessageHeader header;
        header.source_uid = source;
        header.dest_uid = dest;
        header.message_type = msg_type;
        header.flags = flags;
        header.payload_size = pld_size;
        header.version = protocol_version;

        return header;
    }
}
