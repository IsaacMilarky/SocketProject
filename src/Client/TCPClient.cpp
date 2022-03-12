#include "../../include/Client/TCPClient.hpp"


TCPClient::TCPClient(std::string addressString, int port) : client_io_service(), chatConnection(client_io_service)
{
    target_host_endpoint = boost::asio::ip::tcp::endpoint(boost::asio::ip::address::from_string(addressString),port);

    //Start a session with the server.
    chatConnection.socket.connect(target_host_endpoint);
}