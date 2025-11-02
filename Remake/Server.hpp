#include <cstdint>
#include <boost/asio.hpp>

using boost::asio::ip::tcp;
namespace bas = boost::asio;
namespace basip = boost::asio::ip;

class Server
{
private:
    short unsigned int port_ = 6666;

    boost::asio::io_context io_context_;
    basip::tcp::endpoint endpoint_;
    basip::tcp::acceptor acceptor_;

public:
    Server(short unsigned int port);
    ~Server() = default;
    void Run();

    void Handle_Client(tcp::socket socket);

    void Close_Socket(boost::asio::ip::tcp::socket socket);
};