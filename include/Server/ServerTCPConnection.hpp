#include <ctime>
#include <iostream>
#include <string>
#include <boost/bind/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio.hpp>

/*
    Simple struct with allocator constructors to allow program to .emplace connections into list using an io_service object

    Adds buffer object to enclose socket with the data going through it.
    Created by Isaac Milarsky 3/8/2022
*/



struct ServerTCPConnection
{
    boost::asio::ip::tcp::socket socket;
    boost::asio::streambuf read_buffer;

    ServerTCPConnection(boost::asio::io_service & io_service) : socket( io_service ), read_buffer() {}
    ServerTCPConnection(boost::asio::io_service & io_service, size_t max_buffer_size ) : socket( io_service ), read_buffer(max_buffer_size) {}
};