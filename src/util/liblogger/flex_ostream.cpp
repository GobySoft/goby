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

#include "flex_ostream.h"
#include "logger_manipulators.h"

goby::util::FlexOstream* goby::util::FlexOstream::inst_ = 0;

goby::util::FlexOstream& goby::util::glogger(logger_lock::LockAction lock_action /*= none*/)
{    
    if(!FlexOstream::inst_) FlexOstream::inst_ = new FlexOstream();
    
    if(lock_action == logger_lock::lock)
    {
        goby::util::Logger::mutex.lock(); 
    }

    return(*FlexOstream::inst_);
}

void goby::util::FlexOstream::add_group(const std::string& name,
                                        Colors::Color color /*= Colors::nocolor*/,
                                        const std::string& description /*= ""*/)
{

    if(description.empty())
    {
        Group ng(name, name, color);
        sb_.add_group(name, ng);
    }
    else
    {
        Group ng(name, description, color);
        sb_.add_group(name, ng);
    }
    
    
    *this << "Adding FlexOstream group: " << sb_.color2esc_code(color) << name
          << sb_.color2esc_code(Colors::nocolor) << " (" << description << ")" << std::endl;
}


std::ostream& goby::util::FlexOstream::operator<<(std::ostream& (*pf) (std::ostream&))
{
    if(pf == die)   sb_.set_die_flag(true);
    if(pf == debug) sb_.set_debug_flag(true);
    if(pf == warn)  sb_.set_warn_flag(true);

    return std::ostream::operator<<(pf);
}            

