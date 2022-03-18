#include "../include/TCPServer.hpp"
#include <algorithm>
#include <boost/system/detail/error_code.hpp>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <map>

//Init ioservice and acceptor in constant definitions up top.
TCPServer::TCPServer(int port) : server_ioservice( ), server_acceptor( server_ioservice )
{
    
    //Create endpoint on port at 127.0.0.1 per instructions.
    auto endpoint = boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port);

    //Use socket api to open,bind and listen the endpoint.
    server_acceptor.open(endpoint.protocol());
    server_acceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
    server_acceptor.bind(endpoint);
    server_acceptor.listen();
    start_accept();
}

TCPServer::~TCPServer()
{
    
}

void TCPServer::loadExistingUsers(std::map<std::string, std::string> userPairs)
{
    for(auto const& iter : userPairs)
    {
        usernamePasswordPairs[iter.first] = iter.second;
        userloginStatus[iter.first] = nullptr;
    }
}

//Put all the contents of the given connection's socket buffer in a string using '\n' as terminator
void TCPServer::handle_read(ConnectIndex connectionID,boost::system::error_code const & err, size_t bytes_transferred)
{
    //Handle if response from client has encountered a problem.
    if(err)
    {
        std::cerr << "Connection terminated before data was read!" << std::endl;

        //Mark the connection pointer as nullptr and delete the connection object.
        (*connectionID).reset();
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
        //Pass relevant arguments to respective function handlers.
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
        boost::asio::async_read_until((*connectionID)->socket, (*connectionID)->buffer, "\n",handler);
    }
    else
    {
        //Mark the connection pointer as nullptr and delete the connection object.
        (*connectionID).reset();

        //Prepare to accept the next connection.
        start_accept();
    }
}

//Start reading content from user until newline is received and then parse meaning of query.
//return an enum corresponding to the relevant function or -1 if not a valid function.
int TCPServer::do_read(ConnectIndex connectionID, std::vector<std::string>* args,size_t bytes_transferred)
{
    try
    {
        //Get the data from the socket out of the connection buffer into a string.
        std::string line;

        if(bytes_transferred > 0)
        {
            std::istream is( &((*connectionID)->buffer));

            std::getline(is, line);


        }
        else {
            line = "";
        }

        std::string client_message = line;

        //std::cout << "CLient message: " << line << std::endl;

        //Remove newline.
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
        (*connectionID).reset();
        return exitFunction;
    }
    

    return -1;
}

//Called when server receives connection request from client.
void TCPServer::handle_accept(ConnectIndex connectionID, boost::system::error_code const & err)
{
    
    //std::cout << "New connection from: " << connectionID->socket.remote_endpoint().address().to_string() << "\n";
    if(!err)
    {
        //Create a listener to read data from socket and call handler when newline is found.
        auto handler = boost::bind(&TCPServer::handle_read,this,connectionID,boost::asio::placeholders::error,boost::asio::placeholders::bytes_transferred);
        boost::asio::async_read_until((*connectionID)->socket, (*connectionID)->buffer, "\n",handler);
    }
    else
    {
        std::cerr << "Error trying to accept connection!" << std::endl;
        std::cerr << err.message() << std::endl;
        //Delete connection object.
        (*connectionID).reset();
    }

    //Accept next connection when finished.
    start_accept();
    
}

//This is the main server loop. Sets up async acceptor.
void TCPServer::start_accept()
{
    //Search for a free index in the connection array, using -1 if not found.
    //Use suggestion from Arthur Vasser.
    auto connectionRef = std::find_if(server_connections.begin(), server_connections.end(), [](std::unique_ptr<ServerTCPConnection> &element){
        if(element == nullptr)
            return true;
        
        return false;
    });

    if(connectionRef == nullptr)
    {
        return;
    }

    //Create a socket and buffer from OS io object from boost.
    *connectionRef = std::unique_ptr<ServerTCPConnection>(new ServerTCPConnection(server_ioservice));

    //Create handler to be called when connect() is called by the client.
    auto handler = boost::bind(&TCPServer::handle_accept,this,connectionRef,boost::asio::placeholders::error);
    //Block until it is possible to accept a connection.
    server_acceptor.async_accept((*connectionRef)->socket,handler);

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
void TCPServer::handle_login(std::string userID, std::string password, ConnectIndex connectionID)
{
    //Search for connection that matches the connection that requested login.
    std::map<std::string,ServerTCPConnection*>::iterator iter = userloginStatus.begin();
    bool userLoggedin = false;

    while(iter != userloginStatus.end())
    {
        if(iter->second != nullptr && (*connectionID)->socket.remote_endpoint() == iter->second->socket.remote_endpoint())
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
            userloginStatus[userID] = (*connectionID).get();

            //Tell the user that they are logged in.
            std::string confirm( ">login confirmed\r\n" );
            respond(confirm, &(*connectionID)->socket);

            
            //Tell everyone except user that user has joined.   
            std::string userJoin( userID + " joins.\r\n" );

            for(int iter = 0; iter < MAX_CLIENTS; iter++)
            {
                if(server_connections[iter].get() == (*connectionID).get())
                {
                    continue;
                }
                //Check if there is an empty spot.
                ServerTCPConnection * connectionRef = server_connections[iter].get();

                if(connectionRef != nullptr)
                {
                    //boost::asio::write( connectionRef->socket, boost::asio::buffer( *buff ), ignored_error );
                    respond(userJoin, &connectionRef->socket);
                }
            }

            std::cout << userID << " login." << std::endl;
        }
        else
        {
            //Denied. User name or password incorrect.
            std::string denied( ">Denied. User name or password incorrect.\r\n" );
            respond(denied, &(*connectionID)->socket);
        }
    }
    else
    {
        std::string denial( ">Denied. Can't login while logged in.\r\n" );
        respond(denial, &(*connectionID)->socket);
    }

}

//Perform the newuser function and respond to the connection that made it.
void TCPServer::handle_newuser(std::string userID, std::string password, ConnectIndex connectionID)
{

    //Search for connection that matches the connection that requested newuser.
    std::map<std::string,ServerTCPConnection*>::iterator iter = userloginStatus.begin();
    bool userLoggedin = false;

    while(iter != userloginStatus.end())
    {
        if(iter->second != nullptr && (*connectionID)->socket.remote_endpoint() == iter->second->socket.remote_endpoint())
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
        //If count >= 1 then it exists and is true
        if(usernamePasswordPairs.count(userID))
        {
            std::string denied( ">Denied. User account already exists.\r\n" );
            respond(denied, &(*connectionID)->socket);

        }
        else
        {
            //Add user to map
            usernamePasswordPairs[userID] = password;


            std::cout << "New user account created." << std::endl;
            std::string newUserCreate( ">New user account created. Please login.\r\n" );
            respond(newUserCreate, &(*connectionID)->socket);

            //Write users to file as they are created so that if program is closed they persist.
            save_users_to_file();
        }
    }
    else
    {
        std::string denied( ">Denied. Can't create user while logged in.\r\n" );
        respond(denied, &(*connectionID)->socket);
    }

}

//Perform the send function and respond to all relevant connections.
void TCPServer::handle_send_all(std::string message, ConnectIndex connectionID)
{
    std::map<std::string,ServerTCPConnection*>::iterator iter = userloginStatus.begin();

    bool userLoggedin = false;
    //Also get the userID to send back.
    std::string userID = "";

    while(iter != userloginStatus.end())
    {
        if(iter->second != nullptr && (*connectionID)->socket.remote_endpoint() == iter->second->socket.remote_endpoint())
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

        
        //boost::system::error_code ignored_error;

        for(int iter = 0; iter < MAX_CLIENTS; iter++)
        {
            if(server_connections[iter].get() != (*connectionID).get())
            {
                std::string yell(userID + ": " + message + "\r\n" );
                //Check if there is an empty spot.
                ServerTCPConnection * connectionRef = server_connections[iter].get();

                if(connectionRef != nullptr)
                {
                    respond(yell, &connectionRef->socket);
                }
            }
            else 
            {
                std::string empty(">\r\n" );
                //Check if there is an empty spot.
                ServerTCPConnection * connectionRef = server_connections[iter].get();

                if(connectionRef != nullptr)
                {
                    respond(empty, &connectionRef->socket);
                }
            }
            
        }
    }
    else
    {
        std::string denied( ">Denied. Please login first.\r\n" );
        respond(denied, &(*connectionID)->socket);
    }

}

//Perform the send_user function and respond to both relevant users.
void TCPServer::handle_send_user(std::string userDst, std::string message, ConnectIndex connectionID)
{
    std::map<std::string,ServerTCPConnection*>::iterator iter = userloginStatus.begin();

    bool userLoggedin = false;
    //Also get the userID to send back.
    std::string userID = "";

    ServerTCPConnection * connectionRef = nullptr;

    //Find the user logged in.
    while(iter != userloginStatus.end())
    {
        if(iter->second != nullptr && (*connectionID)->socket.remote_endpoint() == iter->second->socket.remote_endpoint())
        {
            userLoggedin = true;
            userID = iter->first;
            connectionRef = iter->second;
            break;
        }
        iter++;
    }
    
    //Get the connection of the target of the message in the logged in users map.
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

            std::string dstMsg( userID + ": " + message + "\r\n" );
            respond(dstMsg,&dstRef->socket);

            //Respond to client that made request.
            std::string srcMsg( "\r\n" );
            respond(srcMsg, &(*connectionID)->socket);
        }
        else
        {
            std::string messageForClient( ">User requested not found!\r\n" );
            respond(messageForClient,&(*connectionID)->socket);
        }
        
    }
    else
    {
        std::string messageForClient( ">Denied. Please login first.\r\n" );
        respond(messageForClient, &(*connectionID)->socket);
    }

}

//Search the userloginStatus map for the non null connection objects
void TCPServer::handle_who(ConnectIndex connectionID)
{
    std::map<std::string,ServerTCPConnection*>::iterator iter = userloginStatus.begin();

    //Hold every username in this.
    std::string message = "";

    while(iter != userloginStatus.end())
    {
        if(iter->second != nullptr )
        {
            //Add name if connection not null.
            message += iter->first + ", ";
        }
        iter++;
    }

    //Get rid of the comma and space at the end.
    message.pop_back();
    message.pop_back();

    //Respond to the client with logged in users.
    std::string forClient( ">" + message + "\r\n" );
    respond(forClient, &(*connectionID)->socket);
}


//Perform the logout function and respond to the connection that made it.
void TCPServer::handle_logout(ConnectIndex connectionID)
{
    //First figure out if the connection is logged in and record the name if it is.
    std::map<std::string,ServerTCPConnection*>::iterator iter = userloginStatus.begin();
    bool userWasLoggedIn = false;
    std::string name = "";

    while(iter != userloginStatus.end())
    {
        if(iter->second != nullptr && (*connectionID)->socket.remote_endpoint() == iter->second->socket.remote_endpoint())
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
        
        //boost::system::error_code ignored_error;
        //boost::asio::write( server_connections[connectionID]->socket, boost::asio::buffer( *buff ), ignored_error );
        for(int iter = 0; iter < MAX_CLIENTS; iter++)
        {
            if(server_connections[iter].get() != (*connectionID).get())
            {
                std::string message( name + " left.\r\n" );
                //Check if there is an empty spot.
                ServerTCPConnection * connectionRef = server_connections[iter].get();

                if(connectionRef != nullptr)
                {
                    respond(message,&connectionRef->socket);
                }
            }
            else 
            {
                std::string message( ">\r\n" );
                //Check if there is an empty spot.
                ServerTCPConnection * connectionRef = server_connections[iter].get();

                if(connectionRef != nullptr)
                {
                    respond(message, &connectionRef->socket);
                }
                
            }
            
        }
    }
    else
    {
        std::string message( ">Denied. Please login first.\r\n" );
        respond(message, &(*connectionID)->socket );
    }
}

void TCPServer::respond(std::string message, boost::asio::ip::tcp::socket *clientSocket)
{
    auto buff = std::make_shared<std::string>(message);
    boost::system::error_code ignored;
    boost::asio::write(*clientSocket, boost::asio::buffer(*buff),ignored);
}


void TCPServer::run()
{
    server_ioservice.run();
}