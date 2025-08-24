
#include "ESP_8266_Device.hpp"
#include <DHT.h>

#define DHTPIN 2 // GPIO number, not Dx label
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);

smh::ESP8266_Device device("test_device_out");

WiFiServer server(DEFAULT_CLIENT_PORT);

int test = 0;

void flash_pin(int pin)
{
    Serial.printf("Flashing pin: %d\n", pin);
    pinMode(pin, OUTPUT);
    digitalWrite(pin, HIGH);
    delay(300);
    digitalWrite(pin, LOW);
    delay(300);
    digitalWrite(pin, HIGH);
    delay(300);
    digitalWrite(pin, LOW);
    delay(300);
    digitalWrite(pin, HIGH);
}

void setup()
{
    if (!device.Init())
        return;

    pinMode(A0, INPUT);
    dht.begin();

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

void send_controll_data_message(int value)
{
    std::string message = string_format("%d", value);

    std::vector<uint8_t> payload(message.size(), 0);

    std::copy(message.begin(), message.end(), payload.data());

    smh::MessageHeader header = smh::create_header(device.get_uid(), device.get_uid(), MSG_CONTROL, SMH_FLAG_NONE, payload.size());

    std::shared_ptr<smh::Message> msg = std::make_shared<smh::Message>(header, payload);

    device.send_message_to_server(msg);
}

void loop()
{
    device.check_incomming_message(server);
    /*
    if (test == 200)
        send_controll_message(false), ++test;
    else if (test == 400)
        send_controll_message(true),
            test = 0;
    else
        ++test;
    */

    float t = dht.readTemperature();
    float h = dht.readHumidity();
    Serial.printf("Temp: %.1f °C  Hum: %.1f %%\n", t, h);

    device.set_sensor_value("temperature", std::to_string(t));
    device.set_sensor_value("humidity", std::to_string(h));
    device.send_peripheral_data_to_server();
    delay(2000);
}