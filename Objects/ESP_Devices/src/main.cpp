#include "Device.hpp"

smh::Smh_Device device("test_device_out");

void setup()
{
    if (!device.Init())
        ;

    // TODO: Add a timer on an arbitrary amount of seconds to perform and interrupt to query
    // the server on any new data for the device. This will be a Smh_Device function that creates the
    // interrupt timer. Something like create_query_timer(); that will return a timer or set one up.

    // TODO: Add a system that will be using the temperatures an dsensors, like a map or vector of pairs
    // in the json server file. Concept: <sensor/function>:<value>. For example:
    // PERIPHERAL:temperature:23.1;PERIPHERAL:pressure:10;PERIPHERAL:moisture:25
    // Firstly info type (other data can have other identifiers, tokenize first by ';', than by ':');
    // and save this to the json file as dirty data and into the peripherals maybe ?

    // TODO: Add a litening socket for incomming connection from server in case there is a CONTROLL command
    // For this device to execute soem function.
}

void loop()
{
}