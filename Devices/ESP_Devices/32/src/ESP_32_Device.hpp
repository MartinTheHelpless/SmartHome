#include <string>
#include <Arduino.h>
#include <WiFi.h>

#include <iterator>
#include <memory>
#include <exception>

#include "Utils/Defines.hpp"
#include "Comms/Message.hpp"
#include "Utils/Helper_Functions.hpp"

#define LED_BUILTIN 2

template <typename... Args>
std::string string_format(const std::string &format, Args... args)
{
    int size_s = std::snprintf(nullptr, 0, format.c_str(), args...) + 1; // Extra space for '\0'
    if (size_s <= 0)
        return "";
    auto size = static_cast<size_t>(size_s);
    std::unique_ptr<char[]> buf(new char[size]);
    std::snprintf(buf.get(), size, format.c_str(), args...);
    return std::string(buf.get(), buf.get() + size - 1); // We don't want the '\0' inside
}

namespace smh
{
    class ESP32_Device
    {
    protected:
        std::string ip_ = "";
        std::string device_name_ = "";

        uint8_t device_uid_ = 0;

        std::string server_ip_ = "10.42.0.1";
        uint16_t server_port_ = DEFAULT_SERVER_PORT;

        std::string ssid = "tmp_netw";
        std::string password = "password";

        std::vector<std::pair<std::string, std::string>> peripherals_sensors = {
            {"temperature", "1200.7"},
            {"pressure", "1800.4"},
            {"humidity", "45.0"},
            {"UV", "6"}

        };

        std::vector<std::pair<std::string, std::string>> peripherals_controls = {
            {"LED", "on_OFF"}

        }; // TODO: Add peripheral send logic to init message on server and here
        // Peripherals will be sent with their data, not in inti message.
        // if new peripherals added, no need for init to have that, only get data
        // and send with peripheral type/name

        WiFiClient client_out;
        WiFiClient client_in;

        std::vector<uint8_t>
        readSocket(WiFiClient &client, int to_read = MAX_MESSAGE_SIZE)
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

        std::shared_ptr<Message> receive_msg()
        {
            const int HEADER_SIZE = sizeof(MessageHeader);

            unsigned long start = millis();
            while (client_out.available() < HEADER_SIZE)
            {
                if (millis() - start > 2000)
                    return std::make_shared<Message>(std::vector<uint8_t>());
                delay(5);
            }

            std::vector<uint8_t> buffer(HEADER_SIZE);
            client_out.read(buffer.data(), HEADER_SIZE);

            MessageHeader header;
            memcpy(&header, buffer.data(), HEADER_SIZE);

            std::vector<uint8_t> payload(header.payload_size);
            int bytesRead = 0;
            start = millis();
            while (bytesRead < header.payload_size)
            {
                int avail = client_out.available();
                if (avail > 0)
                {
                    int toRead = std::min(avail, header.payload_size - bytesRead);
                    bytesRead += client_out.read(payload.data() + bytesRead, toRead);
                }
                if (millis() - start > 2000)
                    break;
                delay(5);
            }

            buffer.insert(buffer.end(), payload.begin(), payload.begin() + bytesRead);

            return std::make_shared<Message>(header, payload);
        }

    public:
        bool send_message_to_server(std::shared_ptr<Message> msg)
        {
            if (!client_out.connected() && !connect_to_server())
                return false;

            auto buffer = msg->serialize();

            size_t sent = client_out.write(buffer.data(), buffer.size());

            Serial.printf("Sent %d bytes\n", sent);

            if (sent != buffer.size())
                return false;

            return true;
        }

        std::shared_ptr<Message> receive_msg_from(WiFiClient &sock)
        {
            const int HEADER_SIZE = sizeof(MessageHeader);
            unsigned long start = millis();
            while (sock.available() < HEADER_SIZE)
            {
                if (millis() - start > 2000)
                    return std::make_shared<Message>(std::vector<uint8_t>());
                delay(5);
            }

            std::vector<uint8_t> buffer(HEADER_SIZE);
            sock.read(buffer.data(), HEADER_SIZE);
            MessageHeader header;
            memcpy(&header, buffer.data(), HEADER_SIZE);

            std::vector<uint8_t> payload(header.payload_size);
            int bytesRead = 0;
            start = millis();
            while (bytesRead < header.payload_size)
            {
                int avail = sock.available();
                if (avail > 0)
                {
                    int toRead = std::min(avail, header.payload_size - bytesRead);
                    bytesRead += sock.read(payload.data() + bytesRead, toRead);
                }
                if (millis() - start > 2000)
                    break;
                delay(5);
            }

            buffer.insert(buffer.end(), payload.begin(), payload.begin() + bytesRead);
            return std::make_shared<Message>(header, payload);
        }

        virtual void handle_msg_received(const std::shared_ptr<Message> &msg)
        {
            switch (msg->get_message_type())
            {
            case Smh_Msg_Type::MSG_CONTROL:
            {
                Serial.println("Received control message");
                auto control_messages = split_string(msg->get_payload_str().c_str(), ';');
                for (auto message : control_messages)
                {
                    auto periph_action = split_string(message.c_str(), ':');
                    if (periph_action.size() != 2)
                        continue;

                    if (periph_action[0] == "LED")
                    {
                        if (periph_action[1] == "ON")
                        {
                            Serial.println("Turning LED ON");
                            digitalWrite(LED_BUILTIN, LOW);
                        }
                        else if (periph_action[1] == "OFF")
                        {
                            Serial.println("Turning LED OFF");
                            digitalWrite(LED_BUILTIN, HIGH);
                        }
                        else
                            Serial.printf("Unknown action \"%s\" on peripheral %s",
                                          periph_action[1].c_str(), periph_action[0].c_str());
                    }
                }
                break;
            }
            default:
                break;
            }
        }

        void check_incomming_message(WiFiServer &server)
        {
            client_in = server.available();
            if (client_in)
            {
                Serial.println("Client connected!");
                while (client_in.connected())
                {
                    while (client_in.available())
                    {
                        uint8_t buffer[MAX_MESSAGE_SIZE];
                        size_t bytes_received = client_in.read(buffer, MAX_MESSAGE_SIZE); // non-blocking read
                        if (bytes_received > 0)
                        {
                            std::vector<uint8_t> data(bytes_received);
                            std::copy(buffer, buffer + bytes_received, data.data());
                            handle_msg_received(std::make_shared<smh::Message>(data));
                        }
                    }
                }
                client_in.stop();
                Serial.println("Client disconnected");
            }
        }

        uint8_t get_uid() { return device_uid_; }

        ESP32_Device(std::string device_name) : device_name_(device_name)
        {
            pinMode(LED_BUILTIN, OUTPUT);
            digitalWrite(LED_BUILTIN, HIGH);
        }
        ~ESP32_Device() {}

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

            client_out.stop();

            Serial.printf("This Devices's assigned UID: %d\nDevice initialization successful\n", device_uid_);

            return true;
        }

        bool connect_to_wifi()
        {
            Serial.begin(115200);
            WiFi.begin(ssid.c_str(), password.c_str());
            Serial.print("\nConnecting to Wi-Fi");

            unsigned long start = millis();
            while (WiFi.status() != WL_CONNECTED)
            {
                if (millis() - start > 10000)
                { // 10s timeout
                    Serial.println("\nWiFi connect failed!");
                    return false;
                }
                delay(500); // yields to FreeRTOS
                Serial.print(".");
            }
            Serial.println("\nWi-Fi connected!");
            Serial.print("ESP IP address: ");
            Serial.println(WiFi.localIP());
            return true;
        }

        bool connect_to_server()
        {
            if (!client_out.connected())
            {
                Serial.printf("Trying to connect to a server on %s:%d\n", server_ip_.c_str(), server_port_);
                if (client_out.connect(server_ip_.c_str(), server_port_))
                    Serial.println("Connected to server!");
                else
                {
                    Serial.println("Failed to connect to server");
                    return false;
                }
            }
            return true;
        }

        bool send_init_message()
        {
            MessageHeader header = create_header(0, SMH_SERVER_UID, MSG_POST, SMH_FLAG_IS_INIT_MSG, device_name_.length());

            std::vector<uint8_t> payload(device_name_.length(), 0);
            std::copy(device_name_.begin(), device_name_.end(), payload.begin());

            std::shared_ptr<Message> msg = std::make_shared<Message>(header, payload);

            return send_message_to_server(msg);
        }

        bool send_peripheral_data_to_server()
        {
            if (peripherals_sensors.size() | peripherals_controls.size() == 0)
                return send_ping_to_server();

            std::string data;

            for (const auto &[peripheral, value] : peripherals_sensors)
                data += string_format("PERIPHERAL_S:%s:%s;", peripheral.c_str(), value.c_str());

            for (const auto &[peripheral, value] : peripherals_controls)
                data += string_format("PERIPHERAL_C:%s:%s;", peripheral.c_str(), value.c_str());

            data[data.length() - 1] = 0;

            MessageHeader header = create_header(device_uid_, SMH_SERVER_UID, MSG_POST);

            std::vector<uint8_t> payload(data.length(), 0);
            std::copy(data.begin(), data.end(), payload.data());

            std::shared_ptr<Message> msg = std::make_shared<Message>(header, payload);

            bool result = send_message_to_server(msg);
            client_out.stop();
            return result;
        }

        bool send_ping_to_server()
        {
            MessageHeader header = create_header(device_uid_, SMH_SERVER_UID, MSG_PING);
            std::shared_ptr<Message> msg = std::make_shared<Message>(header);

            bool result = send_message_to_server(msg);
            Serial.println("Sent ping (keepalive) to server");
            client_out.stop();
            return result;
        }
    };
}