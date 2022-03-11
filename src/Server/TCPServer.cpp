#include "../../include/Server/TCPServer.hpp"
#include <boost/system/detail/error_code.hpp>
#include <sstream>
#include <string>

TCPServer::TCPServer() : server_ioservice( ), server_acceptor( server_ioservice )
{
    //Get username password pairs from file
    std::ifstream usertext("../users.txt");

    if(usertext.is_open())
    {
        std::string line;

        while(std::getline(usertext,line))
        {
            std::cout << "Got line: " << line << std::endl;

            const char delim = ',';
            std::string token;
            std::stringstream userLine(line);

            std::getline(userLine,token,delim);

            std::string user = token;
            //Remove the first character for user.
            user.erase(user.begin());

            std::cout << "user: " << user << std::endl;
            std::getline(userLine,token,delim);

            std::string password = token;

            password.erase(password.end());

            std::cout << "Password: " << password << std::endl;
        }


        usertext.close();

    }
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

    client_message.pop_back();

    std::stringstream streamData(client_message);
    //return an enum corresponding to the relevant function or -1 if not a valid function.
    const char delim = ' ';
    std::string token;

    std::getline(streamData,token, delim);
    
    //std::cout << "Token: " << token << std::endl;

    //std::cout << "Compare: " << token.compare("logout") << std::endl;

    if(token.compare("login") == 0)
    {
        while(std::getline(streamData, token, delim))
        {
            std::cout << "Arg: " << token << std::endl;
            args->push_back(token);
        }
        
        return login;
    }
    else if(token.compare("newuser") == 0)
    {
        while(std::getline(streamData, token, delim))
        {
            std::cout << "Arg: " << token << std::endl;
            args->push_back(token);
        }

        return newuser;
    }   
    else if(token.compare("send") == 0)
    {
        std::cout << "Arg: " << client_message << std::endl;
        args->push_back(client_message);

        return sendMessage;
    }
    else if(token.compare("logout") == 0)
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


        switch(code)
        {
            case login:
                handle_login(arguments.at(0), arguments.at(1), connectionID);
                break;
        }
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


void TCPServer::handle_login(std::string userID, std::string password, ServerTCPConnection * connectionID)
{

}



void TCPServer::run()
{
    server_ioservice.run();
}