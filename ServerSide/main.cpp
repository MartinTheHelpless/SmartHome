#include "Smh_Server.hpp"
#include "../Utils/Defines.hpp"

int main(int argc, char const *argv[])
{

    // Running the program: ./server_app <server top directory> <server port>
    // Both arguments optional

    if (argc == 1)
    {
        smh::Server srv;
        srv.run();
    }
    else if (argc == 2)
    {
        smh::Server srv(argv[1]);
        srv.run();
    }
    else if (argc == 3)
    {
        smh::Server srv(std::stoi(argv[2]), argv[1]);
        srv.run();
    }

    return 0;
}
