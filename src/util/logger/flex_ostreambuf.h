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

#include "goby/util/protobuf/logger.pb.h"

#include "term_color.h"

class Group;

namespace goby
{
    namespace util
    {

        class FlexNCurses;

        namespace logger
        {
            extern boost::mutex mutex;
            enum Verbosity { QUIET = protobuf::GLogConfig::QUIET,
                             WARN = protobuf::GLogConfig::WARN,
                             VERBOSE = protobuf::GLogConfig::VERBOSE,
                             GUI = protobuf::GLogConfig::GUI,
                             DEBUG1 = protobuf::GLogConfig::DEBUG1,
                             DEBUG2 = protobuf::GLogConfig::DEBUG2,
                             DEBUG3 = protobuf::GLogConfig::DEBUG3,
                             DIE };
        };
        
        
        /// Class derived from std::stringbuf that allows us to insert things before the stream and control output. This is the string buffer used by goby::util::FlexOstream for the Goby Logger (glogger)
        class FlexOStreamBuf : public std::stringbuf
        {
          public:
            FlexOStreamBuf();
            ~FlexOStreamBuf();

            /// virtual inherited from std::ostream. Called when std::endl or std::flush is inserted into the stream
            int sync();

            /// name of the application being served
            void name(const std::string & s)
            { name_ = s; }

            /// add a stream to the logger
            void add_stream(logger::Verbosity verbosity, std::ostream* os);            

            /// do all attached streams have Verbosity == quiet?
            bool is_quiet()
            { return highest_verbosity_ == logger::QUIET; }    

            /// is there an attached stream with Verbosity == gui (ncurses GUI)
            bool is_gui()
            { return is_gui_; }

            logger::Verbosity highest_verbosity()
            {
                return highest_verbosity_;
            }
            
            
            /// current group name (last insertion of group("") into the stream)
            void group_name(const std::string & s)
            { group_name_ = s; }

            /// exit on error at the next call to sync()
            void set_die_flag(bool b)
            { die_flag_ = b; }

            void set_verbosity_depth(logger::Verbosity depth)
            { current_verbosity_ = depth; }

            /// add a new group
            void add_group(const std::string& name, Group g);

            /// TODO(tes): unnecessary?
            std::string color2esc_code(Colors::Color color)
            { return TermColor::esc_code_from_col(color); }

            /// refresh the display (does nothing if !is_gui())
            void refresh();
          private:
            void display(std::string& s);    
            void strip_escapes(std::string& s);
            
          private:

            class StreamConfig
            {
              public:
                StreamConfig(std::ostream* os, logger::Verbosity verbosity)
                    : os_(os),
                    verbosity_(verbosity)
                    { }
                
                void set_verbosity(logger::Verbosity verbosity) {  verbosity_ = verbosity; }

                std::ostream* os() const { return os_; }
                logger::Verbosity verbosity() const { return verbosity_; }
                
              private:
                std::ostream* os_;
                logger::Verbosity verbosity_;
            };
            
            std::string name_;
            std::string group_name_;
    
            std::map<std::string, Group> groups_;

            bool die_flag_;
            logger::Verbosity current_verbosity_;
            
#ifdef HAS_NCURSES
            FlexNCurses* curses_;
#endif            
            boost::shared_ptr<boost::thread> input_thread_;

            boost::posix_time::ptime start_time_;

            std::vector<StreamConfig> streams_;

            bool is_gui_;

            logger::Verbosity highest_verbosity_;            
        };
    }
}
#endif

