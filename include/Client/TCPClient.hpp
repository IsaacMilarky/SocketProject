#include "../ServerTCPConnection.hpp"
#include <boost/asio/ip/address.hpp>
#include <sstream>
#include <algorithm>
#include <iterator>
#include <string>
#include <vector>


class TCPClient
{

    boost::asio::io_service client_io_service;

    boost::asio::ip::tcp::endpoint target_host_endpoint;

    ServerTCPConnection chatConnection;

public:
    TCPClient(std::string,int);

    void parse_user_message(std::string);

    //Send functions
    void terminate_connection();
    void handle_login(std::vector<std::string>*);
    void handle_newuser(std::vector<std::string>*);

    void handle_send(std::string);

};
