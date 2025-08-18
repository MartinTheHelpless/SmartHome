#include "Device.hpp"

smh::Smh_Device device("test_device_out");

void setup()
{
    if (!device.Init())
        while (true)
            ;
}

void loop()
{
}