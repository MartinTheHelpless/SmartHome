#include <iostream>
#include <thread>
#include <vector>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <mutex>

#include "Defines.hpp"
#include "Message.hpp"

namespace smh
{

    class App
    {
    private:
        int server_fd = -1;
        int port;
        bool running = false;
        std::vector<std::thread> threads;
        std::mutex cout_mutex;

        void lock_and_print(std::string str)
        {
            std::lock_guard<std::mutex> lock(cout_mutex);
            std::cout << str;
        }

        void lock_and_println(std::string str)
        {
            std::lock_guard<std::mutex> lock(cout_mutex);
            std::cout << str << "\n";
        }

        void handleClient(int client_sock)
        {

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
                    }
                    else if (msg.get_header_version() != CURRENT_PROTOCOL_VERSION)
                    {
                        std::lock_guard<std::mutex> lock(cout_mutex);
                        std::cout << "Error: Protocol versions do not match\n";
                    }
                    else if (msg.is_init_msg()) // Message is first after boot or after install
                    {
                        // TODO: Read "Software MAC" and check if file for device exists, if no, generate UID? otherwise read UID and send to client
                    }
                    else
                    {
                        switch (msg.get_header_message_type())
                        {
                        case MSG_TYPE_GET:
                            // Requests data, expects response
                            break;

                        case MSG_TYPE_POST:
                            // Only info for the server, need to save incomming data to file, maybe forward to subscribers
                            break;

                        case MSG_TYPE_PING:
                            // basically just a keepalive, update last contact time on file
                            break;

                        case MSG_TYPE_SUBSCRIBE:
                            // subscribe to a devices POST type messages
                            break;

                        case MSG_TYPE_UNSUBSCRIBE:
                            // unsubscribe from a devices POST type messages
                            break;

                        case MSG_TYPE_CONTROLL:
                        {
                            // If this message type's destination is the server, something is wrong.
                            std::lock_guard<std::mutex> lock(cout_mutex);
                            std::cout << "Error: Controll message received by server - Wrong recipient\n";
                        }
                        break;

                        default:
                            break;
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
        }

    public:
        App(int port_number) : port(port_number) {}

        ~App()
        {
            running = false;
            if (server_fd != -1)
                close(server_fd);

            // Join all threads before exiting
            for (auto &t : threads)
                if (t.joinable())
                    t.join();
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

                // Spawn thread to handle client
                threads.emplace_back(&App::handleClient, this, client_sock);
            }
        }
    };
}