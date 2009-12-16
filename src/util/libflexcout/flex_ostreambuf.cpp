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
#include <ctime>

#include <boost/assign.hpp>
#include <boost/foreach.hpp>
#include <boost/bind.hpp>

#include "flex_ostreambuf.h"

using namespace termcolor;

FlexOStreamBuf::FlexOStreamBuf(): name_("no name"),
                                  verbosity_(verbose),
                                  die_flag_(false),
                                  curses_(curses_mutex_)
{
    boost::assign::insert (verbosity_map_)
        ("quiet",quiet)
        ("terse",terse)
        ("verbose",verbose)
        ("scope",scope);

    Group no_group("", "warnings and ungrouped messages");
    groups_[""] = no_group;    
}

void FlexOStreamBuf::verbosity(const std::string & s)
{
    verbosity_ = verbosity_map_[s];
    
    if(verbosity_ == scope)
    {
        boost::mutex::scoped_lock lock(curses_mutex_);

        curses_.startup();
        curses_.add_win(&groups_[""]);
        
        // std::vector<const Group*> groups;
        // groups.resize(groups_.size());
        
        // typedef std::pair<std::string, int> P;
        // BOOST_FOREACH(const P& p, group_order_)
        //     groups[p.second] = &(groups_[p.first]);
        
//        curses_.recalculate_win(groups_.size(), groups);
        curses_.recalculate_win();
	
        boost::thread input_thread(boost::bind(&FlexNCurses::run_input, boost::ref(curses_)));
        
    }
}

void FlexOStreamBuf::add_group(const std::string & name, Group g)
{
    groups_[name] = g;
    
    if(verbosity_ == scope)
    {
        boost::mutex::scoped_lock lock(curses_mutex_);
        curses_.add_win(&groups_[name]);
    }
}


// called when flush() or std::endl
int FlexOStreamBuf::sync()
{
    std::istream is(this);
    std::string s;
    
    while (!getline(is, s).eof())
        display(s);

    if(die_flag_)
        exit(EXIT_FAILURE);
    
    group_name_.erase();
    
    return 0;
}

void FlexOStreamBuf::display(const std::string & s)
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
            boost::mutex::scoped_lock lock(curses_mutex_);
            std::string line = std::string("\n" +  color_.esc_code_from_str(groups_[group_name_].color()) + "| \33[0m" + s);
            curses_.insert(time(NULL), line, &groups_[group_name_]);
            break;
    }
}
