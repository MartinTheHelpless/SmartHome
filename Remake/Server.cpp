#include "Server.hpp"
#include <iostream>

/*
 * uint32_t port - Port that the server will run on
 * acceptor_ variable is initialized in the constructor with a tcp v4 and an ip address
 * io_context contains all this context
 */
Server::Server(short unsigned int port) : port_(port), acceptor_(io_context_, basip::tcp::endpoint(basip::tcp::v4(), port))
{
}

void Server::Handle_Client(tcp::socket socket)
{

    Close_Socket(std::move(socket));
}

void Server::Close_Socket(boost::asio::ip::tcp::socket socket)
{
    socket.shutdown(basip::tcp::socket::shutdown_both);
    socket.close();
    std::cout << "Socket has been closed" << std::endl;
}

void Server::Run()
{
    while (true)
    {
        tcp::socket socket(io_context_);
        acceptor_.accept(socket);
        if (socket.is_open())
        {
            std::cout << "Connection received" << std::endl;
            std::thread client_thread(&Server::Handle_Client, this, std::move(socket));
            client_thread.detach();
        }
    }
}