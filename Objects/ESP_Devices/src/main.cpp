#include "Device.hpp"

smh::Smh_Device device("test_device_out");

WiFiServer server(9000);

void setup()
{
    if (!device.Init())
        return;

    delay(1500);

    device.send_peripheral_data_to_server();
}

void loop()
{
    device.handle_in_socket();
    delay(5);
}