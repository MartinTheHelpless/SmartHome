#pragma once

#include <cstdint>
#include <cstring>
#include "../../Utils/Defines.hpp"

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

}
