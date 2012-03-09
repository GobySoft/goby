// Copyright 2009-2012 Toby Schneider (https://launchpad.net/~tes)
//                     Massachusetts Institute of Technology (2007-)
//                     Woods Hole Oceanographic Institution (2007-)
//                     Goby Developers Team (https://launchpad.net/~goby-dev)
// 
//
// This file is part of the Goby Underwater Autonomy Project Libraries
// ("The Goby Libraries").
//
// The Goby Libraries are free software: you can redistribute them and/or modify
// them under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// The Goby Libraries are distributed in the hope that they will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with Goby.  If not, see <http://www.gnu.org/licenses/>.


#include "logger_manipulators.h"
#include "flex_ostream.h"
#include "goby/common/time.h"

std::ostream & operator<< (std::ostream& os, const Group & g)
{
    os << "description: " << g.description() << std::endl;
    os << "color: " << goby::common::TermColor::str_from_col(g.color());
    return os;
}

void GroupSetter::operator()(goby::common::FlexOstream& os) const
{    
    os.set_group(group_);
}

void GroupSetter::operator()(std::ostream& os) const
{
    try
    {
        goby::common::FlexOstream& flex = dynamic_cast<goby::common::FlexOstream&>(os);
        flex.set_group(group_);
    }
    catch(...)
    {
        basic_log_header(os, group_);
    }
}

std::ostream& basic_log_header(std::ostream& os, const std::string& group_name)
{
    os << "[ " << goby::common::goby_time_as_string() << " ]";

    if(!group_name.empty())
        os << " " << std::setw(15) << "{" << group_name << "}";
        
    os << ": ";
    return os;
}
