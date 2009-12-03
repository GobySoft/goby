// t. schneider tes@mit.edu 11.10.09
// ocean engineering graduate student - mit / whoi joint program
// massachusetts institute of technology (mit)
// laboratory for autonomous marine sensing systems (lamss)
// 
// this is flex_ostreambuf.h, part of FlexCout
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

#ifndef FlexOStreamBuf20091110H
#define FlexOStreamBuf20091110H

#include <iostream>
#include <sstream>

#include <boost/thread.hpp>

#include "flex_cout_group.h"
#include "flex_ncurses.h"
#include "term_color.h"

// stringbuf that allows us to insert things before the stream and control output
class FlexOStreamBuf : public std::stringbuf
{
  public:
    FlexOStreamBuf();
        
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

    termcolor::TermColor color_;
    
    FlexNCurses curses_;

    boost::mutex curses_mutex_;
    
    //    boost::thread input_thread_;
};



#endif
