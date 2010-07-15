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

void goby::util::FlexOstream::name(const std::string& s)
{ sb_.name(s); }

void goby::util::FlexOstream::group(const std::string& s)
{ sb_.group_name(s); }

void goby::util::FlexOstream::verbosity(const std::string& s)
{ sb_.verbosity(s); }
    
void goby::util::FlexOstream::die_flag(bool b)
{ sb_.die_flag(b); }

    
void goby::util::FlexOstream::add_group(const std::string& name,
                         const std::string& heartbeat /* = "." */,
                         const std::string& color /* = "nocolor" */,
                         const std::string& description /* = "" */)
{
    Group ng(name, description, color, heartbeat);
    sb_.add_group(name, ng);

    if(!sb_.is_scope())
        std::cout << "Adding FlexOstream group: " << sb_.color(color) << name << sb_.color("nocolor")
                  << " (" << description << ")" 
                  << " [heartbeat: " << sb_.color(color) << heartbeat << sb_.color("nocolor")
                  << " ]" << std::endl;
}

std::ostream& goby::util::FlexOstream::operator<<(std::ostream& (*pf) (std::ostream&))
{
    if(pf == die) die_flag(true);
    return (quiet()) ? *this : std::ostream::operator<<(pf);
}
