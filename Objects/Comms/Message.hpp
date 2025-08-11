#include <cstring>

#include "Header.hpp"
#include "Defines.hpp"
namespace smh
{

    class Message
    {
    private:
        MessageHeader header;

        int buffer_data_size_ = 0;

        char buffer_[MAX_MESSAGE_SIZE] = {0};
        char payload_buffer_[MAX_MESSAGE_SIZE] = {0};

        uint16_t payload_size_;

    public:
        Message(const char buffer[MAX_MESSAGE_SIZE], int read_bytes) : buffer_data_size_(read_bytes)
        {
            memcpy(buffer_, buffer, MAX_MESSAGE_SIZE);
        }

        ~Message() = default;

        bool deserialize_header()
        {
            if (buffer_data_size_ < sizeof(MessageHeader))
                return false;

            std::memcpy(&header, buffer_, sizeof(MessageHeader));

            if (header.payload_size >= MAX_MESSAGE_SIZE)
                return false;

            payload_size_ = header.payload_size;

            memcpy(payload_buffer_, buffer_ + sizeof(MessageHeader), payload_size_);

            return true;
        }

        void serialize_header(char *dest_buffer[9])
        {
            std::memcpy(dest_buffer, &header, sizeof(MessageHeader));
        }

        uint8_t get_header_version() { return header.version; }
        uint8_t get_header_flags() { return header.flags; }
        uint8_t get_header_source_uid() { return header.source_uid; }
        uint8_t get_header_dest_uid() { return header.dest_uid; }
        Smh_Msg_Type get_header_message_type() { return header.message_type; }
        uint16_t get_header_payload_size() { return header.payload_size; }

        bool response_expected() { return (header.flags & SMH_FLAG_RESPONSE != 0); }
        bool is_init_msg() { return (header.flags & SMH_IS_INIT_MSG != 0); }
    };

}