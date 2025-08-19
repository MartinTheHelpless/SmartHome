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

        std::string full_buffer_;

        uint16_t payload_size_;
        std::string payload_buffer_;

        bool is_valid_ = false;

    public:
        Message(const char full_buffer[MAX_MESSAGE_SIZE], int read_bytes)
            : full_buffer_size_(read_bytes), full_buffer_(full_buffer), payload_size_(header_.payload_size)
        {
            is_valid_ = deserialize_header();

            if (header_.payload_size > 0)
                std::copy(full_buffer_.begin() + sizeof(MessageHeader), full_buffer_.end(), payload_buffer_.data());
        }

        Message(MessageHeader header) : header_(header), payload_size_(header.payload_size)
        {
            is_valid_ = check_header_valid();
        }

        Message(MessageHeader header, std::string payload_buffer)
            : header_(header), payload_size_(header.payload_size), payload_buffer_(payload_buffer)
        {
            is_valid_ = check_header_valid();
        }

        Message(std::vector<uint8_t> buffer)
        {
            if (buffer.size() < 7)
            {
                is_valid_ = false;
                return;
            }

            uint8_t *ptr = reinterpret_cast<uint8_t *>(&header_);
            std::copy(buffer.begin(), buffer.begin() + sizeof(MessageHeader), ptr);

            if (header_.payload_size > 0)
                std::copy(buffer.begin() + sizeof(MessageHeader), buffer.end(), payload_buffer_.data());

            is_valid_ = check_header_valid();
        }

        ~Message() = default;

        std::vector<uint8_t> serialize()
        {
            std::vector<uint8_t> buffer(sizeof(MessageHeader) + payload_size_, 0);

            uint8_t *ptr = reinterpret_cast<uint8_t *>(&header_);

            std::copy(ptr, ptr + sizeof(MessageHeader), buffer.data());

            if (payload_size_ > 0)
                std::copy(payload_buffer_.begin(), payload_buffer_.end(), buffer.data() + sizeof(MessageHeader));

            return buffer;
        }

        bool deserialize_header()
        {
            uint8_t *ptr = reinterpret_cast<uint8_t *>(&header_);

            std::copy(full_buffer_.begin(), full_buffer_.begin() + sizeof(MessageHeader), ptr); // (&header_, full_buffer_, sizeof(MessageHeader));

            if (header_.payload_size >= MAX_MESSAGE_SIZE)
                return false;

            payload_size_ = header_.payload_size;
            full_buffer_size_ = sizeof(MessageHeader) + header_.payload_size;

            std::copy(full_buffer_.begin() + sizeof(MessageHeader), full_buffer_.end(), payload_buffer_.data());

            if (header_.version != CURRENT_PROTOCOL_VERSION)
                return false;
            return true;
        }

        std::vector<uint8_t> serialize_header()
        {
            std::vector<uint8_t> buffer(sizeof(MessageHeader), 0);

            uint8_t *ptr = reinterpret_cast<uint8_t *>(&header_);

            std::copy(ptr, ptr + sizeof(MessageHeader), buffer.begin());

            return buffer;
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