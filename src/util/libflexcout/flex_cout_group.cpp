// copyright 2009 t. schneider tes@mit.edu
// ocean engineering graudate student - mit / whoi joint program
// massachusetts institute of technology (mit)
// laboratory for autonomous marine sensing systems (lamss)
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

#include "flex_cout_group.h"
#include "flex_cout.h"

std::ostream & operator<< (std::ostream & os, const Group & g)
{
    os << "description: " << g.description() << std::endl;
    os << "heartbeat: " << g.heartbeat() << std::endl;
    os << "color: " << g.color();
    return os;
}

void GroupSetter::operator()(FlexCout & os) const
{ os.group(group_); }
