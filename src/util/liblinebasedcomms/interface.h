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
        /// basic interface class for all the derived serial (and networking mimics) line-based nodes (serial, tcp, udp, etc.)
        class LineBasedInterface
        {
          public:
            void start();

            std::string readline(unsigned clientkey);            

            void write(const std::string& msg);

            void close();
            
            std::string readline_oldest(unsigned clientkey);
            std::string readline_newest(unsigned clientkey);            
            
            unsigned add_user();
            void remove_user(unsigned clientkey);

            bool active() { return active_; }

          protected:            
            LineBasedInterface();
            
            // all implementors of this line based interface must provide do_start, do_write, do_close, and put all read data into "in_"
            virtual void do_start() = 0;
            virtual void do_write(const std::string & line) = 0;
            virtual void do_close(const asio::error_code& error) = 0;            

            void set_active(bool active) { active_ = active; }
            
            asio::io_service io_service_; // the main IO service that runs this connection
            std::vector< std::deque<std::string> > in_; // buffered read data
            boost::mutex in_mutex_;
            boost::posix_time::ptime last_start_time_;

          private:
            asio::io_service::work work_;
            bool active_; // remains true while this object is still operating
            
        };        

    }
}

#endif

