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
#include <sstream>

#include "../Utils/Defines.hpp"
#include "../Comms/Message.hpp"
#include "Smh_File_Helper.hpp"
#include "Json_Data.hpp"
#include "../Comms/Messenger.hpp"
#include "../Utils/Helper_Functions.hpp"

namespace smh
{
    class Server
    {
    private:
        int server_fd_ = -1;
        int port_;
        bool running_ = false;

        Json_Server server_data;
        Json_Website website_data;
        Json_Device raw_website_data;

        std::string website_json_name = "webs_device_ins";

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
            }
            else if (msg.get_header_dest_uid() != SMH_SERVER_UID)
            {
                auto message_type = msg.get_message_type();

                if (message_type == MSG_CONTROL)
                    send_msg_to_device(msg.get_header_dest_uid(), courier, msg);
                else if (message_type != MSG_POST)
                {
                    std::lock_guard<std::mutex> lock(cout_mutex);
                    std::cout << "Message with a device UID is not POST or CONTROLL type. A Mistake perchance ?\n";
                }
                else
                {
                    if (!handle_msg_forward(msg))
                    {
                        std::lock_guard<std::mutex> lock(cout_mutex);
                        std::cout << "Could not forward message. Recipient not found\n";
                    }
                }
            }
            else if (msg.is_init_msg()) // Message sent always after device boot
            {
                char ip_str[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &(client_addr.sin_addr), ip_str, INET_ADDRSTRLEN);

                if (!handle_init_message(courier, msg.get_payload_str(), ip_str)) // In an init message, the payload contains only the software device "MAC"
                {
                    std::lock_guard<std::mutex> lock(cout_mutex);
                    std::cout << "Error while processing an init message\n";
                }
            }
            else // Normal message adressed to server4
            {
                char ip_str[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &(client_addr.sin_addr), ip_str, INET_ADDRSTRLEN);
                handle_message_by_type(msg, courier, ip_str);
            }

            close(client_sock);
        }

        bool handle_msg_forward(const Message &msg)
        {
            std::optional<std::string> device_name = server_data.try_get_device_by_uid(msg.get_header_dest_uid());

            if (device_name == std::nullopt)
                return false;

            Json_Device device_data;
            {
                std::lock_guard<std::mutex> lock(srv_json_mutex);
                file_helper.get_device_json(device_name.value(), device_data);
            }

            device_data.add_dirty_data(msg.get_payload_str());

            return true;
        }

        bool handle_init_message(const Messenger &courier, const std::string &device_sw_mac, const std::string &last_ip)
        {
            Json_Device client_device_json;
            {
                std::lock_guard<std::mutex> lock(srv_json_mutex);

                if (!file_helper.device_file_exists(device_sw_mac))
                {
                    if (!file_helper.create_and_init_device_file(device_sw_mac, client_device_json))
                        return false;

                    client_device_json.set_last_ip(last_ip);
                    int client_device_uid;

                    client_device_uid = server_data.assign_new_uid(device_sw_mac);

                    client_device_json.set_uid(client_device_uid);
                    client_device_json.add_subscriber(0);

                    // OPTIONAL: Add a way to contact all the other devices to tell them that a new device has appeared.
                    // So they could subsribe to their data
                }
                else
                {
                    std::string device_file_path = file_helper.get_full_path_to_device_file(device_sw_mac);
                    client_device_json = Json_Device(device_file_path);
                }
            }

            MessageHeader header = create_header(SMH_SERVER_UID, client_device_json.get_uid(), MSG_POST);
            Message ret_msg = Message(header);

            int n = courier.send_message(ret_msg);
            if (n <= 0)
            {
                std::lock_guard<std::mutex> lock(cout_mutex);
                std::cerr << "send_message failed\n";
                return false;
            }

            return true;
        }

        bool handle_msg_get(Messenger &courier, Json_Device &device_data)
        {
            MessageHeader header;

            std::string payload;
            if (device_data.get_dirty_data_size() > 0)
            {
                std::vector<std::string> dirty_data = device_data.get_dirty_data();
                std::vector<uint8_t> payload_bytes;

                for (size_t i = 0; i < dirty_data.size(); ++i)
                {
                    if (i)
                        payload_bytes.push_back(';'); // separator
                    const std::string &s = dirty_data[i];
                    payload_bytes.insert(payload_bytes.end(), s.begin(), s.end());
                }

                MessageHeader header = create_header(SMH_SERVER_UID, device_data.get_uid(), MSG_POST);
                header.payload_size = static_cast<uint16_t>(payload_bytes.size());

                courier.send_message(Message(header, payload_bytes));

                device_data.clear_dirty_data();
            }
            else
            {
                header = create_header(SMH_SERVER_UID, device_data.get_uid(), MSG_POST);
                courier.send_message(Message(header));
            }
            return true;
        }

        void handle_post_data(std::string raw_messgae, Json_Device &json_dev)
        {
            auto peripherals = json_dev.get_peripherals();

            auto data = split_string(raw_messgae.c_str(), ';');

            bool change_in_peripherals = false;
            for (auto message : data)
            {
                std::vector<std::string> tokens = split_string(message, ':');

                if ((tokens[0] == "PERIPHERAL_S" || tokens[0] == "PERIPHERAL_C") && tokens.size() == 3)
                {
                    peripherals[tokens[1]] = tokens[2];
                    change_in_peripherals = true;
                }
                if (change_in_peripherals)
                    json_dev.set_peripherals(peripherals);
            }
        }

        bool handle_msg_post(const Message &msg, Json_Device &device_data)
        {

            // TODO: Add a detection and handling for message subtypes, such as PERIPHERAL, STATE, ERROR, etc.
            std::string device_name = device_data.get_name();

            handle_post_data(msg.get_payload_str(), device_data);

            std::vector<int> subs = device_data.get_subscribers();
            std::string payload = msg.get_payload_str();

            if (msg.get_payload_size() > 0)
                for (int sub_uid : subs)
                {
                    std::optional<std::string> device_uid = server_data.try_get_device_by_uid(sub_uid);

                    if (device_uid == std::nullopt)
                        continue;

                    Json_Device sub_device;
                    {
                        std::lock_guard<std::mutex> lock(srv_json_mutex);
                        file_helper.get_device_json(device_uid.value(), sub_device);
                    }

                    auto messages = split_string(payload, ';');

                    for (auto message : messages)
                        sub_device.add_dirty_data(device_name + ":" + message);
                }

            website_data.parse_raw_data(raw_website_data);

            return true;
        }

        void subscribe_to_device(int uid_subscriber, std::string device_to_sub_to)
        {
            Json_Device json_data;
            std::lock_guard<std::mutex> lock(srv_json_mutex);
            file_helper.get_device_json(device_to_sub_to, json_data);

            if (json_data.get_uid() == uid_subscriber)
                return;

            json_data.add_subscriber(uid_subscriber);
        }

        void unsubscribe_from_device(int uid_unsubscriber, std::string device_to_unsub_from)
        {
            Json_Device json_data;
            {
                std::lock_guard<std::mutex> lock(srv_json_mutex);
                file_helper.get_device_json(device_to_unsub_from, json_data);
            }

            json_data.remove_subscriber(uid_unsubscriber);
        }

        bool handle_message_by_type(const Message &msg, Messenger &courier, const std::string &last_ip)
        {
            Json_Device source_device_data;
            auto source_device = server_data.try_get_device_by_uid(msg.get_header_source_uid());

            if (source_device == std::nullopt)
                return false;

            {
                std::lock_guard<std::mutex> lock(srv_json_mutex);

                if (!file_helper.device_file_exists(source_device.value()) && msg.get_message_type() != MSG_GET)
                    return false;
                else if (!file_helper.device_file_exists(source_device.value()) && msg.get_message_type() == MSG_GET)
                {
                    MessageHeader header = create_header(SMH_SERVER_UID, msg.get_header_source_uid(), MSG_POST, SMH_FLAG_ERROR);

                    courier.send_message(Message(header));

                    return false;
                }

                file_helper.get_device_json(source_device.value(), source_device_data);
                source_device_data.set_last_ip(last_ip);
            }

            switch (msg.get_message_type())
            {
            case MSG_GET: // Requests dirty data from server, expects response
                handle_msg_get(courier, source_device_data);
                break;

            case MSG_POST: // Only info for the server, need to save incomming data to file, maybe forward to subscribers
                handle_msg_post(msg, source_device_data);
                break;

            case MSG_PING: // basically just a keepalive, update last contact time on file
                source_device_data.set_last_contact_now();
                break;

            case MSG_SUBSCRIBE: // subscribe to a devices POST type messages
                subscribe_to_device(msg.get_header_source_uid(), msg.get_payload_str());
                break;

            case MSG_UNSUBSCRIBE: // unsubscribe from a devices POST type messages
                unsubscribe_from_device(msg.get_header_source_uid(), msg.get_payload_str());
                break;

            case MSG_SUBSCRIBE_MULTI: // subscribe to a devices POST type messages
            {
                std::string devices_str = msg.get_payload_str();

                std::string device;
                std::vector<std::string> devices;
                std::stringstream ss(devices_str);

                while (std::getline(ss, device, ';'))
                    if (!device.empty())
                        devices.push_back(device);

                int subscriber_uid = msg.get_header_source_uid();

                for (auto device : devices)
                    subscribe_to_device(subscriber_uid, device);

                break;
            }

            case MSG_UNSUBSCRIBE_MULTI: // unsubscribe from a devices POST type messages
            {
                std::string devices_str = msg.get_payload_str();

                std::string device;
                std::vector<std::string> devices;
                std::stringstream ss(devices_str);

                while (std::getline(ss, device, ';'))
                    if (!device.empty())
                        devices.push_back(device);

                int subscriber_uid = msg.get_header_source_uid();

                for (auto device : devices)
                    unsubscribe_from_device(subscriber_uid, device);
                break;
            }

            case MSG_CONTROL: // If this message type's destination is the server, something is wrong.
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

        bool send_msg_to_device(int dest_uid, Messenger &courier, const Message &msg)
        {
            std::optional<std::string> device_name = server_data.try_get_device_by_uid(dest_uid);

            if (device_name == std::nullopt)
                return false;

            Json_Device dest_device_data;
            {
                std::lock_guard<std::mutex> lock(srv_json_mutex);
                file_helper.get_device_json(device_name.value(), dest_device_data);
            }

            std::string dest_ip = dest_device_data.get_last_ip();
            if (dest_ip == "0.0.0.0")
                return false;

            return courier.send_msg_to_ip(msg, dest_ip);
        }

    public:
        Server(std::string srv_top_dir = SMH_SERVER_DIR_PATH, int port_number = DEFAULT_SERVER_PORT)
            : port_(port_number), file_helper(false, srv_top_dir)
        {
            std::lock_guard<std::mutex> lock(srv_json_mutex);
            if (!file_helper.server_json_exists() && !file_helper.create_and_init_server_file(server_data))
                std::cout << "Error creating server json" << std::endl;
            else
                server_data = file_helper.get_server_json();

            if (!file_helper.device_file_exists(website_json_name))
                file_helper.create_and_init_device_file(website_json_name, raw_website_data);
            else
                file_helper.get_device_json(website_json_name, raw_website_data);

            if (!file_helper.website_json_exists())
                file_helper.create_and_init_website_file(website_data);
            else
                website_data = file_helper.get_website_json();
        }

        ~Server()
        {
            running_ = false;
            if (server_fd_ != -1)
            {
                shutdown(server_fd_, SHUT_RDWR);
                close(server_fd_);
            }

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

            if ((server_fd_ = socket(AF_INET, SOCK_STREAM, 0)) < 0)
            {
                perror("Socket failed");
                return;
            }

            if (setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)))
            {
                perror("setsockopt");
                return;
            }

            address.sin_family = AF_INET;
            address.sin_addr.s_addr = INADDR_ANY;
            address.sin_port = htons(port_);

            if (bind(server_fd_, (struct sockaddr *)&address, sizeof(address)) < 0)
            {
                perror("Bind failed");
                return;
            }

            if (listen(server_fd_, 5) < 0)
            {
                perror("Listen failed");
                return;
            }

            running_ = true;
            {
                std::lock_guard<std::mutex> lock(cout_mutex);
                std::cout << "Server listening on port " << port_ << "\n";
            }

            while (running_)
            {
                int client_sock = accept(server_fd_, (struct sockaddr *)&address, &addrlen);
                if (client_sock < 0)
                {
                    perror("Accept failed");
                    continue;
                }

                char ip_str[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &(address.sin_addr), ip_str, INET_ADDRSTRLEN);
                {
                    std::lock_guard<std::mutex> lock(cout_mutex);
                    std::cout << "Client connected: " << ip_str << ":" << ntohs(address.sin_port) << "\n";
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
                {
                    std::lock_guard<std::mutex> lock(cout_mutex);
                    std::cout << "Clients connected: " << futures.size() << "\n";
                }
            }
        }
    };
}