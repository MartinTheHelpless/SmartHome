#include "Smh_Example_Sensor.hpp"
#include <memory>

int main(int argc, char const *argv[])
{
    std::unique_ptr<smh::Example_Sensor> temp_sensor =
        std::make_unique<smh::Example_Sensor>("/dev/i2c_device", 0x40, "burianovi.xyz", 9000);

    return 0;
}
