#include "ServerTCPConnection.hpp"
#include <array>
#include <memory>
#include <vector>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <iterator>
#include <string>
#include <map>
#include <stdexcept>


#define MAX_CLIENTS 3
/*
    Encapsulates the function of an async tcp server running on a port.

    Written by Isaac Milarsky 3/8/2022
*/


//Simple enum macro so that we return a number when parsing user packet input
//then a switch statement is used to apply the operation after the meaning is parsed.
enum function {login,newuser,sendMessageAll,sendMessageUser, who, logout, exitFunction }; 


class TCPServer
{
    //Boost object representing OS io functionality and service.
    //Create sockets from this OS independent io object.
    boost::asio::io_service server_ioservice;
    //Accepts connections
    boost::asio::ip::tcp::acceptor server_acceptor;

    std::map<std::string,std::string> usernamePasswordPairs;
    //Keep connections in map.
    std::map<std::string,ServerTCPConnection*> userloginStatus;

    //Active connections in here. Keep in array of smart pointers.
    //std::unique_ptr<ServerTCPConnection> server_connections[MAX_CLIENTS];
    using ConnectIndex = std::array<std::unique_ptr<ServerTCPConnection>,MAX_CLIENTS>::iterator;
    std::array<std::unique_ptr<ServerTCPConnection>, MAX_CLIENTS> server_connections;
    
public:

    //Load user variables and set up server_ioservice.
    TCPServer(int);
    ~TCPServer();

    void loadExistingUsers(std::map<std::string,std::string>);
    //Process data recieved from the user once recieved by socket.
    void handle_read(ConnectIndex,boost::system::error_code const &, size_t );

    //Determine the desired function from the client's message and return it's code.
    int do_read(ConnectIndex, std::vector<std::string>*,size_t);

    //Handle an incoming connect() from a client and set up async_reads to be processed by handle_read
    void handle_accept(ConnectIndex,boost::system::error_code const &);

    //Start up another async_accept when the previous connection has been serviced.
    void start_accept();

    //Saves all user information to users.txt
    void save_users_to_file();

    //Handle functions once meaning is parsed and respond to client.
    void handle_login(std::string,std::string,ConnectIndex);
    void handle_newuser(std::string,std::string,ConnectIndex);
    void handle_send_all(std::string,ConnectIndex);
    void handle_send_user(std::string,std::string,ConnectIndex);
    void handle_who(ConnectIndex);
    void handle_logout(ConnectIndex);

    //Start all async boost.asio operations.
    void run();
};