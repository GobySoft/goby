// t. schneider tes@mit.edu 8.16.09
// ocean engineering graduate student - mit / whoi joint program
// massachusetts institute of technology (mit)
// laboratory for autonomous marine sensing systems (lamss)
// 
// this is flex_cout.cpp
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


#include "flex_cout.h"

void FlexCout::name(const std::string& s)
{ sb_.name(s); }

void FlexCout::group(const std::string& s)
{ sb_.group_name(s); }

void FlexCout::verbosity(const std::string& s)
{ sb_.verbosity(s); }
    
void FlexCout::die_flag(bool b)
{ sb_.die_flag(b); }

    
void FlexCout::add_group(const std::string& name,
                         const std::string& heartbeat /* = "." */,
                         const std::string& color /* = "nocolor" */,
                         const std::string& description /* = "" */)
{
    Group ng(name, description, color, heartbeat);
    sb_.add_group(name, ng);

    if(!sb_.is_scope())
        std::cout << "Adding FlexCout group: " << sb_.color(color) << name << sb_.color("nocolor")
                  << " (" << description << ")" 
                  << " [heartbeat: " << sb_.color(color) << heartbeat << sb_.color("nocolor")
                  << " ]" << std::endl;
}

std::ostream& FlexCout::operator<<(std::ostream& (*pf) (std::ostream&))
{
    if(pf == die) die_flag(true);
    return (quiet()) ? *this : std::ostream::operator<<(pf);
}
