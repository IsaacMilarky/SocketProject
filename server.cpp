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

    //My student ID # is 16244860.
    srv.listen(14860);

    srv.run();
    return 0;
}