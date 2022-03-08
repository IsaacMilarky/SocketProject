#include "ServerTCPConnection.hpp"

class TCPServer
{
    boost::asio::io_service server_ioservice;
    boost::asio::tcp::acceptor server_acceptor;
    
    //Used for the .emplace() method on connection structs
    std::list<ServerTCPConnection> server_connections;

public:

    TCPServer();

    void handle_read(std::list<ServerTCPConnection>::iterator, boost::system::error_code const & , size_t );
    void do_async_read(std::list<ServerTCPConnection>::iterator);

    void handle_write(std::list<ServerTCPConnection>::iterator, std::shared_ptr<std::string>, boost::system::error_code const &);

    void handle_accept(std::list<ServerTCPConnection>::iterator, boost::system::error_code const &);
    void start_accept();

    //listen on port
    void listen(int);

    void run();
}