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

#include <boost/thread.hpp>
#include <boost/shared_ptr.hpp>

#include "logger_manipulators.h"

#include "flex_ncurses.h"
#include "term_color.h"

namespace goby
{
namespace logger
{    
// stringbuf that allows us to insert things before the stream and control output
class FlexOStreamBuf : public std::stringbuf
{
  public:
    FlexOStreamBuf();
    ~FlexOStreamBuf();
    
    int sync();
    
    void name(const std::string & s)
    { name_ = s; }

    void verbosity(const std::string & s);

    bool is_quiet()
    { return (verbosity_ == quiet); }    

    bool is_scope()
    { return (verbosity_ == scope); }
    
    void group_name(const std::string & s)
    { group_name_ = s; }

    void die_flag(bool b)
    { die_flag_ = b; }
    
    void add_group(const std::string& name, Group g);

    std::string color(const std::string& color)
    { return color_.esc_code_from_str(color); }

    void refresh()
    {
        if(verbosity_ == scope)
        {
            boost::mutex::scoped_lock lock(curses_mutex_);
            curses_->recalculate_win();
        }
    }
    
  private:
    void display(const std::string& s);
    
    
  private:
    std::string name_;
    std::string group_name_;
    
    std::map<std::string, Group> groups_;
    
    enum Verbosity { verbose = 0, quiet, terse, scope };
    
    Verbosity verbosity_;

    std::map<std::string, Verbosity> verbosity_map_;
        
    bool die_flag_;

    tcolor::TermColor color_;
    
    FlexNCurses* curses_;

    boost::mutex curses_mutex_;
    boost::shared_ptr<boost::thread> input_thread_;
};
}
}
#endif

