// copyright 2009 t. schneider tes@mit.edu
// 
// this file is part of flex-cout, a terminal display library
// that extends the functionality of std::cout
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

#include <iostream>
#include <limits>

#include <boost/assign.hpp>
#include <boost/foreach.hpp>
#include <boost/bind.hpp>

#include "flex_ostreambuf.h"

using namespace goby::tcolor;

goby::util::FlexOStreamBuf::FlexOStreamBuf(): name_("no name"),
                                  verbosity_(verbose),
                                  die_flag_(false),
                                  curses_(0)
{
    boost::assign::insert (verbosity_map_)
        ("quiet",quiet)
        ("terse",terse)
        ("verbose",verbose)
        ("scope",scope);

    Group no_group("", "warnings and ungrouped messages");
    groups_[""] = no_group;    
}

goby::util::FlexOStreamBuf::~FlexOStreamBuf()
{
    if(curses_) delete curses_;
}

void goby::util::FlexOStreamBuf::verbosity(const std::string & s)
{
    verbosity_ = verbosity_map_[s];
    
    if(verbosity_ == scope)
    {
        curses_ = new FlexNCurses(curses_mutex_);
        
        boost::mutex::scoped_lock lock(curses_mutex_);

        curses_->startup();
        curses_->add_win(&groups_[""]);
        
        curses_->recalculate_win();
	
        input_thread_ = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&FlexNCurses::run_input, boost::ref(curses_))));
        
    }
}

void goby::util::FlexOStreamBuf::add_group(const std::string & name, Group g)
{
    groups_[name] = g;
    
    if(verbosity_ == scope)
    {
        boost::mutex::scoped_lock lock(curses_mutex_);
        curses_->add_win(&groups_[name]);
    }
}


// called when flush() or std::endl
int goby::util::FlexOStreamBuf::sync()
{
    std::istream is(this);
    std::string s;
    
    while (!getline(is, s).eof())
        display(s);

    if(die_flag_)
    {
        if(verbosity_ != quiet)
        {
            std::cout << "Press enter to quit." << std::endl;
        }
        char c;
        std::cin.get(c);
        exit(EXIT_FAILURE);
    }
    
    group_name_.erase();
    
    return 0;
}

void goby::util::FlexOStreamBuf::display(const std::string & s)
{
    bool is_group = groups_.count(group_name_);

    switch(verbosity_)
    {
        default:
        case verbose:
            if(is_group)
                std::cout << color_.esc_code_from_str(groups_[group_name_].color()) << name_ << nocolor << ": " << s << nocolor << std::endl;
            else
                std::cout << name_ << ": " << s << nocolor << std::endl;
            break;

        case terse:
            if (is_group)
                std::cout << color_.esc_code_from_str(groups_[group_name_].color()) << groups_[group_name_].heartbeat() << nocolor;
            else
                std::cout << "." << std::flush;
            break;

        case quiet:
            break;
            
        case scope:
            if(!die_flag_)
            {
                boost::mutex::scoped_lock lock(curses_mutex_);
                std::string line = std::string("\n" +  color_.esc_code_from_str(groups_[group_name_].color()) + "| \33[0m" + s);
                curses_->insert(time(NULL), line, &groups_[group_name_]);
            }
            else
            {
                curses_->alive(false);
                input_thread_->join();
                curses_->cleanup();
                
                std::cout << color_.esc_code_from_str(groups_[group_name_].color()) << name_ << nocolor << ": " << s << nocolor << std::endl;
            }            
                  
            break;
    }
}
