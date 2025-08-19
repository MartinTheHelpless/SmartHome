#include <string>
#include <ESP8266WiFi.h>

#include <Arduino.h>

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

        std::string ssid = "tmp_netw";
        std::string password = "password";

        WiFiClient client;

    public:
        Smh_Device(std::string device_name) : device_name_(device_name) {}
        ~Smh_Device() {}

        bool Init()
        {
            connect_to_wifi();

            send_init_message();

            Message init_server_msg = receive_msg(); // This will be a response the the init message containing this device's UID

            if (!init_server_msg.is_valid())
                return false;

            device_uid_ = init_server_msg.get_header_dest_uid();

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

        int readSocket(WiFiClient &client, uint8_t *buffer, int to_read = MAX_MESSAGE_SIZE)
        {
            if (!client.connected())
                return 0;

            int bytesRead = 0;

            while (client.available() && bytesRead < to_read)
            {
                int c = client.read();
                if (c < 0)
                    break;
                buffer[bytesRead++] = (uint8_t)c;
            }

            return bytesRead;
        }

        bool connect_to_server()
        {
            if (!client.connected())
            {
                Serial.println("Connecting to server...");
                if (client.connect(server_ip_.c_str(), server_port_))
                {
                    Serial.println("Connected to server!");
                    return true;
                }
                else
                {
                    Serial.println("Failed to connect to server");
                    return false;
                }
            }
            return true;
        }

        void send_message_to_server(const Message &msg)
        {
            if (!client.connected() && !connect_to_server())
                return;

            uint8_t buffer[MAX_MESSAGE_SIZE] = {0};

            size_t size = msg.serialize(buffer);

            size_t sent = client.write(buffer, size);

            Serial.printf("Sent %d bytes\n", sent);
        }

        Message receive_msg()
        {
            uint8_t buffer[MAX_MESSAGE_SIZE] = {0};

            int bytes_read = readSocket(client, buffer);

            Message msg((char *)buffer, bytes_read);

            return msg;
        }

        bool send_init_message()
        {
            if (!connect_to_server())
                return false;

            MessageHeader header = create_header(0, SMH_SERVER_UID, MSG_POST, SMH_FLAG_IS_INIT_MSG, device_name_.length());

            Message msg(header, device_name_.c_str());

            send_message_to_server(msg);
            return true;
        }
    };
}