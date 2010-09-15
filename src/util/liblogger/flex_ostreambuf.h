// copyright 2009 t. schneider tes@mit.edu
//
// this file is part of flex-ostream, a terminal display library
// that provides an ostream with both terminal display and file logging
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

#ifndef FlexOStreamBuf20091110H
#define FlexOStreamBuf20091110H

#include <iostream>
#include <sstream>
#include <vector>

#include <boost/thread.hpp>
#include <boost/shared_ptr.hpp>

#include "logger_manipulators.h"

#include "term_color.h"

namespace goby
{
    namespace util
    {

        class FlexNCurses;
        
// stringbuf that allows us to insert things before the stream and control output
        class FlexOStreamBuf : public std::stringbuf
        {
          public:
            FlexOStreamBuf();
            ~FlexOStreamBuf();
    
            int sync();
    
            void name(const std::string & s)
            { name_ = s; }
            
            void add_stream(const std::string& verbosity, std::ostream* os);            

            bool is_quiet()
            { return is_quiet_; }    

            bool is_scope()
            { return is_scope_; }    

            
            void group_name(const std::string & s)
            { group_name_ = s; }

            void die_flag(bool b)
            { die_flag_ = b; }
    
            void add_group(const std::string& name, Group g);

            std::string color(const std::string& color)
            { return color_.esc_code_from_str(color); }

            void refresh();            
    
          private:
            void display(std::string& s);    
            void strip_escapes(std::string& s);
            
          private:
            enum Verbosity { verbose = 0, quiet, terse, scope };
            std::map<std::string, Verbosity> verbosity_map_;


            class StreamConfig
            {
              public:
                StreamConfig(std::ostream* os, Verbosity verbosity)
                    : os_(os),
                    verbosity_(verbosity)
                    { }
                
                std::ostream* os() const { return os_; }
                Verbosity verbosity() const { return verbosity_; }
                
              private:
                std::ostream* os_;
                Verbosity verbosity_;
            };
            
            std::string name_;
            std::string group_name_;
    
            std::map<std::string, Group> groups_;
        
            bool die_flag_;

            tcolor::TermColor color_;
    
            FlexNCurses* curses_;
            
            boost::shared_ptr<boost::thread> input_thread_;

            boost::posix_time::ptime start_time_;

            std::vector<StreamConfig> streams_;

            bool is_quiet_;
            bool is_scope_;
            
        };
    }
}
#endif

