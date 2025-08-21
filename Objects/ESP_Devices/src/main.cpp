#include "Device.hpp"

smh::Smh_Device device("test_device_out");

WiFiServer server(9000);

void setup()
{
    if (!device.Init())
        return;

    delay(1500);

    // device.send_peripheral_data_to_server();

    Serial.println("Sending the control message");

    std::string message = "LED:OFF";

    std::vector<uint8_t> payload(message.size(), 0);

    std::copy(message.begin(), message.end(), payload.data());

    smh::MessageHeader header = smh::create_header(device.get_uid(), device.get_uid(), MSG_CONTROL, SMH_FLAG_NONE, payload.size());

    std::shared_ptr<smh::Message> msg = std::make_shared<smh::Message>(header, payload);

    device.send_message_to_server(msg);

    Serial.println("Control message sent ");
}

void loop()
{
    device.handle_server_messages();
    delay(5);
}