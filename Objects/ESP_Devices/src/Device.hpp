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

        std::string server_ip_ = "10.42.0.1";
        uint16_t server_port_ = DEFAULT_SERVER_PORT;

        std::string ssid = "tmp_netw";
        std::string password = "password";

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

            client.stop();

            Serial.printf("This Devices's assigned UID: %d\nDevice initialization successful\n", device_uid_);

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
                if (client.connect(server_ip_.c_str(), server_port_))
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

            std::shared_ptr<Message> tmp = std::make_shared<Message>(buffer);

            Serial.println(tmp->get_payload_str().c_str());
            Serial.println(msg->get_payload_str().c_str());

            size_t sent = client.write(buffer.data(), buffer.size());

            Serial.printf("Sent %d bytes\n", sent);
        }

        std::shared_ptr<Message> receive_msg()
        {
            const int HEADER_SIZE = sizeof(MessageHeader);

            unsigned long start = millis();
            while (client.available() < HEADER_SIZE)
            {
                if (millis() - start > 2000)
                    return std::make_shared<Message>(std::vector<uint8_t>());
                delay(5);
            }

            std::vector<uint8_t> buffer(HEADER_SIZE);
            client.read(buffer.data(), HEADER_SIZE);

            MessageHeader header;
            memcpy(&header, buffer.data(), HEADER_SIZE);

            std::vector<uint8_t> payload(header.payload_size);
            int bytesRead = 0;
            start = millis();
            while (bytesRead < header.payload_size)
            {
                int avail = client.available();
                if (avail > 0)
                {
                    int toRead = std::min(avail, header.payload_size - bytesRead);
                    bytesRead += client.read(payload.data() + bytesRead, toRead);
                }
                if (millis() - start > 2000)
                    break;
                delay(5);
            }

            buffer.insert(buffer.end(), payload.begin(), payload.begin() + bytesRead);

            return std::make_shared<Message>(buffer);
        }

        bool send_init_message()
        {
            MessageHeader header = create_header(0, SMH_SERVER_UID, MSG_POST, SMH_FLAG_IS_INIT_MSG, device_name_.length());

            std::vector<uint8_t> payload(device_name_.length(), 0);
            std::copy(device_name_.begin(), device_name_.end(), payload.begin());

            std::shared_ptr<Message> msg = std::make_shared<Message>(header, payload);

            send_message_to_server(msg);

            return true;
        }
    };
}