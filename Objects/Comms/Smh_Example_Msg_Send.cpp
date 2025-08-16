#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>

#include "Message.hpp"

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

    smh::MessageHeader header;

    header.version = 1.0;
    header.flags = SMH_IS_INIT_MSG;
    header.source_uid = 0;
    header.dest_uid = SMH_SERVER_UID;
    header.message_type = MSG_TYPE_POST;
    header.payload_size = 0;

    uint8_t buffer[sizeof(smh::MessageHeader)];

    std::memcpy(buffer, &header, sizeof(smh::MessageHeader));

    const char *payload = "test_device_out";

    header.payload_size = strlen(payload);

    smh::Message msg(header, payload);

    int size = sizeof(smh::Message) + strlen(payload) + 10;

    uint8_t buffer2[MAX_MESSAGE_SIZE];

    int sizee = msg.serialize(buffer2);

    int sock = send(buffer2, sizee);

    char buffera[20] = {0};
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

    return 0;
}
