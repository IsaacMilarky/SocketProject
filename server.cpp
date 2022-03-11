#include <ctime>
#include <iostream>
#include <string>
#include <boost/array.hpp>
#include <boost/asio.hpp>
#include "include/Server/TCPServer.hpp"


int main()
{

    std::cout << "My chat room server. Version One." << std::endl;
    TCPServer srv;
    srv.listen(12345);

    srv.run();
    return 0;
}