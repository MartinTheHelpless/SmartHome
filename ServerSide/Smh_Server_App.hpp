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
#include "../Objects/Comms/Messenger.hpp"

namespace smh
{
    class App
    {
    private:
        int server_fd = -1;
        int port;
        bool running = false;

        Json_Server server_data;

        std::vector<std::future<void>> futures;
        std::mutex cout_mutex, srv_json_mutex;

        Smh_File_Helper file_helper;

        void handleClient(int client_sock, const struct sockaddr_in &client_addr)
        {
            smh::Messenger courier(client_sock);

            Message msg = courier.get_message();

            if (!msg.is_valid())
            {
                std::lock_guard<std::mutex> lock(cout_mutex);
                std::cout << "Client disconnected or read error\n";
                close(client_sock);
                return;
            }

            if (msg.get_header_dest_uid() != SMH_SERVER_UID)
            {
                // Check who is recipient by UID
                // save as dirty data to its file if there is one (next time the device asks if there is anything new for it, it will get the data)
            }
            else if (msg.is_init_msg()) // Message sent always after device boot
            {
                std::string device_sw_mac = msg.get_payload_str(); // In an init message, the payload contains only the software devoce "MAC"
                if (!handle_init_message(courier, device_sw_mac, inet_ntoa(client_addr.sin_addr)))
                {
                    std::lock_guard<std::mutex> lock(cout_mutex);
                    std::cout << "Error: Could not handle init message\n";
                    close(client_sock);
                    return;
                }
            }
            else
            {
                handle_message_by_type(msg);
            }

            close(client_sock);
        }

        bool handle_init_message(const Messenger &courier, const std::string &device_sw_mac, const std::string &last_ip)
        {
            {
                std::lock_guard<std::mutex> lock(cout_mutex);
                std::cout << "Init message received" << std::endl;
            }

            Json_Device client_device_json;

            if (!file_helper.device_file_exists(device_sw_mac))
            {
                if (!file_helper.create_and_init_device_file(device_sw_mac, client_device_json))
                    return false;

                client_device_json.set_last_ip(last_ip);
                int client_device_uid;
                {
                    std::lock_guard<std::mutex> lock(srv_json_mutex);
                    client_device_uid = server_data.assign_new_uid(device_sw_mac);
                }
                client_device_json.set_uid(client_device_uid);
                client_device_json.save();

                // OPTIONAL: Add a way to contact all the other devices to tell them that a new device has appeared.
                // So they could subsribe to their data
            }
            else
            {
                std::string device_file_path = file_helper.get_full_path_to_device_file(device_sw_mac);
                client_device_json = Json_Device(device_file_path);
                client_device_json.set_last_contact_now();
            }

            MessageHeader header;
            header.version = CURRENT_PROTOCOL_VERSION;
            header.flags = SMH_FLAG_NONE;
            header.source_uid = SMH_SERVER_UID;
            header.dest_uid = client_device_json.get_uid();
            header.message_type = MSG_TYPE_POST;
            header.payload_size = 0;

            Message ret_msg = Message(header);

            courier.send_message(ret_msg);

            return true;
        }

        bool handle_message_by_type(const Message &msg)
        {
            switch (msg.get_message_type())
            {
            case MSG_TYPE_GET: // Requested data, expects response
                break;

            case MSG_TYPE_POST: // Only info for the server, need to save incomming data to file, maybe forward to subscribers
                break;

            case MSG_TYPE_PING: // basically just a keepalive, update last contact time on file
            {
                Json_Device json_data;
                file_helper.get_device_json(server_data.get_device_by_uid(msg.get_header_source_uid()), json_data);
                json_data.set_last_contact_now();
                break;
            }

            case MSG_TYPE_SUBSCRIBE: // subscribe to a devices POST type messages
            {
                std::string device_to_sub_to = msg.get_payload_str();

                Json_Device json_data;
                file_helper.get_device_json(device_to_sub_to, json_data);

                json_data.add_subscriber(msg.get_header_source_uid());
                break;
            }

            case MSG_TYPE_UNSUBSCRIBE: // unsubscribe from a devices POST type messages
            {
                std::string device_to_sub_to = msg.get_payload_str();

                Json_Device json_data;
                file_helper.get_device_json(device_to_sub_to, json_data);

                json_data.remove_subscriber(msg.get_header_source_uid());
                break;
            }

            case MSG_TYPE_CONTROLL: // If this message type's destination is the server, something is wrong.
            {
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
            return true;
        }

    public:
        App(int port_number, std::string srv_top_dir = SMH_SERVER_DIR_PATH)
            : port(port_number), file_helper(false, srv_top_dir)
        {
            std::lock_guard<std::mutex> lock(srv_json_mutex);
            if (!file_helper.server_json_exists())
            {
                if (!file_helper.create_and_init_server_file(server_data))
                    std::cout << "Error creating server json" << std::endl;
            }
            else
                server_data = file_helper.get_server_json();
        }

        App(std::string srv_top_dir = SMH_SERVER_DIR_PATH, int port_number = DEFAULT_SERVER_PORT)
            : port(port_number), file_helper(false, srv_top_dir)
        {
            std::lock_guard<std::mutex> lock(srv_json_mutex);
            if (!file_helper.server_json_exists())
            {
                if (!file_helper.create_and_init_server_file(server_data))
                    std::cout << "Error creating server json" << std::endl;
            }
            else
                server_data = file_helper.get_server_json();
        }

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