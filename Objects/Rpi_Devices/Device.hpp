#include <iostream>
#include <cstdint>
#include <vector>

namespace smh
{
    class Device
    {
    private:
        uint8_t UID_;
        std::string IP_;
        std::string device_name; // pseudo sw MAC

        std::vector<std::string> peripherals;

    public:
        Device() {}
        ~Device() {}
    };

}