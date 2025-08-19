#include "Device.hpp"

smh::Smh_Device device("test_device_out");

void setup()
{
    if (!device.Init())
        ;
}

void loop()
{
}
/*

#include <ESP8266WiFi.h>

const char *ssid = "RETIA-Guest";
const char *password = "53002341";

const char *server_ip = "10.9.170.225"; // your PC IP in hotspot network
const uint16_t server_port = 9000;

WiFiClient client;

void setup()
{
    Serial.begin(115200);
    WiFi.begin(ssid, password);

    Serial.print("Connecting to Wi-Fi");
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWi-Fi connected!");
    Serial.print("ESP IP: ");
    Serial.println(WiFi.localIP());

    Serial.println("Connecting to server...");
    if (client.connect(server_ip, server_port))
    {
        Serial.println("Connected to server!");
        client.print("Hello from ESP!\n"); // send data
        client.flush();
        Serial.println("Data sent!");
    }
    else
    {
        Serial.println("Failed to connect to server");
    }
}

void loop()
{
    // nothing
}
*/
