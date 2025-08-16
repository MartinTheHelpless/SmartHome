#pragma once

#include <gpiod.h>
#include <iostream>

namespace smh
{
    class GPIOHelper
    {
        struct gpiod_chip *chip_;
        struct gpiod_line *line_;
        int line_num_;

    public:
        GPIOHelper(const char *chipname, int line_num)
            : chip_(nullptr), line_(nullptr), line_num_(line_num)
        {
            chip_ = gpiod_chip_open_by_name(chipname);
            if (!chip_)
            {
                std::cerr << "Failed to open chip " << chipname << std::endl;
                return;
            }

            line_ = gpiod_chip_get_line(chip_, line_num_);
            if (!line_)
            {
                std::cerr << "Failed to get line " << line_num_ << std::endl;
                gpiod_chip_close(chip_);
                chip_ = nullptr;
            }
        }

        ~GPIOHelper()
        {
            if (line_)
                gpiod_line_release(line_);
            if (chip_)
                gpiod_chip_close(chip_);
        }

        bool setOutput(bool value)
        {
            if (!line_)
                return false;

            if (gpiod_line_request_output(line_, "smh_gpio", value ? 1 : 0) < 0)
            {
                std::cerr << "Failed to request line as output" << std::endl;
                return false;
            }

            if (gpiod_line_set_value(line_, value ? 1 : 0) < 0)
            {
                std::cerr << "Failed to set line value" << std::endl;
                return false;
            }
            return true;
        }

        bool getInput()
        {
            if (!line_)
                return -1;

            if (gpiod_line_request_input(line_, "smh_gpio") < 0)
            {
                std::cerr << "Failed to request line as input" << std::endl;
                return -1;
            }

            int val = gpiod_line_get_value(line_);
            if (val < 0)
            {
                std::cerr << "Failed to get line value" << std::endl;
                return -1;
            }
            return val;
        }
    };
}