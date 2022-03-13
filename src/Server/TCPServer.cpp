#include "../../include/Server/TCPServer.hpp"
#include <boost/system/detail/error_code.hpp>
#include <fstream>
#include <sstream>
#include <string>
#include <map>

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
            user.erase(remove(user.begin(), user.end(), ' '), user.end());
            user.erase(remove(user.begin(), user.end(), '('), user.end());
            user.erase(remove(user.begin(), user.end(), ')'), user.end());

            //std::cout << "user: " << user << std::endl;
            std::getline(userLine,token,delim);

            std::string password = token;

            password.erase(password.length() - 1);

            password.erase(remove(password.begin(), password.end(), ' '), password.end());
            password.erase(remove(password.begin(), password.end(), '('), password.end());
            password.erase(remove(password.begin(), password.end(), ')'), password.end());

            //std::cout << "Password: " << password << std::endl;

            usernamePasswordPairs[user] = password;
            userloginStatus[user] = nullptr;
        }


        usertext.close();

    }
}

TCPServer::~TCPServer()
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
    try
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
            size_t pos = client_message.find("send");

            if(pos != std::string::npos)
            {
                client_message.erase(pos,4);
            }

            std::cout << "Arg: " << client_message << std::endl;
            args->push_back(client_message);

            return sendMessage;
        }
        else if(token.compare("logout") == 0)
        {
            return logout;
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "Connection terminated before client sent data!" << std::endl;
        std::cerr << e.what() << std::endl;
        return logout;
    }
    

    return -1;
}

void TCPServer::handle_accept(ServerTCPConnection* connectionID)
{
    
    //std::cout << "New connection from: " << connectionID->socket.remote_endpoint().address().to_string() << "\n";

    int code = -2;

    while(code != logout)
    {
        //Have a place to store arguments.
        std::vector<std::string> arguments;

        //Read until logout.
        code = do_read( connectionID , &arguments);

        switch(code)
        {
            case login:
                handle_login(arguments.at(0), arguments.at(1), connectionID);
                break;
            case newuser:
                handle_newuser(arguments.at(0), arguments.at(1), connectionID);
                break;
            case sendMessage:
                handle_send(arguments.at(0), connectionID);
                break;
            case logout:
                handle_logout(connectionID);
                break;
            default:
                std::cout << "Error occured" << std::endl;
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


void TCPServer::save_users_to_file()
{
    std::ofstream usertext("../users.txt",std::ofstream::trunc);

    if(usertext.is_open())
    {
        //Write map to file
        std::map<std::string,std::string>::iterator iter = usernamePasswordPairs.begin();

        while(iter != usernamePasswordPairs.end())
        {
            usertext << "(" << iter->first << ", " << iter->second << ")\n";
            iter++;
        }

        usertext.close();
    }
    else
        std::cout << "Unable to write users and passwords to file" << std::endl;
}



void TCPServer::handle_login(std::string userID, std::string password, ServerTCPConnection * connectionID)
{
    if(usernamePasswordPairs.count(userID) && usernamePasswordPairs[userID].compare(password) == 0)
    {
        userloginStatus[userID] = connectionID;

        auto buff = std::make_shared<std::string>( "login confirmed\r\n" );
        boost::system::error_code ignored_error;
        boost::asio::write( connectionID->socket, boost::asio::buffer( *buff ), ignored_error );

        std::cout << userID << " login." << std::endl;
    }
    else
    {
        //Denied. User name or password incorrect.
        auto buff = std::make_shared<std::string>( "Denied. User name or password incorrect.\r\n" );
        boost::system::error_code ignored_error;
        boost::asio::write( connectionID->socket, boost::asio::buffer( *buff ), ignored_error );
    }
}

void TCPServer::handle_newuser(std::string userID, std::string password, ServerTCPConnection * connectionID)
{
    if(usernamePasswordPairs.count(userID))
    {
        auto buff = std::make_shared<std::string>( "Denied. User account already exists.\r\n" );
        boost::system::error_code ignored_error;
        boost::asio::write( connectionID->socket, boost::asio::buffer( *buff ), ignored_error );
        
    }
    else
    {
        usernamePasswordPairs[userID] = password;


        std::cout << "New user account created." << std::endl;
        auto buff = std::make_shared<std::string>( "New user account created. Please login.\r\n" );
        boost::system::error_code ignored_error;
        boost::asio::write( connectionID->socket, boost::asio::buffer( *buff ), ignored_error );

        save_users_to_file();
    }

    

}

void TCPServer::handle_send(std::string message, ServerTCPConnection * connectionID)
{
    std::map<std::string,ServerTCPConnection*>::iterator iter = userloginStatus.begin();

    bool userLoggedin = false;
    std::string userID = "";

    while(iter != userloginStatus.end())
    {
        if(iter->second != nullptr && connectionID->socket.remote_endpoint() == iter->second->socket.remote_endpoint())
        {
            userLoggedin = true;
            userID = iter->first;
            break;
        }
        iter++;
    }

    if(userLoggedin)
    {
        //Send to client (Version 1 is only one client version 2 would send to all connected clients)
        std::cout << userID << ":" << message << std::endl;

        auto buff = std::make_shared<std::string>( userID + ":" + message + "\r\n" );
        boost::system::error_code ignored_error;
        boost::asio::write( connectionID->socket, boost::asio::buffer( *buff ), ignored_error );
    }
    else
    {
        auto buff = std::make_shared<std::string>( "Denied. Please login first.\r\n" );
        boost::system::error_code ignored_error;
        boost::asio::write( connectionID->socket, boost::asio::buffer( *buff ), ignored_error );
    }

}

void TCPServer::handle_logout(ServerTCPConnection * connectionID)
{
    std::map<std::string,ServerTCPConnection*>::iterator iter = userloginStatus.begin();

    while(iter != userloginStatus.end())
    {
        if(iter->second != nullptr && connectionID->socket.remote_endpoint() == iter->second->socket.remote_endpoint())
        {
            userloginStatus[iter->first] = nullptr;
            std::cout << iter->first << " left." << std::endl;

            break;
        }
        iter++;
    }
}


void TCPServer::run()
{
    server_ioservice.run();
}