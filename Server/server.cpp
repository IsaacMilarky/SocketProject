#include <iostream>
#include <map>
#include <string>
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

std::map<std::string,std::string> parseUserFile(std::string);



int main()
{

    std::cout << "My chat room server. Version Two.\n" << std::endl;


    
    //Make sure that we can create an endpoint bind and catch if we can't
    try
    {
        //Set up and bind an endpoint into an async connection acceptor on this port.
        //Get a reference to srv
        TCPServer srv(SERVER_PORT);

        srv.loadExistingUsers(parseUserFile("../users.txt"));

        //Activate boost's I/O object to start running event listeners asynchronously.
        srv.run();
    }
    catch(const std::exception &e)
    {
        std::cerr << "Localhost with port " << SERVER_PORT << " not available for bind and listen!" << std::endl;
        return -1;
    }   


    
    
    return 0;
}

std::map<std::string,std::string> parseUserFile(std::string filePath)
{
    std::map<std::string,std::string> usersWithPasswords;

    //Get username password pairs from file
    std::ifstream usertext(filePath);

    if(usertext.is_open())
    {
        std::string line;
        
        //Parse each line of username password file
        while(std::getline(usertext,line))
        {
            const char delim = ',';
            std::string token;

            //Put file's line in string stream and get characters before and after ','
            std::stringstream userLine(line);
            std::getline(userLine,token,delim);

            std::string user = token;
            //Remove the characters not part of user or password ' ' or '(',')'
            user.erase(remove(user.begin(), user.end(), ' '), user.end());
            user.erase(remove(user.begin(), user.end(), '('), user.end());
            user.erase(remove(user.begin(), user.end(), ')'), user.end());

            //Same idea but get text after , for the password and remove parenthesis and spaces
            std::getline(userLine,token,delim);

            std::string password = token;

            password.erase(password.length() - 1);

            password.erase(remove(password.begin(), password.end(), ' '), password.end());
            password.erase(remove(password.begin(), password.end(), '('), password.end());
            password.erase(remove(password.begin(), password.end(), ')'), password.end());

            //Record username password pair and that they haven't connected yet.
            usersWithPasswords[user] = password;
            //userloginStatus[user] = nullptr;
        }


        usertext.close();

    }

    return usersWithPasswords;
}