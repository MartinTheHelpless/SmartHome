#include "Device.hpp"

smh::Smh_Device device("test_device_out");

WiFiServer server(9000);

void setup()
{
    if (!device.Init())
        ;

    pinMode(LED_BUILTIN, OUTPUT);

    digitalWrite(LED_BUILTIN, LOW);
    delay(500);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(1500);
    device.send_peripheral_data_to_server();
}

void loop()
{
}