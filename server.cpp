#include <ctime>
#include <iostream>
#include <string>
#include <boost/array.hpp>
#include <boost/asio.hpp>
#include "include/TCPServer.hpp"


int main()
{
    auto srv = TCPServer();
    srv.listen(12345);

    srv.run();
    return 0;
}