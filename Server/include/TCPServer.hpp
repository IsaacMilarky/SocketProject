#include "ServerTCPConnection.hpp"
#include <vector>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <iterator>
#include <string>
#include <map>

/*
    Encapsulates the function of a sync tcp server running on a port.

    Written by Isaac Milarsky 3/8/2022
*/


//Simple enum macro so that we return a number when parsing user packet input
//then a switch statement is used to apply the operation after the meaning is parsed.
enum function {login,newuser,sendMessage, logout}; 


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
    
public:

    TCPServer();
    ~TCPServer();

    std::string handle_read(ServerTCPConnection*, size_t );
    int do_read(ServerTCPConnection*, std::vector<std::string>*);

    void handle_accept(ServerTCPConnection*);
    void start_accept();

    //listen on ports
    void listen(int);

    void save_users_to_file();

    void handle_login(std::string,std::string,ServerTCPConnection *);
    void handle_newuser(std::string,std::string,ServerTCPConnection*);
    void handle_send(std::string,ServerTCPConnection*);
    void handle_logout(ServerTCPConnection*);

    void run();
};