#include "ServerTCPConnection.hpp"
#include <boost/asio/ip/address.hpp>
#include <sstream>
#include <algorithm>
#include <iterator>
#include <string>
#include <vector>

/*
    Encapsulates the function of a TCP client connection

    Written by Isaac Milarsky 3/11/2022
*/

class TCPClient
{

    //Boost object representing OS io functionality and service.
    //Create sockets from this OS independent io object.
    boost::asio::io_service client_io_service;

    //The target for the client.
    boost::asio::ip::tcp::endpoint target_host_endpoint;

    //The socket and buffer for the connection.
    ServerTCPConnection chatConnection;

public:
    TCPClient(std::string,int);

    void parse_user_message(std::string);

    //Send functions
    void terminate_connection();
    void handle_login(std::vector<std::string>*);
    void handle_newuser(std::vector<std::string>*);

    void handle_send(std::string);

    void handle_logout();

    void handle_who();

    std::string wait_for_response();

};
