#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>

#include "Message.hpp"
#include "../ServerSide/Json_Data.hpp"
#include "../Utils/Helper_Functions.hpp"

int send(uint8_t *buffer, int size)
{
    const char *server_ip = "127.0.0.1";
    const int server_port = 9000;

    // Create socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        perror("Socket creation failed");
        return 1;
    }

    // Server address
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0)
    {
        perror("Invalid address");
        close(sock);
        return 1;
    }

    // Connect to server
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Connection failed");
        close(sock);
        return 1;
    }

    std::cout << "Connected to server\n";

    // Send buffer
    ssize_t sent_bytes = send(sock, buffer, size, 0);
    if (sent_bytes < 0)
        perror("Send failed");
    else
        std::cout << "Sent " << sent_bytes << " bytes to server\n";

    // Close socket
    std::cout << "Connection closed\n";

    return sock;
}

int main(int argc, char const *argv[])
{
    std::string message = "PERIPHERAL_S:temperature:15.5;PERIPHERAL_S:lighting:10.4;PERIPHERAL_S:humidity:31.7";

    smh::MessageHeader header = smh::create_header(3, SMH_SERVER_UID, MSG_POST, SMH_FLAG_NONE, message.size());

    std::vector<uint8_t> payload(message.size(), 0);

    std::copy(message.begin(), message.end(), payload.data());

    smh::Message msg(header, payload);

    if (!msg.is_valid())
    {
        std::cout << "invalid message" << std::endl;
        return 1;
    }

    std::vector<uint8_t> buffer = msg.serialize();

    int sock = send(buffer.data(), buffer.size());

    char buffera[MAX_MESSAGE_SIZE] = {0};
    int read_bytes = read(sock, buffera, sizeof(buffera) - 1);
    if (read_bytes > 0)
    {
        buffera[read_bytes] = '\0';
        std::cout << "Received " << read_bytes << " bytes from client\n";
    }
    close(sock);
    smh::Message msgs(buffera, read_bytes);

    std::cout << "Device New UID: " << msgs.get_header_dest_uid() << std::endl;
    std::cout << "Received from UID: " << msgs.get_header_source_uid() << std::endl;

    std::vector<std::string> data_received = smh::split_string(msgs.get_payload_str(), ';');

    for (auto line : data_received)
        std::cout << line << std::endl;

    return 0;
}
