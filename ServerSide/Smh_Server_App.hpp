#pragma once

#include <iostream>
#include <thread>
#include <vector>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <future>
#include <mutex>
#include <chrono>

#include "../Utils/Defines.hpp"
#include "../Objects/Comms/Message.hpp"
#include "Smh_File_Helper.hpp"
#include "../Objects/Comms/Json_Data.hpp"

namespace smh
{
    class App
    {
    private:
        int server_fd = -1;
        int port;
        bool running = false;
        std::vector<std::future<void>> futures;
        std::mutex cout_mutex;

        Smh_File_Helper file_helper;

        void handleClient(int client_sock, const struct sockaddr_in &client_addr)
        {

            std::cout << "Socket Number:" << client_sock << std::endl;

            char buffer[MAX_MESSAGE_SIZE] = {0};
            int read_bytes = read(client_sock, buffer, sizeof(buffer) - 1);
            if (read_bytes > 0)
            {
                buffer[read_bytes] = '\0';
                {
                    std::lock_guard<std::mutex> lock(cout_mutex);
                    std::cout << "Received " << read_bytes << " bytes from client\n";
                }
                Message msg(buffer, read_bytes);

                if (msg.deserialize_header())
                {

                    if (msg.get_header_dest_uid() != SMH_SERVER_UID)
                    {
                        // Check who is recipient by UID, forward message to recipient
                        std::lock_guard<std::mutex> lock(cout_mutex);
                        std::cout << "This message is not for the server" << std::endl;
                    }
                    else if (msg.get_header_version() != CURRENT_PROTOCOL_VERSION)
                    {
                        std::lock_guard<std::mutex> lock(cout_mutex);
                        std::cout << "Error: Protocol versions do not match\n";
                    }
                    else if (msg.is_init_msg()) // Message is first after boot or after install
                    {
                        std::string device_sw_mac = msg.get_payload_str(); // In an init message, the payload contains only the software devoce "MAC"
                        int client_device_uid = msg.get_header_source_uid();
                        Json_Data client_device_json;

                        if (!file_helper.device_file_exists(device_sw_mac))
                        {
                            if (strstr(device_sw_mac.c_str(), "out")) // outside device
                                file_helper.create_outside_device_file(device_sw_mac);
                            else if (strstr(device_sw_mac.c_str(), "ins")) // inside device
                                file_helper.create_inside_device_file(device_sw_mac);
                            else
                            {
                                std::lock_guard<std::mutex> lock(cout_mutex);
                                std::cout << "An incorrect SW MAC address detected: " << device_sw_mac << "\n";
                                close(client_sock);
                                return;
                            }

                            std::string device_file_path = file_helper.get_full_path_to_device(device_sw_mac);

                            if (device_file_path == "")
                            {
                                std::lock_guard<std::mutex> lock(cout_mutex);
                                std::cout << "An error has occured while searching for a device json file:\n\t" << device_file_path << "\n";
                                close(client_sock);
                                return;
                            }

                            {
                                std::lock_guard<std::mutex> lock(cout_mutex);
                                std::cout << "Json file found:\t\n"
                                          << device_file_path << std::endl;
                            }

                            client_device_uid = 20; // TODO: Make a system for UIDs so that they are actually generated and saved

                            client_device_json = Json_Data(device_file_path);
                            client_device_json.init(client_device_uid, device_sw_mac, inet_ntoa(client_addr.sin_addr));

                            auto now = std::chrono::system_clock::now();
                            uint64_t now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
                            client_device_json.set_last_contact(now_ms);
                            client_device_json.save();

                            // TODO: Add a way to contact all the other devices to tell them that a new device has appeared.
                            // So they could subsribe to their data
                        }
                        else
                        {
                            std::string device_file_path = file_helper.get_full_path_to_device(device_sw_mac);
                            client_device_json = Json_Data(device_file_path);
                        }

                        auto now = std::chrono::system_clock::now();
                        uint64_t now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
                        client_device_json.set_last_contact(now_ms);
                        client_device_json.save();

                        MessageHeader header;
                        header.version = 1.0;
                        header.flags = 0;
                        header.source_uid = SMH_SERVER_UID;
                        header.dest_uid = client_device_uid;
                        header.message_type = MSG_TYPE_POST;
                        header.payload_size = 0;

                        Message ret_msg = Message(header);

                        uint8_t send_buffer[MAX_MESSAGE_SIZE];

                        int size = ret_msg.serialize(send_buffer);

                        int sent_bytes = send(client_sock, send_buffer, size, 0); // Sending back info the device needs and expects after initial message
                        std::cout << sent_bytes << " Bytes sent to the client" << std::endl;
                    }

                    else
                    {
                        switch (msg.get_message_type())
                        {
                        case MSG_TYPE_GET:
                        {
                            // Requests data, expects response
                            break;
                        }

                        case MSG_TYPE_POST:
                        {
                            // Only info for the server, need to save incomming data to file, maybe forward to subscribers
                            break;
                        }

                        case MSG_TYPE_PING:
                        {
                            // basically just a keepalive, update last contact time on file
                            break;
                        }

                        case MSG_TYPE_SUBSCRIBE:
                        {
                            // subscribe to a devices POST type messages
                            break;
                        }

                        case MSG_TYPE_UNSUBSCRIBE:
                        {
                            // unsubscribe from a devices POST type messages
                            break;
                        }

                        case MSG_TYPE_CONTROLL: // Case done
                        {
                            // If this message type's destination is the server, something is wrong.
                            std::lock_guard<std::mutex> lock(cout_mutex);
                            std::cout << "Error: Controll message received by server - Wrong recipient\n";
                            break;
                        }

                        default:
                        {
                            std::lock_guard<std::mutex> lock(cout_mutex);
                            std::cout << "Unknown message type" << std::endl;
                            break;
                        }
                        }
                    }
                }
                else
                {
                    std::lock_guard<std::mutex> lock(cout_mutex);
                    std::cout << "Error: Header deserialization error\n";
                }
            }
            else
            {
                std::lock_guard<std::mutex> lock(cout_mutex);
                std::cout << "Client disconnected or read error\n";
            }

            close(client_sock);
            std::cout << "A client disconnected\n";
        }

    public:
        App(int port_number, std::string srv_top_dir = SMH_SERVER_DIR_PATH)
            : port(port_number), file_helper(false, srv_top_dir) {}

        App(std::string srv_top_dir = SMH_SERVER_DIR_PATH, int port_number = DEFAULT_SERVER_PORT)
            : port(port_number), file_helper(false, srv_top_dir) {}

        ~App()
        {
            running = false;
            if (server_fd != -1)
                close(server_fd);

            // Wait for all futures
            for (auto it = futures.begin(); it != futures.end();)
            {
                if (it->valid())
                    it->get();
                it = futures.erase(it);
            }
        }

        void run()
        {
            struct sockaddr_in address;
            int opt = 1;
            socklen_t addrlen = sizeof(address);

            if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
            {
                perror("Socket failed");
                return;
            }

            if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)))
            {
                perror("setsockopt");
                return;
            }

            address.sin_family = AF_INET;
            address.sin_addr.s_addr = INADDR_ANY;
            address.sin_port = htons(port);

            if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
            {
                perror("Bind failed");
                return;
            }

            if (listen(server_fd, 5) < 0)
            {
                perror("Listen failed");
                return;
            }

            running = true;

            std::cout << "Server listening on port " << port << "\n";

            while (running)
            {
                int client_sock = accept(server_fd, (struct sockaddr *)&address, &addrlen);
                if (client_sock < 0)
                {
                    perror("Accept failed");
                    continue;
                }

                {
                    std::lock_guard<std::mutex> lock(cout_mutex);
                    std::cout << "Client connected: "
                              << inet_ntoa(address.sin_addr) << ":" << ntohs(address.sin_port) << "\n";
                }

                auto fut = std::async(std::launch::async,
                                      [this, client_sock, address]()
                                      { handleClient(client_sock, address); });
                futures.push_back(std::move(fut));

                for (auto it = futures.begin(); it != futures.end();)
                    if (it->wait_for(std::chrono::seconds(0)) == std::future_status::ready)
                    {
                        it->get();
                        it = futures.erase(it);
                    }
                    else
                        ++it;

                std::cout << "Clients connected: " << futures.size() << "\n";
            }
        }
    };
}