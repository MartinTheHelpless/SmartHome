#include "ESP_32_Device.hpp"

smh::ESP32_Device device("test_device_out", "ssid", "pass");

void setup()
{
    if (!device.Init())
        return;

    std::map<std::string, std::string> controls = {{"LED", "on_OFF"}};

    device.set_peripherals_controls(controls);

    pinMode(A0, INPUT);
}

void loop()
{
    device.check_incomming_message();
    delay(2000);
}