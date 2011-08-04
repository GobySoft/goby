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
#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <boost/array.hpp>
#include <boost/asio.hpp>

#include "goby/util/time.h"
#include "goby/protobuf/linebasedcomms.pb.h"

#include <string>

namespace goby
{
    namespace util
    {
        /// basic interface class for all the derived serial (and networking mimics) line-based nodes (serial, tcp, udp, etc.)
        class LineBasedInterface
        {
          public:
            LineBasedInterface(const std::string& delimiter);
            virtual ~LineBasedInterface() { }

            // start the connection
            void start();
            // close the connection cleanly
            void close();
            // is the connection alive and well?
            bool active() { return active_; }
            

            enum AccessOrder { NEWEST_FIRST, OLDEST_FIRST };

            /// \brief returns string line (including delimiter)
            ///
            /// \return true if data was read, false if no data to read
            bool readline(std::string* s, AccessOrder order = OLDEST_FIRST)
            {
                static protobuf::Datagram datagram;
                datagram.Clear();
                bool is_data = readline(&datagram, order);
                *s = datagram.data();
                return is_data;
            }
            
            
            bool readline(protobuf::Datagram* msg, AccessOrder order = OLDEST_FIRST);

            // write a line to the buffer
            void write(const std::string& s)
            {
                static protobuf::Datagram datagram;
                datagram.Clear();
                datagram.set_data(s);
                write(datagram);
            }
            
            
            void write(const protobuf::Datagram& msg);

            // empties the read buffer
            void clear();
            
            void set_delimiter(const std::string& s) { delimiter_ = s; }
            std::string delimiter() const { return delimiter_; }
            

            
          protected:            
            
            // all implementors of this line based interface must provide do_start, do_write, do_close, and put all read data into "in_"
            virtual void do_start() = 0;
            virtual void do_write(const protobuf::Datagram& line) = 0;
            virtual void do_close(const boost::system::error_code& error) = 0;            

            void set_active(bool active) { active_ = active; }
            
            static std::string delimiter_;
            static boost::asio::io_service io_service_; // the main IO service that runs this connection
            static std::deque<protobuf::Datagram> in_; // buffered read data
            static boost::mutex in_mutex_;


          private:
            
            boost::asio::io_service::work work_;
            bool active_; // remains true while this object is still operating
            
        };        

    }
}

#endif

