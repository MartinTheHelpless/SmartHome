#include "Smh_Server_App.hpp"
#include "../Utils/Defines.hpp"

int main(int argc, char const *argv[])
{

    // Running the program: ./server_app <server top directory> <server port>
    // Both arguments optional

    if (argc == 1)
    {
        smh::App app;
        app.run();
    }
    else if (argc == 2)
    {
        smh::App app(argv[1]);
        app.run();
    }
    else if (argc == 3)
    {
        smh::App app(std::stoi(argv[2]), argv[1]);
        app.run();
    }

    return 0;
}
