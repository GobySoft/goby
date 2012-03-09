
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
