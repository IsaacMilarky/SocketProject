#include <boost/chrono/duration.hpp>
#include <iostream>
#include <boost/asio.hpp>
#include <boost/array.hpp>
#include <boost/chrono.hpp>
#include <boost/thread/thread.hpp> 
#include "include/Client/TCPClient.hpp"

int main()
{
    std::cout << "My chat room client. Version one." << std::endl;

    TCPClient client("127.0.0.1",14860);

    std::string userInput = "";

    while(userInput != "logout")
    {
        std::cout << ">";
        std::cin >> userInput;
        std::cout << std::endl;

        boost::this_thread::sleep_for(boost::chrono::milliseconds(50));

        
    }

    return 0;
}