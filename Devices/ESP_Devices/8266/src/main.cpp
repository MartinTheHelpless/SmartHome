
#include "ESP8266_Temp_Sensor.hpp"
#include <DHT.h>

#define DHTPIN 2 // GPIO number, not Dx label
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);

smh::Temp_Sensor device("Pergola Meteostation Outside", "STARNET-Burian", "pilir3453");

void setup()
{
    if (!device.Init())
        return;

    std::map<std::string, std::string> controls = {{"LED", "on_OFF"}};

    device.set_peripherals_controls(controls);

    pinMode(A0, INPUT);
    dht.begin();
}

void loop()
{
    device.check_incomming_message();

    float t = dht.readTemperature();
    float h = dht.readHumidity();
    Serial.printf("Temp: %.1f °C  Hum: %.1f %%\n", t, h);

    device.set_sensor_value("temperature", t);
    device.set_sensor_value("humidity", h);
    device.send_peripheral_data_to_server();
    delay(2000);
}