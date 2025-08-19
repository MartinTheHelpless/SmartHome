#include "Smh_Server.hpp"
#include "../Utils/Defines.hpp"

std::pair<std::string, int> parse_args(int argc, const char *argv[])
{
    int server_port = DEFAULT_SERVER_PORT;
    std::string server_dir_path = SMH_SERVER_DIR_PATH;

    if (argc > 1)
        for (int i = 1; i < argc; i++)
            if (strncmp(argv[i], "-p", 2) == 0 && i + 1 <= argc)
                server_port = std::stoi(argv[++i]);
            else if (strncmp(argv[i], "-d", 2) == 0 && i + 1 <= argc)
                server_dir_path = argv[++i];

    return std::pair<std::string, int>(server_dir_path, server_port);
}

// Running the program: ./server_app -d <server top directory> -p <server port>
// Both arguments optional

int main(int argc, char const *argv[])
{
    auto dir_port_pair = parse_args(argc, argv);

    smh::Server srv(dir_port_pair.second, dir_port_pair.first);
    srv.run();

    return 0;
}
