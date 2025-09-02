#include <string>
#include <Arduino.h>
#include <WiFi.h>

#include <iterator>
#include <memory>
#include <exception>
#include <iomanip>

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
        std::string ip_;
        std::string device_name_;

        uint8_t device_uid_ = 0;

        std::string server_ip_ = DEFAULT_SERVER_IP;
        uint16_t server_port_ = DEFAULT_SERVER_PORT;

        std::string ssid;
        std::string password;

        std::map<std::string, std::string> peripherals_sensors;
        std::map<std::string, std::string> peripherals_controls;

        // TODO: Add peripheral send logic to init message on server and here
        // Peripherals will be sent with their data, not in inti message.
        // if new peripherals added, no need for init to have that, only get data
        // and send with peripheral type/name

        WiFiClient client_out, client_in;
        WiFiServer server;

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

        virtual void handle_msg_received(const std::shared_ptr<Message> &msg) { Serial.println("Message received"); }

    public:
        ESP32_Device() = delete;

        ESP32_Device(std::string device_name, std::string wifi_ssid, std::string wifi_pass,
                     std::string server_ip = DEFAULT_SERVER_IP, uint16_t srv_port = DEFAULT_SERVER_PORT)
            : device_name_(device_name), server_ip_(server_ip), server_port_(srv_port), ssid(wifi_ssid), password(wifi_pass), server(srv_port)
        {
        }
        ~ESP32_Device() = default;

        bool Init()
        {
            if (!connect_to_wifi())
                return false;

            server.begin();

            while (!connect_to_server())
                delay(2000);

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

        void check_incomming_message()
        {
            client_in = server.accept();
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

        bool send_peripheral_data_to_server()
        {
            if ((peripherals_sensors.size() | peripherals_controls.size()) == 0)
                return send_ping_to_server();

            std::string data;

            for (const auto &[peripheral, value] : peripherals_sensors)
                data += string_format("PERIPHERAL_S:%s:%s;", peripheral.c_str(), value.c_str());

            for (const auto &[peripheral, value] : peripherals_controls)
                data += string_format("PERIPHERAL_C:%s:%s;", peripheral.c_str(), value.c_str());

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

        void set_peripherals_sensors(const std::map<std::string, std::string> sensors) { peripherals_sensors = sensors; }

        void set_peripherals_controls(const std::map<std::string, std::string> controls) { peripherals_controls = controls; }
    };
}