// copyright 2010 t. schneider tes@mit.edu
//
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


#ifndef ASIOLineBasedInterface20100715H
#define ASIOLineBasedInterface20100715H

#include <iostream>
#include <deque>
#include <fstream>
#include <boost/lexical_cast.hpp>
#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <boost/array.hpp>
#include <asio.hpp>

#include "goby/util/time.h"

#include <string>

namespace goby
{
    namespace util
    {
        /// basic interface class for all the derived ASIO line-based nodes (serial, tcp, udp, etc.)
        class ASIOLineBasedInterface
        {
          public:
            void start()
            {
                if(active_) return;
                
                last_start_time_ = goby_time();
                io_service_.post(boost::bind(&ASIOLineBasedInterface::do_start, this));
            }        
            
            void write(const std::string& msg) // pass the write data to the do_write function via the io service in the other thread
            { io_service_.post(boost::bind(&ASIOLineBasedInterface::do_write, this, msg)); }

            void close() // call the do_close function via the io service in the other thread
            { io_service_.post(boost::bind(&ASIOLineBasedInterface::do_close, this, asio::error_code())); }            

            std::string readline(unsigned clientkey)
            { return readline_oldest(clientkey); }
            
            std::string readline_oldest(unsigned clientkey)
            {
                boost::mutex::scoped_lock lock(in_mutex_);
                if(in_.at(clientkey).empty()) return "";
                else
                {
                    std::string in = in_.at(clientkey).front();
                    in_.at(clientkey).pop_front();
                    return in;
                }
            }
            
            std::string readline_newest(unsigned clientkey)
            {
                boost::mutex::scoped_lock lock(in_mutex_);
                if(in_.at(clientkey).empty()) return "";
                else
                {
                    std::string in = in_.at(clientkey).back();
                    in_.at(clientkey).pop_back();
                    return in;
                }
            }
            
            bool active()
            { return active_; }
            
            unsigned add_user()
            {
                boost::mutex::scoped_lock lock(in_mutex_);                        
                in_.push_back(std::deque<std::string>());
                return in_.size()-1;
            }

            void release_user(unsigned clientkey)
            {
                boost::mutex::scoped_lock lock(in_mutex_);                        
                in_.erase(in_.begin() + clientkey);
            }            

          protected:            
          ASIOLineBasedInterface()
              : work_(io_service_),
                active_(false)
            {
                in_.push_back(std::deque<std::string>());
                boost::thread t(boost::bind(&asio::io_service::run, &io_service_));
            }
            
            virtual void do_start() = 0;
            virtual void do_write(const std::string & line) = 0;
            virtual void do_close(const asio::error_code& error) = 0;            

            void set_active(bool active) 
            { active_ = active; }
            
            asio::io_service io_service_; // the main IO service that runs this connection
            std::vector< std::deque<std::string> > in_; // buffered read data
            boost::mutex in_mutex_;
            boost::posix_time::ptime last_start_time_;

          private:
            asio::io_service::work work_;
            bool active_; // remains true while this object is still operating
            
        };        

        // template for type of client socket (asio::serial_port, asio::ip::tcp::socket)
        template<typename AsyncReadStream>
            class ASIOAsyncConnection
        {
          protected:
          ASIOAsyncConnection(AsyncReadStream& socket,
                              std::deque<std::string>& out,
                              std::vector< std::deque<std::string> >& in,
                              boost::mutex& in_mutex,
                              const std::string& delimiter)
              : socket_(socket),
                out_(out),
                in_(in),
                in_mutex_(in_mutex),
                delimiter_(delimiter)
                { }
            
            void read_start()
            {
                async_read_until(socket_, buffer_, delimiter_,
                                 boost::bind(&ASIOAsyncConnection::read_complete,
                                             this,
                                             asio::placeholders::error));
            }
            
            void write_start()
            { // Start an asynchronous write and call write_complete when it completes or fails
                
                asio::async_write(socket_,
                                  asio::buffer(out_.front()),
                                  boost::bind(&ASIOAsyncConnection::write_complete,
                                              this,
                                              asio::placeholders::error));
            }
            
            void read_complete(const asio::error_code& error)
            {     
                if(error) return socket_close(error);

                std::istream is(&buffer_);
                std::string line;
                
                char last = delimiter_.at(delimiter_.length()-1);
                while(!std::getline(is, line, last).eof())
                {
                    line = extra_ + line + last;
                    // grab a lock on the in_ deque because the user can modify    
                    boost::mutex::scoped_lock lock(in_mutex_);
                    
                    // add for all the users
                    for(std::vector< std::deque<std::string> >::iterator it = in_.begin(), n = in_.end(); it != n; ++it)
                        it->push_back(line);
                    
                    extra_.clear();
                }
                
                // store any remainder for the next round
                if(!line.empty()) extra_ = line;
                
                read_start(); // start waiting for another asynchronous read again
            }    

            void write_complete(const asio::error_code& error)
            { // the asynchronous read operation has now completed or failed and returned an error
                if(error) return socket_close(error);
                
                out_.pop_front(); // remove the completed data
                if (!out_.empty()) // if there is anthing left to be written
                    write_start(); // then start sending the next item in the buffer
            }

            virtual void socket_close(const asio::error_code& error) = 0;

          private:
            AsyncReadStream& socket_;
            std::deque<std::string>& out_; // buffered write data
            std::vector< std::deque<std::string> >& in_; // buffered read data
            boost::mutex& in_mutex_;
            
            asio::streambuf buffer_; // streambuf to store serial data in for use by program
            std::string delimiter_;
            std::string extra_;
        };
    }
}

#endif

