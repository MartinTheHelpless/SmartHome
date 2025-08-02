#include <cstdint>

#define DEFAULT_GPIO_CHIP "/dev/gpiochip0"

enum Smh_Msg_Type : uint8_t
{
    MSG_TYPE_GET = 0,        // Request data from server
    MSG_TYPE_POST = 1,       // Send status data to server (sensors, states, ...)
    MSG_TYPE_PING = 2,       // Keepalive message
    MSG_TYPE_SUBSCRIBE = 3,  // Subscribe to a device info
    MSG_TYPE_UNSUBSCRIBE = 4 // Unsubscribe to a device info
};
static_assert(sizeof(Smh_Msg_Type) == 1, "Smh_Msg_Type must be 1 byte");

#define SMH_FLAG_RESPONSE 10000000 // Expecting a response
#define SMH_FLAG_RESERVED 01000000 // Reserved for future flags
#define SMH_FLAG_RESERVED 00100000 // Reserved for future flags
#define SMH_FLAG_RESERVED 00010000 // Reserved for future flags
#define SMH_FLAG_RESERVED 00001000 // Reserved for future flags
#define SMH_FLAG_RESERVED 00000100 // Reserved for future flags
#define SMH_FLAG_RESERVED 00000010 // Reserved for future flags
#define SMH_FLAG_RESERVED 00000001 // Reserved for future flags
