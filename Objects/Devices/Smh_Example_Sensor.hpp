#include "Smh_Object.hpp"
#include "Header.hpp"
#include "GPIO_Helper.hpp"

#include <linux/i2c-dev.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
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

        SSL_CTX *ctx_ = nullptr;
        SSL *ssl_ = nullptr;
        int socket_fd_ = -1;
        std::string server_ip_ = "192.168.1.10"; // Replace as needed
        uint16_t server_port_ = 9000;

    public:
        Example_Sensor(std::string device_path, int sensor_i2c_address, std::string server_ip, int server_port)
            : device_path_(device_path), value_(0), device_address_(sensor_i2c_address), server_ip_(server_ip), server_port_(server_port) {}

        ~Example_Sensor() {}

        bool read_sensor_value()
        {
            int file = open(device_path_.c_str(), O_RDWR);
            if (file < 0)
            {
                std::cerr << "Failed to open I2C device\n";
                return false;
            }

            // I2C address of BME280

            if (ioctl(file, I2C_SLAVE, device_address_) < 0)
            {
                std::cerr << "Failed to acquire bus access or talk to device\n";
                close(file);
                return false;
            }

            // For BME280 you need to write register address then read data
            uint8_t reg = 0xF7; // Example: pressure data start register
            if (write(file, &reg, 1) != 1)
            {
                std::cerr << "Failed to write register address\n";
                close(file);
                return false;
            }

            // Read 8 bytes (pressure, temperature, humidity)
            uint8_t buf[8];
            if (read(file, buf, 8) != 8)
            {
                std::cerr << "Failed to read data from sensor\n";
                close(file);
                return false;
            }

            // Interpret data here based on datasheet...

            std::cout << "Raw data bytes: ";
            for (int i = 0; i < 8; i++)
            {
                printf("%02x ", buf[i]);
            }
            std::cout << std::endl;

            close(file);
            return true;
        }

        bool connect_to_server()
        {
            SSL_library_init();
            OpenSSL_add_all_algorithms();
            SSL_load_error_strings();

            ctx_ = SSL_CTX_new(TLS_client_method());
            if (!ctx_)
                return false;

            socket_fd_ = socket(AF_INET, SOCK_STREAM, 0);
            if (socket_fd_ < 0)
                return false;

            sockaddr_in server_addr{};
            server_addr.sin_family = AF_INET;
            server_addr.sin_port = htons(server_port_);
            inet_pton(AF_INET, server_ip_.c_str(), &server_addr.sin_addr);

            if (connect(socket_fd_, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
                return false;

            ssl_ = SSL_new(ctx_);
            SSL_set_fd(ssl_, socket_fd_);
            if (SSL_connect(ssl_) <= 0)
                return false;

            return true;
        }

        void disconnect()
        {
            if (ssl_)
            {
                SSL_shutdown(ssl_);
                SSL_free(ssl_);
            }
            if (socket_fd_ >= 0)
                close(socket_fd_);
            if (ctx_)
                SSL_CTX_free(ctx_);
        }

        virtual void send() override
        {
            serialize(); // Fill your buffer (data_) before sending

            if (ssl_ && !data_.empty())
            {
                SSL_write(ssl_, data_.data(), data_.size());
            }
        }

        virtual void receive() override
        {
            uint8_t buffer[1024];
            int len = SSL_read(ssl_, buffer, sizeof(buffer));

            if (len > 0)
            {
                data_.assign(buffer, buffer + len);
                deserialize();
            }
        }

        virtual void serialize() override
        {
            // For example, construct a simple packet
            data_.clear();
            data_.push_back(0x01); // version
            data_.push_back(UID_);
            data_.insert(data_.end(), {0xAA, 0xBB, 0xCC}); // dummy payload
        }

        virtual void deserialize() override
        {
            if (data_.size() > 2)
            {
                UID_ = data_[1]; // Assuming second byte is UID
                // Further parse as needed...
            }
        }
    };
}

#endif