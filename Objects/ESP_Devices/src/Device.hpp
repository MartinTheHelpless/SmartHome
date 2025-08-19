#include <string>
#include <Arduino.h>
#include <ESP8266WiFi.h>

#include <iterator>
#include <memory>

#include "../../../Utils/Defines.hpp"
#include "../../Comms/Message.hpp"

namespace smh
{

    class Smh_Device
    {
    protected:
        std::string ip_ = "";
        std::string device_name_ = "";

        uint8_t device_uid_ = 0;

        std::string server_ip_ = "10.9.170.225";
        uint16_t server_port_ = DEFAULT_SERVER_PORT;

        std::string ssid = "RETIA-Guest";
        std::string password = "53002341";

        WiFiClient client;

    public:
        Smh_Device(std::string device_name) : device_name_(device_name) {}
        ~Smh_Device() {}

        bool Init()
        {
            connect_to_wifi();

            if (!connect_to_server())
                return false;

            send_init_message();

            std::shared_ptr<Message> init_server_msg = receive_msg(); // This will be a response the the init message containing this device's UID

            if (!init_server_msg->is_valid())
            {
                Serial.println("Received init message from server is invalid");
                return false;
            }

            device_uid_ = init_server_msg->get_header_dest_uid();

            Serial.printf("This Devices's assigned UID: %d\nDevice initialization successful\n", device_uid_);

            client.stop();

            return true;
        }

        void connect_to_wifi()
        {
            Serial.begin(115200);
            WiFi.begin(ssid.c_str(), password.c_str());
            Serial.print("Connecting to Wi-Fi");

            while (WiFi.status() != WL_CONNECTED)
            {
                delay(500);
                Serial.print(".");
            }

            Serial.println("\nWi-Fi connected!");
            Serial.print("ESP IP address: ");
            Serial.println(WiFi.localIP());
        }

        std::vector<uint8_t> readSocket(WiFiClient &client, int to_read = MAX_MESSAGE_SIZE)
        {
            if (!client.connected())
                return std::vector<uint8_t>();

            int bytesRead = 0;
            std::vector<uint8_t> buffer;

            while (client.available() && bytesRead < to_read)
            {
                int c = client.read();
                if (c < 0)
                    break;
                buffer.push_back((uint8_t)c);
            }
            Serial.printf("buffer size = %d\n", buffer.size());

            return buffer;
        }

        bool connect_to_server()
        {
            if (!client.connected())
            {
                if (client.connect("10.9.170.225", 9000))
                    Serial.println("Connected to server!");
                else
                {
                    Serial.println("Failed to connect to server");
                    return false;
                }
            }
            return true;
        }

        void send_message_to_server(std::shared_ptr<Message> msg)
        {
            auto buffer = msg->serialize();

            size_t sent = client.write(buffer.data(), buffer.size());

            Serial.printf("Sent %d bytes\n", sent);
        }

        std::shared_ptr<Message> receive_msg()
        {
            auto buffer = readSocket(client);

            std::shared_ptr<Message> msg = std::make_shared<Message>(buffer);

            return msg;
        }

        bool send_init_message()
        {
            MessageHeader header = create_header(0, SMH_SERVER_UID, MSG_POST, SMH_FLAG_IS_INIT_MSG, device_name_.length());

            std::shared_ptr<Message> msg = std::make_shared<Message>(header, device_name_);

            send_message_to_server(msg);

            return true;
        }
    };
}