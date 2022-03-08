#include <ctime>
#include <iostream>
#include <string>
#include <boost/array.hpp>
#include <boost/asio.hpp>
#include "include/Server/TCPServer.hpp"


int main()
{
    TCPServer srv;
    srv.listen(12345);

    srv.run();
    return 0;
}