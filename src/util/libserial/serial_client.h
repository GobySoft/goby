// t. schneider tes@mit.edu 06.18.09
// ocean engineering graduate student - mit / whoi joint program
// massachusetts institute of technology (mit)
// laboratory for autonomous marine sensing systems (lamss)
// 
// this is SerialClient.hpp
//
// see the readme file within this directory for information
// pertaining to usage and purpose of this script.
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

#ifndef SerialClientH
#define SerialClientH

#include "asio.hpp"
#include <boost/lexical_cast.hpp>
#include <boost/thread.hpp>
#include <boost/bind.hpp> 
#include <deque>
#include <fstream>


class SerialClient
{
  public:
  SerialClient(asio::io_service& io_service,
               std::deque<std::string> & in,
               boost::mutex & in_mutex,
               std::string delimiter = "\r\n"):
    active_(false),
        io_service_(io_service),
        serial_port_(io_service),
        in_(in),
        in_mutex_(in_mutex),
        delimiter_(delimiter)
        { }

    void start(const std::string & name, unsigned int baud)
    {
        serial_port_.open(name);
        serial_port_.set_option(asio::serial_port_base::baud_rate(baud));
        read_start();
    }
    
    void write(const std::string & msg) // pass the write data to the do_write function via the io service in the other thread
    { io_service_.post(boost::bind(&SerialClient::do_write, this, msg)); }

    void close() // call the do_close function via the io service in the other thread
    { io_service_.post(boost::bind(&SerialClient::do_close, this, asio::error_code())); }
    
    bool active() // return true if the socket is still active
    { return active_; }
    
  private:
    
    void read_start(void)
    { // Start an asynchronous read and call read_complete when it completes or fails
        async_read_until(serial_port_, buffer_, delimiter_,
                         boost::bind(&SerialClient::read_complete,
                                     this,
                                     asio::placeholders::error,
                                     asio::placeholders::bytes_transferred));
    }
    
    void read_complete(const asio::error_code& error, size_t bytes_transferred)
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
    

    void do_write(const std::string & line) // give it a string
    { // callback to handle write call from outside this class
        bool write_in_progress = !out_.empty(); // is there anything currently being written?
        out_.push_back(line); // store in write buffer

        if (!write_in_progress) // if nothing is currently being written, then start
            write_start();
    }

    void write_start()
    { // Start an asynchronous write and call write_complete when it completes or fails
        asio::async_write(serial_port_,
                          asio::buffer(out_.front()),
                          boost::bind(&SerialClient::write_complete,
                                      this,
                                      asio::placeholders::error));
    }

        
    void write_complete(const asio::error_code& error)
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

    void do_close(const asio::error_code& error)
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

  private:
    bool active_; // remains true while this object is still operating
    asio::io_service& io_service_; // the main IO service that runs this connection
    asio::serial_port serial_port_; // the serial port this instance is connected to
    asio::streambuf buffer_; // streambuf to store serial data in for use by program
    std::deque<std::string> out_; // buffered write data 
    std::deque<std::string> & in_; // buffered read data
    boost::mutex & in_mutex_;    
    std::string delimiter_;
    
    
}; 

#endif
