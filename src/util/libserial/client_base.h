// copyright 2009, 2010 t. schneider tes@mit.edu
// 
// this file is part of comms, a library for handling various communications.
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

#ifndef ClientBase20100628H
#define ClientBase20100628H

#include <iostream>
#include <ctime>
#include <deque>
#include <fstream>
#include <set>

#include <boost/lexical_cast.hpp>
#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <boost/array.hpp>
#include <asio.hpp>




namespace comms
{
    // seconds to wait before trying a reconnect
    const unsigned RETRY_INTERVAL = 10;

    template<typename AsyncReadStream>
        class ClientBase
    {
      public:
        void start()
        {
            if(active_) return;
            
            if(time(0) - RETRY_INTERVAL < last_start_time_) return;
            
            last_start_time_ = time(0);
            io_service_.post(boost::bind(&ClientBase::do_start, this));
        }
        
        
        void write(const std::string & msg) // pass the write data to the do_write function via the io service in the other thread
        { io_service_.post(boost::bind(&ClientBase::do_write, this, msg)); }
      
        void close() // call the do_close function via the io service in the other thread
        { io_service_.post(boost::bind(&ClientBase::do_close, this, asio::error_code())); }
    
        bool active() // return true if the socket is still active
        { return active_; }

        void add_user(std::deque<std::string>* in) { in_.insert(in); }
        void release_user(std::deque<std::string>* in) { in_.erase(in); }

      protected:

      ClientBase(AsyncReadStream& socket,
                 std::deque<std::string>* in,
                 boost::mutex* in_mutex,
                 std::string delimiter = "\r\n")
          : socket_(socket),
            active_(false),
            work_(io_service_),
            in_mutex_(in_mutex),
            delimiter_(delimiter),
            last_start_time_(0)
            {
                in_.insert(in);
                boost::thread t(boost::bind(&asio::io_service::run, &io_service_));
            }

        void do_start()
        {
            active_ = start_specific();

            if(active_)
                read_start();
        }
        
        
        virtual bool start_specific() = 0;
        void read_start()
        {
            // Start an asynchronous read and call read_complete when it completes or fails
            async_read_until(socket_, buffer_, delimiter_,
                             boost::bind(&ClientBase::read_complete,
                                         this,
                                         asio::placeholders::error));
        }
        
        
        void read_complete(const asio::error_code& error)
        { // the asynchronous read operation has now completed or failed and returned an error
    
            if (!error)
            { // read completed, so process the data
                
                std::istream is(&buffer_);
                std::string line;
                
                char last = delimiter_.at(delimiter_.length()-1);
                while(!std::getline(is, line, last).eof())
                {
                    line = extra_ + line + last;
                    // grab a lock on the in_ deque because the user can modify    
                    boost::mutex::scoped_lock lock(*in_mutex_);
                    
                    // add for all the users
                    for(std::set< std::deque<std::string>* >::iterator it = in_.begin(), n = in_.end(); it != n; ++it)
                        (*it)->push_back(line);

                    extra_.clear();
                }

                // store any remainder for the next round
                if(!line.empty()) extra_ = line;

                read_start(); // start waiting for another asynchronous read again
            }
            else
                do_close(error);
        }
    
        void do_write(const std::string & line)
        { // callback to handle write call from outside this class
            bool write_in_progress = !out_.empty(); // is there anything currently being written?
            out_.push_back(line); // store in write buffer

            if (!write_in_progress) // if nothing is currently being written, then start
                write_start();
        }

        void write_start()
        { // Start an asynchronous write and call write_complete when it completes or fails
            asio::async_write(socket_,
                              asio::buffer(out_.front()),
                              boost::bind(&ClientBase::write_complete,
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

            active_ = false;
    
            if (error)
            {
                std::cerr << "error: " << error.message() << std::endl; // show the error message
                std::cerr << "reconnecting ..." << std::endl;
                socket_.close();
                start();
            }
        }

        
      protected:        
        AsyncReadStream& socket_;
        
        bool active_; // remains true while this object is still operating
        asio::io_service io_service_; // the main IO service that runs this connection
        asio::io_service::work work_;
        
        
        asio::streambuf buffer_; // streambuf to store serial data in for use by program
        std::deque<std::string> out_; // buffered write data 
        std::set< std::deque<std::string>* > in_; // buffered read data
        boost::mutex* in_mutex_;
        std::string delimiter_;

        time_t last_start_time_;

        std::string extra_;
        
        
      private:
        ClientBase(const ClientBase&);
        ClientBase& operator= (const ClientBase&);
        
    }; 
}

#endif
