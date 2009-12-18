// copyright 2009 t. schneider tes@mit.edu
// 
// this file is part of serial, a library for handling serial
// communications.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This software is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this software.  If not, see <http://www.gnu.org/licenses/>.

#include <boost/thread.hpp>

#include "serial_client.h"


serial::SerialClient::SerialClient(asio::io_service& io_service,
                                   std::deque<std::string> & in,
                                   boost::mutex & in_mutex,
                                   std::string delimiter /* = "\r\n" */)
    : active_(false),
      io_service_(io_service),
      serial_port_(io_service),
      in_(in),
      in_mutex_(in_mutex),
      delimiter_(delimiter)
{ }

void serial::SerialClient::start(const std::string & name, unsigned int baud)
{
    serial_port_.open(name);
    serial_port_.set_option(asio::serial_port_base::baud_rate(baud));
    read_start();
}


void serial::SerialClient::read_start(void)
{ // Start an asynchronous read and call read_complete when it completes or fails
    async_read_until(serial_port_, buffer_, delimiter_,
                     boost::bind(&serial::SerialClient::read_complete,
                                 this,
                                 asio::placeholders::error,
                                 asio::placeholders::bytes_transferred));
}

void serial::SerialClient::read_complete(const asio::error_code& error, size_t bytes_transferred)
{ // the asynchronous read operation has now completed or failed and returned an error
    if (!error)
    { // read completed, so process the data
        std::istream is(&buffer_);
        std::string line;
        
        char last = delimiter_.at(delimiter_.length()-1);
        std::getline(is, line, last);
        line += last;
        
        // grab a lock on the in_ deque because the user can modify
        boost::mutex::scoped_lock lock(in_mutex_);

        in_.push_back(line);

                
        read_start(); // start waiting for another asynchronous read again
    }
    else
        do_close(error);
}

void serial::SerialClient::do_write(const std::string & line) // give it a string
{ // callback to handle write call from outside this class
    bool write_in_progress = !out_.empty(); // is there anything currently being written?
    out_.push_back(line); // store in write buffer

    if (!write_in_progress) // if nothing is currently being written, then start
        write_start();
}

void serial::SerialClient::write_start()
{ // Start an asynchronous write and call write_complete when it completes or fails
    asio::async_write(serial_port_,
                      asio::buffer(out_.front()),
                      boost::bind(&serial::SerialClient::write_complete,
                                  this,
                                  asio::placeholders::error));
}

        
void serial::SerialClient::write_complete(const asio::error_code& error)
{ // the asynchronous read operation has now completed or failed and returned an error
    if (!error)
    { // write completed, so send next write data
        out_.pop_front(); // remove the completed data
        if (!out_.empty()) // if there is anthing left to be written
            write_start(); // then start sending the next item in the buffer
    }
    else
        do_close(error);
}

void serial::SerialClient::do_close(const asio::error_code& error)
{ // something has gone wrong, so close the socket & make this object inactive
    if (error == asio::error::operation_aborted) // if this call is the result of a timer cancel()
        return; // ignore it because the connection cancelled the timer
    if (error)
        std::cerr << "error: " << error.message() << std::endl; // show the error message
    else
        std::cout << "error: serial port connection did not succeed.\n";
        
    serial_port_.close();
    active_ = false;
}
