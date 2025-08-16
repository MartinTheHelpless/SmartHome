#pragma once

#include <cstring>

#include "Header.hpp"
#include "../../Utils/Defines.hpp"
namespace smh
{

    class Message
    {
    private:
        MessageHeader header_;

        int full_buffer_data_size_ = 0;

        uint8_t full_buffer_[MAX_MESSAGE_SIZE] = {0};
        char payload_buffer_[MAX_MESSAGE_SIZE] = {0};

        uint16_t payload_size_ = 0;

    public:
        Message(const char buffer[MAX_MESSAGE_SIZE], int read_bytes) : full_buffer_data_size_(read_bytes)
        {
            memcpy(full_buffer_, buffer, MAX_MESSAGE_SIZE);
            deserialize_header();
        }

        Message(MessageHeader header) : header_(header)
        {
            deserialize_header();
        }

        Message(MessageHeader header, const char *buffer)
            : header_(header), payload_size_(header.payload_size)
        {
            deserialize_header();
            memcpy(payload_buffer_, buffer, payload_size_);
            std::cout << payload_buffer_ << std::endl;
        }

        ~Message() = default;

        bool deserialize(const uint8_t *src_buffer, size_t size)
        {
            if (size < sizeof(MessageHeader))
                return false;

            std::memcpy(full_buffer_, src_buffer, size);

            std::memcpy(&header_, src_buffer, sizeof(MessageHeader));
            payload_size_ = header_.payload_size;

            if (payload_size_ > 0)
            {
                if (payload_size_ + sizeof(MessageHeader) > size || payload_size_ >= MAX_MESSAGE_SIZE)
                    return false;
                std::memcpy(payload_buffer_, src_buffer + sizeof(MessageHeader), payload_size_);
            }

            return true;
        }

        size_t serialize(uint8_t *dest_buffer) const
        {
            std::memcpy(dest_buffer, &header_, sizeof(MessageHeader));
            if (payload_size_ > 0)
                std::memcpy(dest_buffer + sizeof(MessageHeader), payload_buffer_, payload_size_);
            std::cout << "Payload size : " << payload_size_ << std::endl;
            return sizeof(MessageHeader) + payload_size_;
        }

        bool deserialize_header()
        {
            if (full_buffer_data_size_ < sizeof(MessageHeader))
                return false;

            std::memcpy(&header_, full_buffer_, sizeof(MessageHeader));

            if (header_.payload_size >= MAX_MESSAGE_SIZE)
                return false;

            payload_size_ = header_.payload_size;

            memcpy(payload_buffer_, full_buffer_ + sizeof(MessageHeader), payload_size_);

            return true;
        }

        void serialize_header(char *dest_buffer[9])
        {
            std::memcpy(dest_buffer, &header_, sizeof(MessageHeader));
        }

        void set_header(const MessageHeader &header) { header_ = header; }

        int get_header_version() { return header_.version; }
        int get_header_flags() { return header_.flags; }
        int get_header_source_uid() { return header_.source_uid; }
        int get_header_dest_uid() { return header_.dest_uid; }
        Smh_Msg_Type get_message_type() { return header_.message_type; }
        uint16_t get_header_payload_size() { return header_.payload_size; }

        bool response_expected() { return (header_.flags & SMH_FLAG_RESPONSE != 0); }
        bool is_init_msg() { return ((header_.flags & SMH_IS_INIT_MSG) != 0); }
        std::string get_payload_str() { return std::string(payload_buffer_); }
    };
}