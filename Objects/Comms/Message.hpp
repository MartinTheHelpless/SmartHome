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

        int full_buffer_size_ = 0;

        uint8_t full_buffer_[MAX_MESSAGE_SIZE] = {0};
        char payload_buffer_[MAX_MESSAGE_SIZE] = {0};

        uint16_t payload_size_ = 0;

        bool is_valid_ = false;

    public:
        Message(const char full_buffer[MAX_MESSAGE_SIZE], int read_bytes) : full_buffer_size_(read_bytes)
        {
            memcpy(full_buffer_, full_buffer, MAX_MESSAGE_SIZE);
            is_valid_ = deserialize_header();

            if (header_.payload_size > 0)
                memcpy(payload_buffer_, full_buffer + sizeof(MessageHeader), header_.payload_size);
            payload_size_ = header_.payload_size;
        }

        Message(MessageHeader header) : header_(header), payload_size_(header.payload_size)
        {
            is_valid_ = check_header_valid();
        }

        Message(MessageHeader header, const char *payload_buffer)
            : header_(header), payload_size_(header.payload_size)
        {
            is_valid_ = check_header_valid();
            memcpy(payload_buffer_, payload_buffer, payload_size_);
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
            // std::cout << "Sent payload size : " << header_.payload_size << std::endl;
            return sizeof(MessageHeader) + payload_size_;
        }

        bool deserialize_header()
        {
            std::memcpy(&header_, full_buffer_, sizeof(MessageHeader));

            if (header_.payload_size >= MAX_MESSAGE_SIZE)
                return false;

            payload_size_ = header_.payload_size;
            full_buffer_size_ = sizeof(MessageHeader) + header_.payload_size;

            memcpy(payload_buffer_, full_buffer_ + sizeof(MessageHeader), payload_size_);

            if (header_.version != CURRENT_PROTOCOL_VERSION)
                return false;

            return true;
        }

        void serialize_header(char *dest_buffer[9])
        {
            std::memcpy(dest_buffer, &header_, sizeof(MessageHeader));
        }

        bool check_header_valid()
        {
            return CURRENT_PROTOCOL_VERSION == header_.version;
        }

        void set_header(const MessageHeader &header) { header_ = header; }

        int get_header_version() const { return header_.version; }
        int get_header_flags() const { return header_.flags; }
        int get_header_source_uid() const { return header_.source_uid; }
        int get_header_dest_uid() const { return header_.dest_uid; }
        Smh_Msg_Type get_message_type() const { return header_.message_type; }
        uint16_t get_header_payload_size() const { return header_.payload_size; }

        bool is_init_msg() const { return ((header_.flags & SMH_FLAG_IS_INIT_MSG) != 0); }
        bool is_valid() const { return is_valid_; }
        std::string get_payload_str() const { return std::string(payload_buffer_); }
    };
}