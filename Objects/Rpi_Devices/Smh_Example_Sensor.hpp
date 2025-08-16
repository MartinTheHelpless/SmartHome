#pragma once

#include "GPIO_Helper.hpp"
#include "Smh_Object.hpp"
#include "Header.hpp"
#include "Defines.hpp"

#include <linux/i2c-dev.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <iostream>
#include <cstring>

#define RASPBERRY_PI_DEVICE

#ifdef RASPBERRY_PI_DEVICE

namespace smh
{
    class Example_Sensor : private Object
    {
    private:
        std::string device_path_;
        int value_;
        int device_address_;

        int socket_fd_ = -1;
        std::string server_ip_;
        uint16_t server_port_;

    public:
        Example_Sensor(std::string device_path, int sensor_i2c_address, std::string server_ip = DEFAULT_SERVER_IP, int server_port = DEFAULT_SERVER_PORT)
            : device_path_(device_path), value_(0), device_address_(sensor_i2c_address), server_ip_(server_ip), server_port_(server_port) {}

        ~Example_Sensor()
        {
            disconnect();
        }

        bool read_sensor_value()
        {
            int file = open(device_path_.c_str(), O_RDWR);
            if (file < 0)
            {
                std::cerr << "Failed to open I2C device\n";
                return false;
            }

            if (ioctl(file, I2C_SLAVE, device_address_) < 0)
            {
                std::cerr << "Failed to acquire bus access or talk to device\n";
                close(file);
                return false;
            }

            uint8_t reg = 0xF7;
            if (write(file, &reg, 1) != 1)
            {
                std::cerr << "Failed to write register address\n";
                close(file);
                return false;
            }

            uint8_t buf[8];
            if (read(file, buf, 8) != 8)
            {
                std::cerr << "Failed to read data from sensor\n";
                close(file);
                return false;
            }

            std::cout << "Raw data bytes: ";
            for (int i = 0; i < 8; i++)
                printf("%02x ", buf[i]);
            std::cout << std::endl;

            close(file);
            return true;
        }

        bool connect_to_server()
        {
            socket_fd_ = socket(AF_INET, SOCK_STREAM, 0);
            if (socket_fd_ < 0)
            {
                std::cerr << "Socket creation failed\n";
                return false;
            }

            sockaddr_in server_addr{};
            server_addr.sin_family = AF_INET;
            server_addr.sin_port = htons(server_port_);
            if (inet_pton(AF_INET, server_ip_.c_str(), &server_addr.sin_addr) <= 0)
            {
                std::cerr << "Invalid server IP address\n";
                close(socket_fd_);
                socket_fd_ = -1;
                return false;
            }

            if (connect(socket_fd_, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
            {
                std::cerr << "Failed to connect to server\n";
                close(socket_fd_);
                socket_fd_ = -1;
                return false;
            }

            return true;
        }

        void disconnect()
        {
            if (socket_fd_ >= 0)
            {
                close(socket_fd_);
                socket_fd_ = -1;
            }
        }

        virtual void send() override
        {
            serialize(); // Fill your buffer (data_) before sending

            if (socket_fd_ >= 0 && !data_.empty())
            {
                ssize_t sent = ::send(socket_fd_, data_.data(), data_.size(), 0);
                if (sent < 0)
                {
                    std::cerr << "Send failed\n";
                }
            }
        }

        virtual void receive() override
        {
            uint8_t buffer[1024];
            ssize_t len = recv(socket_fd_, buffer, sizeof(buffer), 0);

            if (len > 0)
            {
                data_.assign(buffer, buffer + len);
                deserialize();
            }
            else if (len == 0)
            {
                std::cerr << "Server closed connection\n";
                disconnect();
            }
            else
            {
                std::cerr << "Receive error\n";
            }
        }

        virtual void serialize() override
        {
            data_.clear();
            data_.push_back(0x01); // version
            data_.push_back(UID_);
            data_.insert(data_.end(), {0xAA, 0xBB, 0xCC}); // dummy payload
        }

        virtual void deserialize() override
        {
            if (data_.size() > 2)
            {
                UID_ = data_[1];
                // Further parse as needed...
            }
        }
    };
}

#endif
