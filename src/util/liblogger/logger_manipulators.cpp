// copyright 2009 t. schneider tes@mit.edu
// ocean engineering graudate student - mit / whoi joint program
// massachusetts institute of technology (mit)
// laboratory for autonomous marine sensing systems (lamss)
// 
// this file is part of goby-logger,
// the goby logging library
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

#include "logger_manipulators.h"
#include "flex_ostream.h"
#include "goby/util/time.h"

std::ostream & operator<< (std::ostream& os, const Group & g)
{
    os << "description: " << g.description() << std::endl;
    os << "color: " << goby::util::TermColor::str_from_col(g.color());
    return os;
}

void GroupSetter::operator()(goby::util::FlexOstream& os) const
{
    os.set_group(group_);
}

void GroupSetter::operator()(std::ostream& os) const
{
    basic_log_header(os, group_);
}

std::ostream& basic_log_header(std::ostream& os, const std::string& group_name)
{
    return os << goby::util::goby_time_as_string() << "\t"
              << std::setw(15) << group_name << "\t";
}
