#include <boost/chrono/duration.hpp>
#include <iostream>
#include <boost/asio.hpp>
#include <boost/array.hpp>
#include <boost/chrono.hpp>
#include <boost/thread/thread.hpp> 
#include <string>
#include <memory>
#include "include/TCPClient.hpp"

/*
    Name: Isaac Milarsky (imfbp)
    Date: 3/16/2022
    Program Description:
        This implements a simple asynchronous TCP client in Boost.asio.
        Connection to server is established at program start.

        Boost.asio is used on standard input file descriptors as well as sockets
        for the client.

        Initally an async connection listener is set up that sends connect packet to server.

        The server sends back the accept which activates an asyncrhonous handler that in turn sets up
        two asynchronous handlers for any server messages (Like a send user message) and for reading
        from the user's standard input.

        Replicates earlier functionality but can listen at the same time it can write to server.

*/


int main()
{
    std::cout << "My chat room client. Version Two." << std::endl;

    std::cout << ">";

    //Get heap pointer to client to catch any exceptions in the constructor.
    std::unique_ptr<TCPClient> clientRef;

    try
    {
        //TCPClient client("127.0.0.1",14860);
        clientRef = std::unique_ptr<TCPClient>(new TCPClient("127.0.0.1",14860));
    }
    catch(const std::exception &e)
    {
        std::cerr << "Host not up or connection refused!" << std::endl;
        return -1;
    }

    //Add connection listener to io_service.
    clientRef->start_connect();

    //Start running async operations once listeners are set up.
    clientRef->run();

    return 0;
}