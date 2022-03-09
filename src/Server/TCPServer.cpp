#include "../../include/Server/TCPServer.hpp"
#include <boost/system/detail/error_code.hpp>

TCPServer::TCPServer() : server_ioservice( ), server_acceptor( server_ioservice )
{

}


std::string TCPServer::handle_read(ServerTCPConnection* connectionID, size_t bytes_transferred)
{
    std::string line;

    if(bytes_transferred > 0)
    {
        std::istream is( &connectionID->buffer);

        std::getline(is, line);

        std::cout << "[Connection] has received message: \n \t" << line << std::endl;

    }
    else {
        line = "";
    }

    return line;
}

int TCPServer::do_read(ServerTCPConnection* connectionID, std::vector<std::string>* args)
{
    boost::asio::read_until(connectionID->socket, connectionID->buffer, "\n");

    //boost::system::error_code ignored_error;
    std::string client_message = handle_read(connectionID, connectionID->buffer.size());

    //return an enum corresponding to the relevant function or -1 if not a valid function.
    std::string delim = " ";
    //Pop off first function argument.
    std::string serverFunctionToken = client_message.substr(0, client_message.find(delim));;    
    client_message.erase(0,client_message.find(delim) + delim.length());


    if(serverFunctionToken.compare("login") == 0)
    {
        while(client_message.length() > 0)
        {
            //Pop off next argument
            std:: cout << "Arg: " << client_message.substr(0, client_message.find(delim)) << std::endl;
            args->push_back(client_message.substr(0, client_message.find(delim)));
            client_message.erase(0,client_message.find(delim) + delim.length());
        }

        return login;
    }
    else if(serverFunctionToken.compare("newuser") == 0)
    {
        while(client_message.length() > 0)
        {
            //Pop off next argument
            std:: cout << "Arg: " << client_message.substr(0, client_message.find(delim)) << std::endl;
            args->push_back(client_message.substr(0, client_message.find(delim)));
            client_message.erase(0,client_message.find(delim) + delim.length());
        }

        return newuser;
    }
    else if(serverFunctionToken.compare("send") == 0)
    {
        //Pop off argument
        std:: cout << "Arg: " << client_message << std::endl;
        args->push_back(client_message);

        return sendMessage;
    }
    else if(serverFunctionToken.compare("logout") == 0)
    {
        return logout;
    }

    return -1;

}

//Not really used.
void TCPServer::handle_write(ServerTCPConnection* connectionID, std::shared_ptr<std::string> messageBuffer, boost::system::error_code const & err)
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
    }
}

void TCPServer::handle_accept(ServerTCPConnection* connectionID)
{
    
    std::cout << "New connection from: " << connectionID->socket.remote_endpoint().address().to_string() << "\n";

    int code = -2;

    while(code != logout)
    {
        //Have a place to store arguments.
        std::vector<std::string> arguments;

        //Read until logout.
        code = do_read( connectionID , &arguments);

        auto buff = std::make_shared<std::string>( "Hello World!\r\n\r\n" );
        boost::system::error_code ignored_error;

        boost::asio::write( connectionID->socket, boost::asio::buffer( *buff ), ignored_error );
    }
    
}

void TCPServer::start_accept()
{
    //Iterative server loop for only one client at a time.

    for(;;)
    {
        ServerTCPConnection connection(server_ioservice);

        //Block until it is possible to accept a connection.
        server_acceptor.accept(connection.socket);

        handle_accept(&connection);
    }
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