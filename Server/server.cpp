#include <ctime>
#include <iostream>
#include <string>
#include <memory>
#include <boost/array.hpp>
#include <boost/asio.hpp>
#include "include/TCPServer.hpp"


int main()
{

    std::cout << "My chat room server. Version One." << std::endl;

    std::unique_ptr<TCPServer> srv;

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