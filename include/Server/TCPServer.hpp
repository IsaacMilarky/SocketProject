#include "ServerTCPConnection.hpp"
#include "ServerFunctions.hpp"
#include <vector>
#include <sstream>
#include <algorithm>
#include <iterator>
#include <string>
#include <deque>

/*
    Encapsulates the function of an async tcp server running on a port.

    Written by Isaac Milarsky 3/8/2022
*/

class TCPServer
{
    boost::asio::io_service server_ioservice;
    boost::asio::ip::tcp::acceptor server_acceptor;
    
public:

    TCPServer();

    std::string handle_read(ServerTCPConnection*, size_t );
    int do_read(ServerTCPConnection*, std::vector<std::string>*);

    void handle_write(ServerTCPConnection*, std::shared_ptr<std::string>, boost::system::error_code const &);

    void handle_accept(ServerTCPConnection*);
    void start_accept();

    //listen on ports
    void listen(int);

    void run();
};