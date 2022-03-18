#include <ctime>
#include <iostream>
#include <string>
#include <memory>
#include <boost/array.hpp>
#include <boost/asio.hpp>
#include "include/TCPServer.hpp"

//My student ID # is 16244860.
#define SERVER_PORT 14860
/*
    Name: Isaac Milarsky (imfbp)
    Date: 3/15/2022
    Program Description:
        This implements a simple TCP server using Boost.asio with asynchronous socket API
        operations. 

        Accepts a max of three connections at once as per version 2 requirements.

*/

int main()
{

    std::cout << "My chat room server. Version Two.\n" << std::endl;


    //Get a reference to srv on the heap.
    std::unique_ptr<TCPServer> srv;

    //Make sure that we can create an endpoint bind and catch if we can't
    try
    {
        //Load ioservice, existing users from users.txt and the acceptor.
        srv = std::unique_ptr<TCPServer>(new TCPServer());
        
        //Set up and bind an endpoint into an async connection acceptor on this port.
        srv->listen(SERVER_PORT);
    }
    catch(const std::exception &e)
    {
        std::cerr << "Localhost with port " << SERVER_PORT << " not available for bind and listen!" << std::endl;
        return -1;
    }


    
    //Activate boost's I/O object to start running event listeners asynchronously.
    srv->run();
    return 0;
}