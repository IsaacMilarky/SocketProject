#include "ServerTCPConnection.hpp"
#include <boost/asio/ip/address.hpp>
#include <boost/asio/posix/stream_descriptor.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/system/detail/error_code.hpp>
#include <boost/asio.hpp>
#include <boost/array.hpp>
#include <boost/bind/bind.hpp>
#include <sstream>
#include <algorithm>
#include <iterator>
#include <string>
#include <vector>

/*
    Encapsulates the function of an async TCP client connection

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


    //Objects for concurrent user std input
    //First two act as file descriptor objects for stdin
    boost::asio::posix::stream_descriptor userInput_;
    boost::asio::posix::stream_descriptor userOutput_;
    //Last one acts as a buffer for data operations on stdin.
    boost::asio::streambuf input_buffer_;

public:
    TCPClient(std::string,int);

    //Async connect.
    void start_connect();
    void handle_connect(const boost::system::error_code&);

    //Set up server data listener.
    void start_listen();
    void handle_server_message(const boost::system::error_code &, size_t);

    //Set up listener for user's input with Boost.asio
    void start_get_std_input();
    void handle_std_input(const boost::system::error_code &, size_t);

    void parse_user_message(std::string);


    //Handle the functionality of the client-side server functions.
    void handle_write(const boost::system::error_code&);
    //Send functions
    void terminate_connection();
    void handle_login(std::vector<std::string>*);
    void handle_newuser(std::vector<std::string>*);
    void handle_send(std::string);
    void handle_logout();
    void handle_who();


    void run();

};
