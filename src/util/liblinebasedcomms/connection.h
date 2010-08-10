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

#ifndef ASIOLineBasedConnection20100715H
#define ASIOLineBasedConnection20100715H

namespace goby
{
    namespace util
    {

// template for type of client socket (asio::serial_port, asio::ip::tcp::socket)
        template<typename ASIOAsyncReadStream>
            class LineBasedConnection
        {
          public:
            void set_delimiter(const std::string& s) { delimiter_ = s; }
            std::string delimiter() const { return delimiter_; }
            
            
          protected:
          LineBasedConnection(ASIOAsyncReadStream& socket,
                              std::deque<std::string>& out,
                              std::deque<std::string>& in,
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
                                 boost::bind(&LineBasedConnection::read_complete,
                                             this,
                                             asio::placeholders::error));
            }
            
            void write_start()
            { // Start an asynchronous write and call write_complete when it completes or fails
                
                asio::async_write(socket_,
                                  asio::buffer(out_.front()),
                                  boost::bind(&LineBasedConnection::write_complete,
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
                    
                    in_.push_back(line);
                    
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
            ASIOAsyncReadStream& socket_;
            std::deque<std::string>& out_; // buffered write data
            std::deque<std::string>& in_; // buffered read data
            boost::mutex& in_mutex_;
            
            asio::streambuf buffer_; // streambuf to store serial data in for use by program
            std::string delimiter_;
            std::string extra_;
        };
    }
}

#endif
