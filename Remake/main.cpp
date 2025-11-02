#include "Server.hpp"

int main(int, char const *[])
{
    Server srv(6666);

    srv.Run();

    return 0;
}
