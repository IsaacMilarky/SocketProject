#include <ctime>
#include <iostream>
#include <string>
#include <memory>
#include <boost/array.hpp>
#include <boost/asio.hpp>
#include "include/TCPServer.hpp"

/*
    Name: Isaac Milarsky (imfbp)
    Date: 3/15/2022
    Program Description:
        This implements a simple TCP server using Boost.asio with asynchronous socket API
        operations. 

*/

int main()
{

    std::cout << "My chat room server. Version Two.\n" << std::endl;


    //Get a reference to srv on the heap.
    std::unique_ptr<TCPServer> srv;

    //Make sure that we can create an endpoint bind and catch if we can't
    try
    {
        
        srv = std::unique_ptr<TCPServer>(new TCPServer());
        //My student ID # is 16244860.
        srv->listen(14860);
    }
    catch(const std::exception &e)
    {
        std::cerr << "Localhost with port " << 14860 << " not available for bind and listen!" << std::endl;
        return -1;
    }


    

    srv->run();
    return 0;
}