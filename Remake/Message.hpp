#include <vector>
#include <cstdint>
#include <boost/asio.hpp>

#ifndef MESSAGE_VERSION
#define MESSAGE_VERSION 1.0
#endif

#define SERVER_UID 1
#define SERVER_IP ""

enum MessageType
{
    Something = 0,
    Nothing,
    Else
};

namespace bas = boost::asio;
namespace basip = boost::asio::ip;

struct Message
{

private:
    std::vector<uint8_t> payload;

public:
    MessageType type;

    double version;

    uint32_t flags;
    uint32_t length;
    uint32_t dest_uid;
    uint32_t source_uid;

    basip::address_v4 source;
    basip::address_v4 destination;

    Message(MessageType msg_type, uint32_t source, uint32_t flgs)
        : type(msg_type), version(MESSAGE_VERSION), flags(flgs), length(0), dest_uid(0), source_uid(source) {}

    Message(MessageType msg_type, uint32_t source, uint32_t destinat, uint32_t flgs)
        : type(msg_type), version(MESSAGE_VERSION), flags(flgs), length(0), dest_uid(destinat), source_uid(source) {}
    ~Message();

    void set_payload(std::vector<uint8_t> payload)
    {
        length = payload.size();
        this->payload = payload;
    }
};