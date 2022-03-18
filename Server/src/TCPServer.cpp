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
void TCPServer::handle_read(int connectionID,boost::system::error_code const & err, size_t bytes_transferred)
{
    if(err)
    {
        std::cerr << "Connection terminated before data was read!" << std::endl;
        server_connections[connectionID].reset();
        start_accept();
        return;
    }

    int code = -2;
    
    //Have a place to store arguments.
    std::vector<std::string> arguments;

    //Read until logout.
    code = do_read( connectionID , &arguments,bytes_transferred);

    //Respond to parsed user input. We now have the specific function plus all arguments.
    switch(code)
    {
        case login:
            handle_login(arguments.at(0), arguments.at(1), connectionID);
            break;
        case newuser:
            handle_newuser(arguments.at(0), arguments.at(1), connectionID);
            break;
        case sendMessageAll:
            handle_send_all(arguments.at(0), connectionID);
            break;
        case sendMessageUser:
            handle_send_user(arguments.at(0),arguments.at(1),connectionID);
            break;
        case who:
            handle_who(connectionID);
            break;
        case logout:
            handle_logout(connectionID);
            break;
        case exitFunction:
            break;
        default:
            std::cout << "Error occured" << std::endl;
    }
    
    if(code != exitFunction)
    {
        //If message is not an exit message create a async read listener to read the next message.
        auto handler = boost::bind(&TCPServer::handle_read,this,connectionID,boost::asio::placeholders::error,boost::asio::placeholders::bytes_transferred);
        boost::asio::async_read_until(server_connections[connectionID]->socket, server_connections[connectionID]->buffer, "\n",handler);
    }
    else
    {
        server_connections[connectionID].reset();
        start_accept();
    }
}

//Start reading content from user until newline is received and then parse meaning of query.
//return an enum corresponding to the relevant function or -1 if not a valid function.
int TCPServer::do_read(int connectionID, std::vector<std::string>* args,size_t bytes_transferred)
{
    try
    {
        std::string line;

        if(bytes_transferred > 0)
        {
            std::istream is( &server_connections[connectionID]->buffer);

            std::getline(is, line);


        }
        else {
            line = "";
        }

        std::string client_message = line;

        //std::cout << "CLient message: " << line << std::endl;

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
            //Check if send 'all' or send.
            std::getline(streamData,token, delim);
            if(token.compare("all") == 0)
            {
                std::string message = "";

                while(std::getline(streamData,token,delim))
                {
                    message += token + " ";
                }
                args->push_back(message);

                return sendMessageAll;
            }
            else
            {
                args->push_back(token);
                std::string message = "";

                while(std::getline(streamData,token,delim))
                {
                    message += token + " ";
                }
                args->push_back(message);
                return sendMessageUser;
            }
            
        }
        else if(token.compare("who") == 0)
        {
            return who;
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
        server_connections[connectionID].reset();
        return exitFunction;
    }
    

    return -1;
}

//Called when server receives connection request from client.
void TCPServer::handle_accept(int connectionID, boost::system::error_code const & err)
{
    
    //std::cout << "New connection from: " << connectionID->socket.remote_endpoint().address().to_string() << "\n";
    if(!err)
    {
        auto handler = boost::bind(&TCPServer::handle_read,this,connectionID,boost::asio::placeholders::error,boost::asio::placeholders::bytes_transferred);
        boost::asio::async_read_until(server_connections[connectionID]->socket, server_connections[connectionID]->buffer, "\n",handler);
    }
    else
    {
        std::cerr << "Error trying to accept connection!" << std::endl;
        std::cerr << err.message() << std::endl;
        //Delete connection object.
        server_connections[connectionID].reset();
    }

    start_accept();
    
}

//This is the main server loop. Sets up async acceptor.
void TCPServer::start_accept()
{
    int index = -1;

    for(int iter = 0; iter < MAX_CLIENTS; iter++)
    {
        //std::cout << "index " << iter << std::endl;
       //Check if there is an empty spot.
        ServerTCPConnection * connectionRef = server_connections[iter].get();

        //std::cout << connectionRef << std::endl;
        if(connectionRef == nullptr)
        {
            index = iter;
            break;
        }

    }

    if(index == -1)
    {
        //std::cerr << "No more available connections!" << std::endl;
        return;
    }
    //Create a socket and buffer from OS io object from boost.

    server_connections[index] = std::unique_ptr<ServerTCPConnection>(new ServerTCPConnection(server_ioservice));

    auto handler = boost::bind(&TCPServer::handle_accept,this,index,boost::asio::placeholders::error);
    //Block until it is possible to accept a connection.
    server_acceptor.async_accept(server_connections[index]->socket,handler);

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
void TCPServer::handle_login(std::string userID, std::string password, int connectionID)
{
    //Search for connection that matches the connection that requested login.
    std::map<std::string,ServerTCPConnection*>::iterator iter = userloginStatus.begin();
    bool userLoggedin = false;

    while(iter != userloginStatus.end())
    {
        if(iter->second != nullptr && server_connections[connectionID]->socket.remote_endpoint() == iter->second->socket.remote_endpoint())
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
            userloginStatus[userID] = server_connections[connectionID].get();

            //Tell the user that they are logged in.
            auto buff = std::make_shared<std::string>( ">login confirmed\r\n" );
            boost::system::error_code ignored_error;
            boost::asio::write( server_connections[connectionID]->socket, boost::asio::buffer( *buff ), ignored_error );

            buff = std::make_shared<std::string>( userID + " joins.\r\n" );

            for(int iter = 0; iter < MAX_CLIENTS; iter++)
            {
                if(iter == connectionID)
                {
                    continue;
                }
                //Check if there is an empty spot.
                ServerTCPConnection * connectionRef = server_connections[iter].get();

                if(connectionRef != nullptr)
                {
                    boost::asio::write( connectionRef->socket, boost::asio::buffer( *buff ), ignored_error );
                }
            }

            std::cout << userID << " login." << std::endl;
        }
        else
        {
            //Denied. User name or password incorrect.
            auto buff = std::make_shared<std::string>( ">Denied. User name or password incorrect.\r\n" );
            boost::system::error_code ignored_error;
            boost::asio::write( server_connections[connectionID]->socket, boost::asio::buffer( *buff ), ignored_error );
        }
    }
    else
    {
        auto buff = std::make_shared<std::string>( ">Denied. Can't login while logged in.\r\n" );
        boost::system::error_code ignored_error;
        boost::asio::write( server_connections[connectionID]->socket, boost::asio::buffer( *buff ), ignored_error );
    }

}

//Perform the newuser function and respond to the connection that made it.
void TCPServer::handle_newuser(std::string userID, std::string password, int connectionID)
{

    //Search for connection that matches the connection that requested newuser.
    std::map<std::string,ServerTCPConnection*>::iterator iter = userloginStatus.begin();
    bool userLoggedin = false;

    while(iter != userloginStatus.end())
    {
        if(iter->second != nullptr && server_connections[connectionID]->socket.remote_endpoint() == iter->second->socket.remote_endpoint())
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
            auto buff = std::make_shared<std::string>( ">Denied. User account already exists.\r\n" );
            boost::system::error_code ignored_error;
            boost::asio::write( server_connections[connectionID]->socket, boost::asio::buffer( *buff ), ignored_error );

        }
        else
        {
            //Add user to map
            usernamePasswordPairs[userID] = password;


            std::cout << "New user account created." << std::endl;
            auto buff = std::make_shared<std::string>( ">New user account created. Please login.\r\n" );
            boost::system::error_code ignored_error;
            boost::asio::write( server_connections[connectionID]->socket, boost::asio::buffer( *buff ), ignored_error );

            //Write users to file as they are created so that if program is closed they persist.
            save_users_to_file();
        }
    }
    else
    {
        auto buff = std::make_shared<std::string>( ">Denied. Can't create user while logged in.\r\n" );
        boost::system::error_code ignored_error;
        boost::asio::write( server_connections[connectionID]->socket, boost::asio::buffer( *buff ), ignored_error );
    }

}

//Perform the send function and respond to the connection that made it.
void TCPServer::handle_send_all(std::string message, int connectionID)
{
    std::map<std::string,ServerTCPConnection*>::iterator iter = userloginStatus.begin();

    bool userLoggedin = false;
    //Also get the userID to send back.
    std::string userID = "";

    while(iter != userloginStatus.end())
    {
        if(iter->second != nullptr && server_connections[connectionID]->socket.remote_endpoint() == iter->second->socket.remote_endpoint())
        {
            userLoggedin = true;
            userID = iter->first;
            break;
        }
        iter++;
    }

    if(userLoggedin)
    {
        //Send to clients
        std::cout << userID << ": " << message << std::endl;

        
        boost::system::error_code ignored_error;

        for(int iter = 0; iter < MAX_CLIENTS; iter++)
        {
            if(iter != connectionID)
            {
                auto buff = std::make_shared<std::string>(userID + ": " + message + "\r\n" );
                //Check if there is an empty spot.
                ServerTCPConnection * connectionRef = server_connections[iter].get();

                if(connectionRef != nullptr)
                {
                    boost::asio::write( connectionRef->socket, boost::asio::buffer( *buff ), ignored_error );
                }
            }
            else 
            {
                auto buff = std::make_shared<std::string>(">\r\n" );
                //Check if there is an empty spot.
                ServerTCPConnection * connectionRef = server_connections[iter].get();

                if(connectionRef != nullptr)
                {
                    boost::asio::write( connectionRef->socket, boost::asio::buffer( *buff ), ignored_error );
                }
            }
            
        }
    }
    else
    {
        auto buff = std::make_shared<std::string>( ">Denied. Please login first.\r\n" );
        boost::system::error_code ignored_error;
        boost::asio::write( server_connections[connectionID]->socket, boost::asio::buffer( *buff ), ignored_error );
    }

}

void TCPServer::handle_send_user(std::string userDst, std::string message, int connectionID)
{
    std::map<std::string,ServerTCPConnection*>::iterator iter = userloginStatus.begin();

    bool userLoggedin = false;
    //Also get the userID to send back.
    std::string userID = "";

    ServerTCPConnection * connectionRef = nullptr;

    while(iter != userloginStatus.end())
    {
        if(iter->second != nullptr && server_connections[connectionID]->socket.remote_endpoint() == iter->second->socket.remote_endpoint())
        {
            userLoggedin = true;
            userID = iter->first;
            connectionRef = iter->second;
            break;
        }
        iter++;
    }

    iter = userloginStatus.begin();
    ServerTCPConnection * dstRef = nullptr;
    while(iter != userloginStatus.end())
    {
        if(iter->second != nullptr && userDst == iter->first)
        {
            dstRef = iter->second;
            break;
        }
        iter++;
    }

    if(userLoggedin)
    {
        //Send to client
        if(dstRef)
        {
            std::cout << userID << " (to " << userDst << "): " << message << std::endl;

            auto buff = std::make_shared<std::string>( userID + ": " + message + "\r\n" );
            boost::system::error_code ignored_error;

            boost::asio::write( dstRef->socket, boost::asio::buffer( *buff ), ignored_error );

            //Respond to client that made request.
            buff = std::make_shared<std::string>( "\r\n" );
            boost::asio::write( server_connections[connectionID]->socket, boost::asio::buffer( *buff ), ignored_error );
        }
        else
        {
            auto buff = std::make_shared<std::string>( ">User requested not found!\r\n" );
            boost::system::error_code ignored_error;

            boost::asio::write( server_connections[connectionID]->socket, boost::asio::buffer( *buff ), ignored_error );
        }
        
    }
    else
    {
        auto buff = std::make_shared<std::string>( ">Denied. Please login first.\r\n" );
        boost::system::error_code ignored_error;
        boost::asio::write( server_connections[connectionID]->socket, boost::asio::buffer( *buff ), ignored_error );
    }

}


void TCPServer::handle_who(int connectionID)
{
    std::map<std::string,ServerTCPConnection*>::iterator iter = userloginStatus.begin();

    std::string message = "";

    while(iter != userloginStatus.end())
    {
        if(iter->second != nullptr )
        {
            
            message += iter->first + ", ";
        }
        iter++;
    }

    message.pop_back();
    message.pop_back();

    auto buff = std::make_shared<std::string>( ">" + message + "\r\n" );
    boost::system::error_code ignored_error;
    boost::asio::write( server_connections[connectionID]->socket, boost::asio::buffer( *buff ), ignored_error );

}


//Perform the logout function and respond to the connection that made it.
void TCPServer::handle_logout(int connectionID)
{
    //First figure out if the connection is logged in and record the name if it is.
    std::map<std::string,ServerTCPConnection*>::iterator iter = userloginStatus.begin();
    bool userWasLoggedIn = false;
    std::string name = "";

    while(iter != userloginStatus.end())
    {
        if(iter->second != nullptr && server_connections[connectionID]->socket.remote_endpoint() == iter->second->socket.remote_endpoint())
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
        
        boost::system::error_code ignored_error;
        //boost::asio::write( server_connections[connectionID]->socket, boost::asio::buffer( *buff ), ignored_error );
        for(int iter = 0; iter < MAX_CLIENTS; iter++)
        {
            if(iter != connectionID)
            {
                auto buff = std::make_shared<std::string>( name + " left.\r\n" );
                //Check if there is an empty spot.
                ServerTCPConnection * connectionRef = server_connections[iter].get();

                if(connectionRef != nullptr)
                {
                    boost::asio::write( connectionRef->socket, boost::asio::buffer( *buff ), ignored_error );
                }
            }
            else 
            {
                auto buff = std::make_shared<std::string>( ">\r\n" );
                //Check if there is an empty spot.
                ServerTCPConnection * connectionRef = server_connections[iter].get();

                if(connectionRef != nullptr)
                {
                    boost::asio::write( connectionRef->socket, boost::asio::buffer( *buff ), ignored_error );
                }
                
            }
            
        }
    }
    else
    {
        auto buff = std::make_shared<std::string>( ">Denied. Please login first.\r\n" );
        boost::system::error_code ignored_error;
        boost::asio::write( server_connections[connectionID]->socket, boost::asio::buffer( *buff ), ignored_error );
    }
}


void TCPServer::run()
{
    server_ioservice.run();
}