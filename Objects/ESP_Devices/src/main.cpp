
#include "Device.hpp"

smh::Smh_Device device("test_device_out");

WiFiServer server(DEFAULT_CLIENT_PORT);

int test = 0;

void setup()
{
    if (!device.Init())
        return;

    delay(1500);

    // device.send_peripheral_data_to_server();

    server.begin();
}

void send_controll_message(bool on)
{
    std::string message;

    if (on)
        message = "LED:ON";
    else
        message = "LED:OFF";

    std::vector<uint8_t> payload(message.size(), 0);

    std::copy(message.begin(), message.end(), payload.data());

    smh::MessageHeader header = smh::create_header(device.get_uid(), device.get_uid(), MSG_CONTROL, SMH_FLAG_NONE, payload.size());

    std::shared_ptr<smh::Message> msg = std::make_shared<smh::Message>(header, payload);

    device.send_message_to_server(msg);
}

void loop()
{
    device.check_incomming_message(server);
    delay(5);
    /*if (test == 200)
        send_controll_message(false), ++test;
    else if (test == 400)
        send_controll_message(true),
            test = 0;
    else
        ++test;*/
}