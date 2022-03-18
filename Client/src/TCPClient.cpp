#include "../include/TCPClient.hpp"
#include <boost/asio/placeholders.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/asio/write.hpp>
#include <memory>


//Init ioservice and take care of everything needed to start the connection.
TCPClient::TCPClient(std::string addressString, int port) : client_io_service(),
    chatConnection(client_io_service),
    userInput_(client_io_service, ::dup(STDIN_FILENO)), //Create boost file descriptor objects using dup to copy file descriptor of stdin
    userOutput_(client_io_service, ::dup(STDIN_FILENO))
{
    //Create endpoint object from address.
    target_host_endpoint = boost::asio::ip::tcp::endpoint(boost::asio::ip::address::from_string(addressString),port);
}

//Send connect packet and set up an async handler to activate when server sends response.
void TCPClient::start_connect()
{
    auto handler = boost::bind(&TCPClient::handle_connect,this,boost::asio::placeholders::error);
    chatConnection.socket.async_connect(target_host_endpoint,handler);
}

//Write > to standard output and then start listener for server and then start listener for standard input
void TCPClient::handle_connect(const boost::system::error_code & err)
{
    if(!err)
    {
        boost::system::error_code ignored_error;
        auto buff = std::make_shared<std::string>( ">" );
        //auto printHandler = boost::bind(&TCPClient::handle_write,this,boost::asio::placeholders::error);
        
        boost::asio::write(userOutput_,boost::asio::buffer(*buff),ignored_error);

        //Listen for messages from server
        start_listen();

        //Listen for text from user.
        start_get_std_input();
    }
    else {
        std::cerr << "Could not connect to host!" << std::endl;
    }
}

//Set up event handler for server sending message to client.
void TCPClient::start_listen()
{
    auto handler = boost::bind(&TCPClient::handle_server_message,this,boost::asio::placeholders::error,boost::asio::placeholders::bytes_transferred);
    boost::asio::async_read_until(chatConnection.socket,chatConnection.buffer,"\n",handler);
}

//Get a message from  the connection and print it to screen.
void TCPClient::handle_server_message(const boost::system::error_code & err, size_t bytes_transferred)
{
    if(err)
    {
        std::cerr << "Could not parse message from server!" << std::endl;

    }
    else {
        std::string line;

        //Pull data from buffer into string.
        if(chatConnection.buffer.size() > 0)
        {
            std::istream is( &chatConnection.buffer);
            std::getline(is, line);

        }
        else {
            line = "";
        }

        boost::system::error_code ignored_error;

        //Print string from buffer to standard output.
        auto buff = std::make_shared<std::string>( line + "\n" );
        boost::asio::write(userOutput_,boost::asio::buffer(*buff),ignored_error);

        //Replace prompt string for user.
        buff = std::make_shared<std::string>( ">" );
        boost::asio::write(userOutput_,boost::asio::buffer(*buff),ignored_error);
    }

    //Set up event listener for next server message.
    start_listen();
}

//Set up listener for user input
void TCPClient::start_get_std_input()
{
    auto handler = boost::bind(&TCPClient::handle_std_input,this,boost::asio::placeholders::error,boost::asio::placeholders::bytes_transferred);
    boost::asio::async_read_until(userInput_,input_buffer_,'\n',handler);
}

//Handler activated when newline is detected in standard input.
void TCPClient::handle_std_input(const boost::system::error_code & err, size_t bytes_transferred)
{
    //std::cout << "Got here" << std::endl;
    if(!err)
    {
        //Get a raw character string buffer to read data from the input_buffer
        std::unique_ptr<char[]> inputBuffer = std::unique_ptr<char[]>(new char [bytes_transferred]);

        //Read the relevant amount of bytes into a character string buffer.
        input_buffer_.sgetn(inputBuffer.get(),bytes_transferred);


        //Convert the raw character string into a regular string in a loop.
        std::string userMessage = "";

        for(int iter = 0; iter < bytes_transferred; iter++)
        {
            userMessage += inputBuffer[iter];
        }

        //Get the newline out.
        userMessage.pop_back();
        

        //Pass user message to resolve what function is desired from client.
        parse_user_message(userMessage);
    }

    //Listen for more user input.
    start_get_std_input();
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
        boost::system::error_code ignored_error;

        auto buff = std::make_shared<std::string>( "Command not recognized!\n>" );
        //auto printHandler = boost::bind(&TCPClient::handle_write,this,boost::asio::placeholders::error);
        
        boost::asio::write(userOutput_,boost::asio::buffer(*buff),ignored_error);
        return;
    }
}

//Deal with any errors that are caused by writes done by the client.
void TCPClient::handle_write(const boost::system::error_code & err)
{
    if(err)
    {
        std::cerr << "Error encountered when trying to write to server!" << std::endl;
    }
}

//Set up an async write that sends the data in the background for all functions.
    
//Tell the server to not wait for more data and accept a differant connection.
void TCPClient::terminate_connection()
{

    
    auto buff = std::make_shared<std::string>( "exit \r\n" );
    boost::system::error_code ignored_error;
    auto handler = boost::bind(&TCPClient::handle_write,this,boost::asio::placeholders::error);

    boost::asio::async_write(chatConnection.socket,boost::asio::buffer(*buff),handler);

    //boost::asio::write( chatConnection.socket, boost::asio::buffer( *buff ), ignored_error );

    client_io_service.stop();
}

//Check the arguments and then send the login request to the server.
void TCPClient::handle_login(std::vector<std::string> * argList)
{
    if(argList->size() == 2)
    {
        auto buff = std::make_shared<std::string>( "login " + argList->at(0) + " " + argList->at(1) + " \r\n" );
        boost::system::error_code ignored_error;
        auto handler = boost::bind(&TCPClient::handle_write,this,boost::asio::placeholders::error);

        boost::asio::async_write(chatConnection.socket,boost::asio::buffer(*buff),handler);
        //std::cout << wait_for_response() << std::endl;
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
            auto handler = boost::bind(&TCPClient::handle_write,this,boost::asio::placeholders::error);

            boost::asio::async_write(chatConnection.socket,boost::asio::buffer(*buff),handler);

            //std::cout << wait_for_response() << std::endl;
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
        auto buff = std::make_shared<std::string>("send" + message + " \r\n");
        boost::system::error_code ignored_error;
        auto handler = boost::bind(&TCPClient::handle_write,this,boost::asio::placeholders::error);

        boost::asio::async_write(chatConnection.socket,boost::asio::buffer(*buff),handler);

        //std::cout << wait_for_response() << std::endl;
        return;
    }

    std::cout << "Incorrect usage of send!" << std::endl;
}

//Send standard constant logout message.
void TCPClient::handle_logout()
{
    auto buff = std::make_shared<std::string>( "logout \r\n" );
    boost::system::error_code ignored_error;
    auto handler = boost::bind(&TCPClient::handle_write,this,boost::asio::placeholders::error);

    boost::asio::async_write(chatConnection.socket,boost::asio::buffer(*buff),handler);
}

//Send static message to server like above.
void TCPClient::handle_who()
{
    auto buff = std::make_shared<std::string>( "who \r\n" );
    boost::system::error_code ignored_error;
    auto handler = boost::bind(&TCPClient::handle_write,this,boost::asio::placeholders::error);

    boost::asio::async_write(chatConnection.socket,boost::asio::buffer(*buff),handler);
}


void TCPClient::run()
{
    client_io_service.run();
}