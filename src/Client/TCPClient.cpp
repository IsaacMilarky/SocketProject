#include "../../include/Client/TCPClient.hpp"
#include <memory>


TCPClient::TCPClient(std::string addressString, int port) : client_io_service(), chatConnection(client_io_service)
{
    target_host_endpoint = boost::asio::ip::tcp::endpoint(boost::asio::ip::address::from_string(addressString),port);

    //Start a session with the server.
    chatConnection.socket.connect(target_host_endpoint);
}


void TCPClient::parse_user_message(std::string userMessage)
{

    if(userMessage == "exit")
        return;

    std::stringstream messageStream(userMessage);
    const char delim = ' ';
    std::string token;
    std::string functionIdentifier;

    std::getline(messageStream,token,delim);

    functionIdentifier = token;
    
    //std::cout << functionIdentifier << std::endl;
    //std::cout << token << std::endl;
    //std::cout << userMessage << std::endl;

    std::vector<std::string> argList;

    if(token.compare("login") == 0)
    {
        while(std::getline(messageStream, token, delim))
        {
            //std::cout << "Arg: " << token << std::endl;
            argList.push_back(token);
        }

        handle_login(&argList);
        
        return;
    }
    else if(token.compare("newuser") == 0)
    {
        while(std::getline(messageStream, token, delim))
        {
            //std::cout << "Arg: " << token << std::endl;
            argList.push_back(token);
        }

        handle_newuser(&argList);
        return;
    }   
    else if(token.compare("send") == 0)
    {
        std::string bigArg = userMessage;
        size_t pos = bigArg.find("send");
        if(pos != std::string::npos)
        {
            bigArg.erase(pos,4);
        }
        //std::cout << "Arg: " << bigArg << std::endl;
        //argList.push_back(bigArg);

        handle_send(bigArg);

        return;
    }
    else if(token.compare("logout") == 0)
    {
        terminate_connection();

        return;
    }
    else
    {
        std::cout << "Command not recognized!" << std::endl;
        return;
    }
}

void TCPClient::terminate_connection()
{
    auto buff = std::make_shared<std::string>( "logout \r\n" );
    boost::system::error_code ignored_error;
    boost::asio::write( chatConnection.socket, boost::asio::buffer( *buff ), ignored_error );
}

void TCPClient::handle_login(std::vector<std::string> * argList)
{
    //std::cout << "Arglist: " << argList->size() << std::endl;
    if(argList->size() == 2)
    {
        auto buff = std::make_shared<std::string>( "login " + argList->at(0) + " " + argList->at(1) + " \r\n" );
        boost::system::error_code ignored_error;
        boost::asio::write( chatConnection.socket, boost::asio::buffer( *buff ), ignored_error );

        std::cout << wait_for_response() << std::endl;
    }
    else
    {
        std::cout << "Incorrect use of login command!" << std::endl;
    }
}

void TCPClient::handle_newuser(std::vector<std::string> * argList)
{
    if(argList->size() == 2)
    {
        std::string userId = argList->at(0);
        std::string pass = argList->at(1);

        if(userId.length() >= 3 && userId.length() <= 32 && pass.length() >= 4 && pass.length() <= 8 )
        {
            auto buff = std::make_shared<std::string>( "newuser " + argList->at(0) + " " + argList->at(1) + " \r\n" );
            boost::system::error_code ignored_error;
            boost::asio::write( chatConnection.socket, boost::asio::buffer( *buff ), ignored_error );

            std::cout << wait_for_response() << std::endl;
            return;
        }
    }
    
    
    std::cout << "Incorrect use of newuser!" << std::endl;
    
}

void TCPClient::handle_send(std::string message)
{
    if(message.length() <= 256)
    {
        auto buff = std::make_shared<std::string>("send " + message + " \r\n");
        boost::system::error_code ignored_error;
        boost::asio::write(chatConnection.socket,boost::asio::buffer(*buff),ignored_error);

        std::cout << wait_for_response() << std::endl;
        return;
    }

    std::cout << "Incorrect usage of send!" << std::endl;
}


std::string TCPClient::wait_for_response()
{
    boost::asio::read_until(chatConnection.socket, chatConnection.buffer, "\n");

    std::string line;

    if(chatConnection.buffer.size() > 0)
    {
        std::istream is( &chatConnection.buffer);

        std::getline(is, line);

        //std::cout << line << std::endl;

    }
    else {
        line = "";
    }

    return line;
}