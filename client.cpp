#include <boost/chrono/duration.hpp>
#include <iostream>
#include <boost/asio.hpp>
#include <boost/array.hpp>
#include <boost/chrono.hpp>
#include <boost/thread/thread.hpp> 
#include <string>
#include <memory>
#include "include/Client/TCPClient.hpp"

int main()
{
    std::cout << "My chat room client. Version one." << std::endl;

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

    std::string userInput = "";

    while(userInput != "exit")
    {
        std::cout << ">";
        std::getline(std::cin,userInput);
        std::cout << std::endl;

        clientRef->parse_user_message(userInput);
        //boost::this_thread::sleep_for(boost::chrono::milliseconds(1000));
        
    }


    return 0;
}