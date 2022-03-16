#include "../include/TCPClient.hpp"
#include <memory>


//Init ioservice and take care of everything needed to start the connection.
TCPClient::TCPClient(std::string addressString, int port) : client_io_service(), chatConnection(client_io_service)
{
    target_host_endpoint = boost::asio::ip::tcp::endpoint(boost::asio::ip::address::from_string(addressString),port);

    //Start a session with the server.
    chatConnection.socket.connect(target_host_endpoint);
}

//Get a line from the user and parse it for the desired function to send to the server.
void TCPClient::parse_user_message(std::string userMessage)
{

    //Similar to server parse section, pop off first word and get function
    std::stringstream messageStream(userMessage);
    const char delim = ' ';
    std::string token;

    std::vector<std::string> argList;

    //Put the first word of the userMessage in the token.
    std::getline(messageStream,token,delim);


    if(token.compare("login") == 0)
    {
        //Get the rest of the words and add them as arguments.
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
        //Remove the first word from the message and then use that as the argument.
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
    else if(token.compare("who") == 0)
    {
        handle_who();
        return;
    }
    else if(token.compare("logout") == 0)
    {
        //Send logout message to server.
        handle_logout();

        return;
    }
    else if(token.compare("exit") == 0)
    {
        //End server connection and exit.
        terminate_connection();
        return;
    }
    else
    {
        std::cout << "Command not recognized!" << std::endl;
        return;
    }
}

//Tell the server to not wait for more data and accept a differant connection.
void TCPClient::terminate_connection()
{
    auto buff = std::make_shared<std::string>( "exit \r\n" );
    boost::system::error_code ignored_error;
    boost::asio::write( chatConnection.socket, boost::asio::buffer( *buff ), ignored_error );
}

//Check the arguments and then send the login request to the server.
void TCPClient::handle_login(std::vector<std::string> * argList)
{
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

//Check the argument amount and size before sending newuser command to the server with a string.
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

//Check the message size limit before sending.
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

//Send standard constant logout message.
void TCPClient::handle_logout()
{
    auto buff = std::make_shared<std::string>( "logout \r\n" );
    boost::system::error_code ignored_error;
    boost::asio::write( chatConnection.socket, boost::asio::buffer( *buff ), ignored_error );

    std::cout << wait_for_response() << std::endl;
}

void TCPClient::handle_who()
{
    auto buff = std::make_shared<std::string>( "who \r\n" );
    boost::system::error_code ignored_error;
    boost::asio::write( chatConnection.socket, boost::asio::buffer( *buff ), ignored_error );

    std::cout << wait_for_response() << std::endl;
}

//Read data from the server socket until a newline is found and then return the message to the user.
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