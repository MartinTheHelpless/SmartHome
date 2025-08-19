#pragma once

#include <vector>
#include <cstdint>
#include <cstring>
#include "Header.hpp"
#include "../../Utils/Defines.hpp"

namespace smh
{
    class Message
    {
    private:
        MessageHeader header_;
        std::vector<uint8_t> payload_buffer_;
        uint16_t payload_size_ = 0;
        bool is_valid_ = false;

    public:
        Message(const std::vector<uint8_t> &buffer)
        {
            if (buffer.size() < sizeof(MessageHeader))
            {
                is_valid_ = false;
                return;
            }

            std::memcpy(&header_, buffer.data(), sizeof(MessageHeader));

            if (buffer.size() != sizeof(MessageHeader) + header_.payload_size)
            {
                is_valid_ = false;
                return;
            }

            if (header_.payload_size > 0)
            {
                payload_buffer_.resize(header_.payload_size);
                std::copy(buffer.begin() + sizeof(MessageHeader),
                          buffer.end(),
                          payload_buffer_.begin());
            }

            is_valid_ = check_header_valid();
        }

        Message(const char *full_buffer, int read_bytes)
        {
            if (read_bytes < static_cast<int>(sizeof(MessageHeader)))
            {
                is_valid_ = false;
                return;
            }

            std::memcpy(&header_, full_buffer, sizeof(MessageHeader));
            payload_size_ = header_.payload_size;

            if (read_bytes < static_cast<int>(sizeof(MessageHeader) + payload_size_))
            {
                is_valid_ = false;
                return;
            }

            if (payload_size_ > 0)
                payload_buffer_ = std::vector<uint8_t>(full_buffer + sizeof(MessageHeader),
                                                       full_buffer + sizeof(MessageHeader) + payload_size_);

            is_valid_ = check_header_valid();
        }

        Message(const MessageHeader &header)
            : header_(header)
        {
            is_valid_ = check_header_valid();
        }

        Message(const MessageHeader &header, const std::vector<uint8_t> &payload)
            : header_(header), payload_buffer_(payload)
        {
            header_.payload_size = static_cast<uint16_t>(payload.size());
            is_valid_ = check_header_valid();
        }

        ~Message() = default;

        std::vector<uint8_t> serialize() const
        {
            std::vector<uint8_t> buffer(sizeof(MessageHeader) + payload_buffer_.size());

            std::memcpy(buffer.data(), &header_, sizeof(MessageHeader));

            if (!payload_buffer_.empty())
            {
                std::copy(payload_buffer_.begin(), payload_buffer_.end(), buffer.begin() + sizeof(MessageHeader));
            }

            return buffer;
        }

        std::vector<uint8_t> serialize_header() const
        {
            std::vector<uint8_t> buffer(sizeof(MessageHeader));
            std::memcpy(buffer.data(), &header_, sizeof(MessageHeader));
            return buffer;
        }

        bool check_header_valid() const
        {
            return header_.version == CURRENT_PROTOCOL_VERSION;
        }

        uint16_t get_payload_size() const { return header_.payload_size; }
        const std::vector<uint8_t> &get_payload() const { return payload_buffer_; }

        int get_header_version() const { return header_.version; }
        int get_header_flags() const { return header_.flags; }
        int get_header_source_uid() const { return header_.source_uid; }
        int get_header_dest_uid() const { return header_.dest_uid; }
        Smh_Msg_Type get_message_type() const { return header_.message_type; }

        bool is_init_msg() const { return (header_.flags & SMH_FLAG_IS_INIT_MSG) != 0; }
        bool is_valid() const { return is_valid_; }

        void set_header(const MessageHeader &header)
        {
            header_ = header;
            is_valid_ = check_header_valid();
        }

        void set_payload(const std::vector<uint8_t> &payload)
        {
            payload_buffer_ = payload;
            header_.payload_size = static_cast<uint16_t>(payload.size());
        }

        std::string get_payload_str() const
        {
            return std::string(payload_buffer_.begin(), payload_buffer_.end());
        }
    };
}
