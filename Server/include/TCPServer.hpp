#include "ServerTCPConnection.hpp"
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
    Encapsulates the function of a sync tcp server running on a port.

    Written by Isaac Milarsky 3/8/2022
*/


//Simple enum macro so that we return a number when parsing user packet input
//then a switch statement is used to apply the operation after the meaning is parsed.
enum function {login,newuser,sendMessage, logout, exitFunction }; 


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
    std::unique_ptr<ServerTCPConnection> server_connections[MAX_CLIENTS];
    
public:

    TCPServer();
    ~TCPServer();

    void handle_read(int,boost::system::error_code const &, size_t );
    int do_read(int, std::vector<std::string>*,size_t);

    void handle_accept(int,boost::system::error_code const &);
    void start_accept();

    //listen on ports
    void listen(int);

    void save_users_to_file();

    //Handle functions once meaning is parsed and respond to client.
    void handle_login(std::string,std::string,int);
    void handle_newuser(std::string,std::string,int);
    void handle_send(std::string,int);
    void handle_logout(int);

    void run();
};