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

// credit for much of this goes to Jeff Gray-3 (http://www.nabble.com/Simple-serial-port-demonstration-with-boost-asio-asynchronous-I-O-p19849520.html)

#ifndef SerialClient20091211H
#define SerialClient20091211H

#include <deque>

#include "asio.hpp"
#include <boost/bind.hpp> 

namespace boost { class mutex; }

namespace serial
{    
    class SerialClient
    {
      public:
        SerialClient(asio::io_service& io_service,
                     std::deque<std::string>& in,
                     boost::mutex& in_mutex,
                     std::string delimiter = "\r\n");
        
        void start(const std::string& name, unsigned int baud);
        
        
        void write(const std::string& msg) // pass the write data to the do_write function via the io service in the other thread
        { io_service_.post(boost::bind(&SerialClient::do_write, this, msg)); }
        
        void close() // call the do_close function via the io service in the other thread
        { io_service_.post(boost::bind(&SerialClient::do_close, this, asio::error_code())); }
        
        bool active() // return true if the socket is still active
        { return active_; }
        
      private:
        
        void read_start(void);
        void read_complete(const asio::error_code& error, size_t bytes_transferred);
        void do_write(const std::string& line);
        void write_start();
        void write_complete(const asio::error_code& error);
        void do_close(const asio::error_code& error);
        
  
      private:
        bool active_; // remains true while this object is still operating
        asio::io_service& io_service_; // the main IO service that runs this connection
        asio::serial_port serial_port_; // the serial port this instance is connected to
        asio::streambuf buffer_; // streambuf to store serial data in for use by program
        std::deque<std::string> out_; // buffered write data 
        std::deque<std::string>& in_; // buffered read data
        boost::mutex& in_mutex_;    
        std::string delimiter_;    
    
    }; 
}

#endif
