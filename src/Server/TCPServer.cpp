#include "../../include/Server/TCPServer.hpp"

TCPServer::TCPServer() : server_ioservice( ), server_acceptor( server_ioservice ), server_connections( ) 
{

}


void TCPServer::handle_read(std::list<ServerTCPConnection>::iterator connectionID, boost::system::error_code const & err, size_t bytes_transferred)
{
    if(bytes_transferred > 0)
    {
        std::istream is( &connectionID->read_buffer);
        std::string line;

        std::getline(is, line);

        std::cout << "[Connection] has received message: \n \t" << line << std::endl;

    }

    if(!err)
    {
        do_async_read(connectionID);
    }
    else
    {
        std::cerr << "Unexpected error: " << err.message() << std::endl;
        server_connections.erase(connectionID);
    }
}

void TCPServer::do_async_read(std::list<ServerTCPConnection>::iterator connectionID)
{
    auto handler = boost::bind(&TCPServer::handle_read, this, connectionID, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred);

    boost::asio::async_read_until(connectionID->socket, connectionID->read_buffer, "\n", handler);
}

void TCPServer::handle_write(std::list<ServerTCPConnection>::iterator connectionID, std::shared_ptr<std::string> messageBuffer, boost::system::error_code const & err)
{
    if(!err)
    {
        std::cout << "Message sent\n";

        if( connectionID->socket.is_open())
        {

        }
    }
    else
    {
        std::cerr << "We had an error: " << err.message() << std::endl;
        server_connections.erase(connectionID);
    }
}

void TCPServer::handle_accept(std::list<ServerTCPConnection>::iterator connectionID, boost::system::error_code const & err)
{
    if(!err)
    {
        std::cout << "Connection from: " << connectionID->socket.remote_endpoint().address().to_string() << "\n";
		std::cout << "Sending message\n";
		auto buff = std::make_shared<std::string>( "Hello World!\r\n\r\n" );
		auto handler = boost::bind( &TCPServer::handle_write, this, connectionID, buff, boost::asio::placeholders::error );
		boost::asio::async_write( connectionID->socket, boost::asio::buffer( *buff ), handler );
		do_async_read( connectionID );
    }
    else
    {
        std::cerr << "Unexpected error: " << err.message() << std::endl;
        server_connections.erase(connectionID);
    }
    start_accept();
}

void TCPServer::start_accept()
{
    auto connectionID = server_connections.emplace(server_connections.begin(), server_ioservice);

    auto handler = boost::bind(&TCPServer::handle_accept, this, connectionID, boost::asio::placeholders::error);

    server_acceptor.async_accept(connectionID->socket, handler);
}

void TCPServer::listen(int port)
{
    auto endpoint = boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port);
    server_acceptor.open(endpoint.protocol());
    server_acceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
    server_acceptor.bind(endpoint);
    server_acceptor.listen();
    start_accept();
}

void TCPServer::run()
{
    server_ioservice.run();
}