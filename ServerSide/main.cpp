#include "Smh_Server_App.hpp"
#include "Defines.hpp"

int main()
{
    App app(DEFAULT_SERVER_PORT);
    app.run();
    return 0;
}
