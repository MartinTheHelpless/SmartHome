#pragma once

#include <vector>
#include <cstdint>
#include <algorithm>

namespace smh
{
    class Object
    {
    protected:
        uint8_t UID_;

        bool dirty_;
        uint64_t last_update_timestamp_;

        std::vector<uint8_t> data_;
        std::vector<uint8_t> subscribers_;

    public:
        Object() = default;
        virtual ~Object() = default;

        virtual void send() = 0;
        virtual void receive() = 0;
        virtual void serialize() = 0;
        virtual void deserialize() = 0;

        void markDirty() { dirty_ = true; }
        void clearDirty() { dirty_ = false; }
        bool isDirty() const { return dirty_; }

        void addSubscriber(uint8_t uid)
        {
            if (std::find(subscribers_.begin(), subscribers_.end(), uid) == subscribers_.end())
                subscribers_.push_back(uid);
        }

        void removeSubscriber(uint8_t uid)
        {
            subscribers_.erase(std::remove(subscribers_.begin(), subscribers_.end(), uid), subscribers_.end());
        }
    };
}