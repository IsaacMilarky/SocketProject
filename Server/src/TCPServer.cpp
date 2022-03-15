#include "../include/TCPServer.hpp"
#include <boost/system/detail/error_code.hpp>
#include <fstream>
#include <sstream>
#include <string>
#include <map>

//Init ioservice and acceptor in constant definitions up top.
TCPServer::TCPServer() : server_ioservice( ), server_acceptor( server_ioservice )
{
    //Get username password pairs from file
    std::ifstream usertext("../users.txt");

    if(usertext.is_open())
    {
        std::string line;
        
        //Parse each line of username password file
        while(std::getline(usertext,line))
        {
            //std::cout << "Got line: " << line << std::endl;

            const char delim = ',';
            std::string token;

            //Put file's line in string stream and get characters before and after ','
            std::stringstream userLine(line);
            std::getline(userLine,token,delim);

            std::string user = token;
            //Remove the characters not part of user or password ' ' or '(',')'
            user.erase(remove(user.begin(), user.end(), ' '), user.end());
            user.erase(remove(user.begin(), user.end(), '('), user.end());
            user.erase(remove(user.begin(), user.end(), ')'), user.end());

            //Same idea but get text after , for the password and remove parenthesis and spaces
            std::getline(userLine,token,delim);

            std::string password = token;

            password.erase(password.length() - 1);

            password.erase(remove(password.begin(), password.end(), ' '), password.end());
            password.erase(remove(password.begin(), password.end(), '('), password.end());
            password.erase(remove(password.begin(), password.end(), ')'), password.end());

            //std::cout << "Password: " << password << std::endl;

            //Record username password pair and that they haven't connected yet.
            usernamePasswordPairs[user] = password;
            userloginStatus[user] = nullptr;
        }


        usertext.close();

    }
}

TCPServer::~TCPServer()
{
    
}

//Put all the contents of the given connection's socket buffer in a string using '\n' as terminator
std::string TCPServer::handle_read(ServerTCPConnection* connectionID, size_t bytes_transferred)
{
    std::string line;

    if(bytes_transferred > 0)
    {
        std::istream is( &connectionID->buffer);

        std::getline(is, line);


    }
    else {
        line = "";
    }

    return line;
}

//Start reading content from user until newline is received and then parse meaning of query.
//return an enum corresponding to the relevant function or -1 if not a valid function.
int TCPServer::do_read(ServerTCPConnection* connectionID, std::vector<std::string>* args)
{
    try
    {
        //Read until newline.
        boost::asio::read_until(connectionID->socket, connectionID->buffer, "\n");

        //Put socket buffer in string.
        std::string client_message = handle_read(connectionID, connectionID->buffer.size());


        client_message.pop_back();
        std::stringstream streamData(client_message);
        
        //Pop off first word.
        const char delim = ' ';
        std::string token;
        std::getline(streamData,token, delim);
    
        //Figure out desired user function based on first word.
        if(token.compare("login") == 0)
        {
            //Pop off arguments and pass to vector.
            while(std::getline(streamData, token, delim))
            {
                //std::cout << "Arg: " << token << std::endl;
                args->push_back(token);
            }

            return login;
        }
        else if(token.compare("newuser") == 0)
        {
            while(std::getline(streamData, token, delim))
            {
                //std::cout << "Arg: " << token << std::endl;
                args->push_back(token);
            }

            return newuser;
        }   
        else if(token.compare("send") == 0)
        {
            //Don't send back 'send' as part of message.
            size_t pos = client_message.find("send");

            if(pos != std::string::npos)
            {
                client_message.erase(pos,4);
            }

            //Push back the whole message except the function specifier.
            args->push_back(client_message);

            return sendMessage;
        }
        else if(token.compare("logout") == 0)
        {
            return logout;
        }
        else if(token.compare("exit") == 0)
        {
            return exitFunction;
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "Connection terminated before client sent data!" << std::endl;
        std::cerr << e.what() << std::endl;
        return exitFunction;
    }
    

    return -1;
}

//Called when server receives connection request from client.
void TCPServer::handle_accept(ServerTCPConnection* connectionID)
{
    
    //std::cout << "New connection from: " << connectionID->socket.remote_endpoint().address().to_string() << "\n";

    int code = -2;

    while(code != exitFunction)
    {
        //Have a place to store arguments.
        std::vector<std::string> arguments;

        //Read until logout.
        code = do_read( connectionID , &arguments);

        //Respond to parsed user input. We now have the specific function plus all arguments.
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
            case exitFunction:
                break;
            default:
                std::cout << "Error occured" << std::endl;
        }
    }
    
}

//This is the main server loop, blocks until connect request is seen.
void TCPServer::start_accept()
{
    //Iterative server loop for only one client at a time.

    for(;;)
    {
        //Create a socket and buffer from OS io object from boost.
        ServerTCPConnection connection(server_ioservice);

        //Block until it is possible to accept a connection.
        server_acceptor.accept(connection.socket);

        handle_accept(&connection);
    }
}

//Create the endpoint that the client will connect to and then try to accept a connection.
void TCPServer::listen(int port)
{
    auto endpoint = boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port);
    server_acceptor.open(endpoint.protocol());
    server_acceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
    server_acceptor.bind(endpoint);
    server_acceptor.listen();
    start_accept();
}


//Saves the contents of the usernamepasswordPairs variable that stores login info.
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


//Perform the login function and respond to the connection that made it.
void TCPServer::handle_login(std::string userID, std::string password, ServerTCPConnection * connectionID)
{
    //Search for connection that matches the connection that requested login.
    std::map<std::string,ServerTCPConnection*>::iterator iter = userloginStatus.begin();
    bool userLoggedin = false;

    while(iter != userloginStatus.end())
    {
        if(iter->second != nullptr && connectionID->socket.remote_endpoint() == iter->second->socket.remote_endpoint())
        {
            //If we find a matching connection we know the user is logged in.
            userLoggedin = true;
            //userName = iter->first;
            break;
        }
        iter++;
    }

    if(!userLoggedin)
    {
        //Make sure user exists with the specified password.
        if(usernamePasswordPairs.count(userID) && usernamePasswordPairs[userID].compare(password) == 0)
        {
            //Cache what connection logged on as that user for later confirmation.
            userloginStatus[userID] = connectionID;

            //Tell the user that they are logged in.
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
    else
    {
        auto buff = std::make_shared<std::string>( "Denied. Can't login while logged in.\r\n" );
        boost::system::error_code ignored_error;
        boost::asio::write( connectionID->socket, boost::asio::buffer( *buff ), ignored_error );
    }

}

//Perform the newuser function and respond to the connection that made it.
void TCPServer::handle_newuser(std::string userID, std::string password, ServerTCPConnection * connectionID)
{

    //Search for connection that matches the connection that requested newuser.
    std::map<std::string,ServerTCPConnection*>::iterator iter = userloginStatus.begin();
    bool userLoggedin = false;

    while(iter != userloginStatus.end())
    {
        if(iter->second != nullptr && connectionID->socket.remote_endpoint() == iter->second->socket.remote_endpoint())
        {
            //If we find a matching connection we know the user is logged in.
            userLoggedin = true;
            //userName = iter->first;
            break;
        }
        iter++;
    }

    if(!userLoggedin)
    {
        if(usernamePasswordPairs.count(userID))
        {
            auto buff = std::make_shared<std::string>( "Denied. User account already exists.\r\n" );
            boost::system::error_code ignored_error;
            boost::asio::write( connectionID->socket, boost::asio::buffer( *buff ), ignored_error );

        }
        else
        {
            //Add user to map
            usernamePasswordPairs[userID] = password;


            std::cout << "New user account created." << std::endl;
            auto buff = std::make_shared<std::string>( "New user account created. Please login.\r\n" );
            boost::system::error_code ignored_error;
            boost::asio::write( connectionID->socket, boost::asio::buffer( *buff ), ignored_error );

            //Write users to file as they are created so that if program is closed they persist.
            save_users_to_file();
        }
    }
    else
    {
        auto buff = std::make_shared<std::string>( "Denied. Can't create user while logged in.\r\n" );
        boost::system::error_code ignored_error;
        boost::asio::write( connectionID->socket, boost::asio::buffer( *buff ), ignored_error );
    }

}

//Perform the send function and respond to the connection that made it.
void TCPServer::handle_send(std::string message, ServerTCPConnection * connectionID)
{
    std::map<std::string,ServerTCPConnection*>::iterator iter = userloginStatus.begin();

    bool userLoggedin = false;
    //Also get the userID to send back.
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

//Perform the logout function and respond to the connection that made it.
void TCPServer::handle_logout(ServerTCPConnection * connectionID)
{
    //First figure out if the connection is logged in and record the name if it is.
    std::map<std::string,ServerTCPConnection*>::iterator iter = userloginStatus.begin();
    bool userWasLoggedIn = false;
    std::string name = "";

    while(iter != userloginStatus.end())
    {
        if(iter->second != nullptr && connectionID->socket.remote_endpoint() == iter->second->socket.remote_endpoint())
        {
            //Log them out here if they are logged in.
            userloginStatus[iter->first] = nullptr;
            std::cout << iter->first << " logout." << std::endl;

            //They were logged in, send a success message later on.
            userWasLoggedIn = true;
            name = iter->first;
            break;
        }
        iter++;
    }

    //Send success if suceeded.
    if(userWasLoggedIn)
    {
        auto buff = std::make_shared<std::string>( name + " left.\r\n" );
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


void TCPServer::run()
{
    server_ioservice.run();
}