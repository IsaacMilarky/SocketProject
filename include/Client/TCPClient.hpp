#include "../ServerTCPConnection.hpp"
#include <boost/asio/ip/address.hpp>
#include <sstream>
#include <algorithm>
#include <iterator>
#include <string>


class TCPClient
{

    boost::asio::io_service client_io_service;

    boost::asio::ip::tcp::endpoint target_host_endpoint;

    ServerTCPConnection chatConnection;

public:
    TCPClient(std::string,int);

};
