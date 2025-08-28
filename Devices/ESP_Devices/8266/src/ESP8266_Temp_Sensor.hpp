#include "ESP_8266_Device.hpp"

namespace smh
{
    class Temp_Sensor : public ESP8266_Device
    {

    protected:
    private:
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
                    Serial.println(msg->get_payload_str().c_str());
                    auto periph_action = split_string(message.c_str(), ':');
                    if (periph_action.size() != 2)
                        continue;

                    if (periph_action[0] == "LED")
                    {
                        if (periph_action[1] == "ON")
                        {
                            Serial.println("Turning LED ON");
                            digitalWrite(LED_BUILTIN, LOW);
                            peripherals_controls[periph_action[0]] = "ON_off";
                        }
                        else if (periph_action[1] == "OFF")
                        {
                            Serial.println("Turning LED OFF");
                            digitalWrite(LED_BUILTIN, HIGH);
                            peripherals_controls[periph_action[0]] = "on_OFF";
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

    public:
        Temp_Sensor(std::string device_name, std::string wifi_ssid, std::string wifi_pass, std::string srv_ip = DEFAULT_SERVER_IP, uint16_t srv_port = DEFAULT_SERVER_PORT)
            : ESP8266_Device(device_name, wifi_ssid, wifi_pass, srv_ip, srv_port)
        {
            pinMode(LED_BUILTIN, OUTPUT);
            digitalWrite(LED_BUILTIN, HIGH);
        }
        ~Temp_Sensor() = default;

        void set_sensor_value(std::string sensor, double value)
        {
            std::ostringstream oss;
            oss << std::fixed << std::setprecision(1) << value; // 1 decimal place
            peripherals_sensors[sensor] = oss.str();
        }
    };
}