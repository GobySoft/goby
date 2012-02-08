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
#include <sstream>
#include <limits>
#include <iomanip>

#include <boost/assign.hpp>
#include <boost/foreach.hpp>
#include <boost/bind.hpp>
#include <boost/date_time.hpp>

//#ifdef HAS_NCURSES
#include "flex_ncurses.h"
//#endif

#include "flex_ostreambuf.h"
#include "goby/util/sci.h"
#include "goby/common/time.h"
#include "goby/common/exception.h"
#include "logger_manipulators.h"

using goby::common::goby_time;


boost::mutex curses_mutex;

boost::mutex goby::common::logger::mutex;

goby::common::FlexOStreamBuf::FlexOStreamBuf(): name_("no name"),
                                              die_flag_(false),
                                              current_verbosity_(logger::VERBOSE),
#ifdef HAS_NCURSES
                                              curses_(0),
#endif
                                              start_time_(goby_time()),
                                              is_gui_(false),
                                              highest_verbosity_(logger::QUIET)
                                              
{
    Group no_group("", "Ungrouped messages");
    groups_[""] = no_group;    
}

goby::common::FlexOStreamBuf::~FlexOStreamBuf()
{
#ifdef HAS_NCURSES    
    if(curses_) delete curses_;
#endif
}

void goby::common::FlexOStreamBuf::add_stream(logger::Verbosity verbosity, std::ostream* os)
{
    //check that this stream doesn't exist
    // if so, update its verbosity and return
    bool stream_exists = false;
    BOOST_FOREACH(StreamConfig& sc, streams_)
    {
        if(sc.os() == os)
        {
            sc.set_verbosity(verbosity);
            stream_exists = true;
        }
    }

    if(!stream_exists)
        streams_.push_back(StreamConfig(os, verbosity));

    if(verbosity > highest_verbosity_)
        highest_verbosity_ = verbosity;
}

void goby::common::FlexOStreamBuf::enable_gui()
{
#ifdef HAS_NCURSES    
    is_gui_ = true;
    curses_ = new FlexNCurses;
    
    boost::mutex::scoped_lock lock(curses_mutex);
    
    curses_->startup();
    
    // add any groups already on the screen as ncurses windows
    typedef std::pair<std::string, Group> P;
    BOOST_FOREACH(const P&p, groups_)
        curses_->add_win(&groups_[p.first]);
    
    curses_->recalculate_win();
    
    input_thread_ = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&FlexNCurses::run_input, curses_)));
#else
    throw(goby::Exception("Tried to enable NCurses GUI without compiling against NCurses. Install NCurses and recompile goby or disable GUI functionality"));
#endif
}



void goby::common::FlexOStreamBuf::add_group(const std::string & name, Group g)
{
//    if(groups_.count(name)) return;

    groups_[name] = g;
    
#ifdef HAS_NCURSES    
    if(is_gui_)
    {
        boost::mutex::scoped_lock lock(curses_mutex);
        curses_->add_win(&groups_[name]);
    }
#endif
}


// called when flush() or std::endl
int goby::common::FlexOStreamBuf::sync()
{
    std::istream is(this);
    std::string s;

    while (!getline(is, s).eof())
        display(s);
    
    group_name_.erase();

    current_verbosity_ = logger::VERBOSE;
    
    if(die_flag_) exit(EXIT_FAILURE);
    return 0;
}

void goby::common::FlexOStreamBuf::display(std::string & s)
{

    bool gui_displayed = false;
    BOOST_FOREACH(const StreamConfig& cfg, streams_)
    {
        if((cfg.os() == &std::cout || cfg.os() == &std::cerr || cfg.os() == &std::clog) && current_verbosity_ <= cfg.verbosity())
        {
#ifdef HAS_NCURSES    
            if(is_gui_ && current_verbosity_ <= cfg.verbosity() && !gui_displayed)
            {
                if(!die_flag_)
                {
                    boost::mutex::scoped_lock lock(curses_mutex);
                    std::stringstream line;
                    boost::posix_time::time_duration time_of_day = goby_time().time_of_day();
                    line << "\n"
                         << std::setfill('0') << std::setw(2) << time_of_day.hours() << ":"  
                         << std::setw(2) << time_of_day.minutes() << ":" 
                         << std::setw(2) << time_of_day.seconds() <<
                        TermColor::esc_code_from_col(groups_[group_name_].color()) << " | "  << esc_nocolor
                         <<  s;
                    
                    curses_->insert(goby_time(), line.str(), &groups_[group_name_]);
                }
                else
                {
                    curses_->alive(false);
                    input_thread_->join();
                    curses_->cleanup();
                    std::cerr << TermColor::esc_code_from_col(groups_[group_name_].color()) << name_ << esc_nocolor << ": " << s << esc_nocolor << std::endl;
                }
                gui_displayed = true;
                continue;
            }
#endif            
            
            *cfg.os() <<
                TermColor::esc_code_from_col(groups_[group_name_].color())
                      << name_ << esc_nocolor << " ["
                      << goby::common::goby_time_as_string() << "]";
            if(!group_name_.empty())
                *cfg.os() << " " << "{" << group_name_ << "}";
            *cfg.os() << ": " << s <<  std::endl;
        }
        else if(cfg.os() && current_verbosity_ <= cfg.verbosity())
        {
            basic_log_header(*cfg.os(), group_name_);
            strip_escapes(s);
            *cfg.os() << s << std::endl;
        }
    }
}

void goby::common::FlexOStreamBuf::refresh()
{
#ifdef HAS_NCURSES
    if(is_gui_)
    {
        boost::mutex::scoped_lock lock(curses_mutex);
        curses_->recalculate_win();
    }
#endif
}

// clean out any escape codes for non terminal streams
void goby::common::FlexOStreamBuf::strip_escapes(std::string& s)
{
    static const std::string esc = "\33[";
    static const std::string m = "m";
    
    size_t esc_pos, m_pos;
    while((esc_pos = s.find(esc)) != std::string::npos
          && (m_pos = s.find(m, esc_pos)) != std::string::npos)
        s.erase(esc_pos, m_pos-esc_pos+1);
    
}

