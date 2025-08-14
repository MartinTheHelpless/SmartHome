#include <cstdint>

#define CURRENT_PROTOCOL_VERSION 1.0

#define DEFAULT_GPIO_CHIP "/dev/gpiochip0"

#define DEFAULT_SERVER_PORT 9000
#define DEFAULT_SERVER_IP "192.168.1.100"

#define SMH_SERVER_UID 1

#define MAX_MESSAGE_SIZE 2048

enum Smh_Msg_Type : uint8_t
{
    MSG_TYPE_GET = 0,         // Request data from server
    MSG_TYPE_POST = 1,        // Send status data to server (sensors, states, ...)
    MSG_TYPE_PING = 2,        // Keepalive message
    MSG_TYPE_SUBSCRIBE = 3,   // Subscribe to a device info
    MSG_TYPE_UNSUBSCRIBE = 4, // Unsubscribe to a device info
    MSG_TYPE_CONTROLL = 5     // Controll destination device
};
static_assert(sizeof(Smh_Msg_Type) == 1, "Smh_Msg_Type must be 1 byte");

#define SMH_FLAG_RESPONSE 10000000 // Expecting a response
#define SMH_IS_INIT_MSG 01000000   // This message is an init message
#define SMH_FLAG_RESERVED 00100000 // Reserved for future flags
#define SMH_FLAG_RESERVED 00010000 // Reserved for future flags
#define SMH_FLAG_RESERVED 00001000 // Reserved for future flags
#define SMH_FLAG_RESERVED 00000100 // Reserved for future flags
#define SMH_FLAG_RESERVED 00000010 // Reserved for future flags
#define SMH_FLAG_RESERVED 00000001 // Reserved for future flags

/*
------------------------------------------------- Server File Tree Example -------------------------------------------------

/server_data
│
├── config/
│   ├── server.json              # Server-wide config (port, TLS, etc.)
│   ├── users.json               # Credentials / permissions
│   ├── devices_expected.json    # List of preconfigured devices
│   └── network.json             # Wi-Fi / network settings
│
├── devices/
│   ├── inside/
│   │   ├── kitchen_light_ab12cd.json
│   │   ├── kitchen_temp_ab12ce.json
│   │   └── livingroom_tv_ef34aa.json
│   │
│   └── outside/
│       ├── gate_controller_ff11dd.json
│       └── garden_temp_gh77ff.json
│
├── events/
│   ├── pending/                 # Event queues for devices
│   │   ├── ab12cd.queue
│   │   └── ef34aa.queue
│   └── processed/               # For debugging/logging, optional
│
├── static/
│   ├── html/                    # Web dashboard pages if you serve one
│   ├── css/
│   └── js/
│
├── logs/
│   ├── server.log
│   ├── connections.log
│   └── errors.log
│
└── tmp/
    ├── upload_cache/
    └── temp_messages/


*/

#define SMH_SERVER_DIR_PATH "/home/smh_data"
#define SMH_SERVER_DEVICE_PATH SMH_SERVER_DIR_PATH "/devices"
#define SMH_SERVER_OUT_DEVICES_PATH SMH_SERVER_DEVICE_PATH "/outside"
#define SMH_SERVER_IN_DEVICES_PATH SMH_SERVER_DEVICE_PATH "/inside"
