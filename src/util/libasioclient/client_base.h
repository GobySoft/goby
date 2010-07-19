// copyright 2009, 2010 t. schneider tes@mit.edu
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

#ifndef ClientBase20100628H
#define ClientBase20100628H

#include "base.h"

namespace goby
{
    namespace util
    {
        
        // template for type of client socket (asio::serial_port, asio::ip::tcp::socket)
        template<typename AsyncReadStream>
            class ASIOStreamClient : public ASIOLineBasedInterface, public ASIOAsyncConnection<AsyncReadStream>
        {
          protected:
            enum { RETRY_INTERVAL = 10 };
            
          ASIOStreamClient(AsyncReadStream& socket,
                           const std::string& delimiter)
              : ASIOLineBasedInterface(),
                ASIOAsyncConnection<AsyncReadStream>(socket, out_, in_, in_mutex_, delimiter),
                socket_(socket)
                { }

            // from ASIOLineBasedInterface
            void do_start()
            {
                set_active(start_specific());

                if(active())
                    ASIOAsyncConnection<AsyncReadStream>::read_start();
            }
            
            virtual bool start_specific() = 0;

            // from ASIOLineBasedInterface
            void do_write(const std::string & line)
            { 
                bool write_in_progress = !out_.empty();
                out_.push_back(line);
                if (!write_in_progress) ASIOAsyncConnection<AsyncReadStream>::write_start();
            }

            // from ASIOLineBasedInterface
            void do_close(const asio::error_code& error)
            { 
                if (error == asio::error::operation_aborted) // if this call is the result of a timer cancel()
                    return; // ignore it because the connection cancelled the timer

                set_active(false);
                socket_.close();
                
                using namespace boost::posix_time;
                ptime now  = goby_time();
                if(now - seconds(RETRY_INTERVAL) < last_start_time_)
                    sleep(RETRY_INTERVAL - (now-last_start_time_).total_seconds());
                
                start();
            }
            
            void socket_close(const asio::error_code& error)
            {
                do_close(error);
            }
            
                
          private:        
            AsyncReadStream& socket_;
            std::deque<std::string> out_; // buffered write data
            
            ASIOStreamClient(const ASIOStreamClient&);
            ASIOStreamClient& operator= (const ASIOStreamClient&);
        
        }; 
    }
}

#endif
