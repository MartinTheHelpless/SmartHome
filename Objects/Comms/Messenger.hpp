#include <unistd.h>
#include <arpa/inet.h>

#include "Message.hpp"
#include "Json_Data.hpp"

namespace smh
{
    class Messenger
    {
    private:
        int socket_fd_ = -1;

    public:
        Messenger(int socket_fd) : socket_fd_(socket_fd) {}
        ~Messenger() {}

        int read_socket(char *buffer, int to_read = MAX_MESSAGE_SIZE) const
        {
            int read_bytes = read(socket_fd_, buffer, to_read);
            if (read_bytes > 0)
                buffer[read_bytes] = '\0';

            return read_bytes;
        }

        Message get_message()
        {
            char buffer[MAX_MESSAGE_SIZE] = {0};
            int read_bytes = read_socket(buffer);
            std::cout << "Read amount of bytes: " << read_bytes << std::endl;
            if (read_bytes > 0)
                return Message(buffer, read_bytes);

            return Message(buffer, 0);
        }

        int send_message(const Message &msg) const
        {
            uint8_t send_buffer[MAX_MESSAGE_SIZE];

            int size = msg.serialize(send_buffer);

            int sent_bytes = send(socket_fd_, send_buffer, size, 0); // Sending back info the device needs and expects after initial message

            return sent_bytes;
        }

        bool send_msg_to_ip() { return false; }
    };
}
