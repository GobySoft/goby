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

#ifndef FlexOstream20091211H
#define FlexOstream20091211H

#include <iostream>
#include <sstream>
#include <map>

#include "flex_ostreambuf.h"

namespace goby
{
namespace logger
{    
// ostream extended class for holding the FlexOStreamBuf
class FlexOstream : public std::ostream
{
  public:
    FlexOstream() : std::ostream(&sb_)
    {}
    
    void name(const std::string & s);
    void group(const std::string & s);
    void verbosity(const std::string & s);

    void refresh() { sb_.refresh(); }
    
    
    void die_flag(bool b);
    bool quiet()
    { return (sb_.is_quiet()); }
    
    void add_group(const std::string & name,
                   const std::string & heartbeat = ".",
                   const std::string & color = "nocolor",
                   const std::string & description = "");
    
    
    // overload this function so we can look for the die manipulator
    // and set the die_flag
    // to exit after this line
    std::ostream & operator<<(std::ostream & (*pf) (std::ostream &));    

//provide interfaces to the rest of the types
    std::ostream& operator<< (bool& val )
    { return (quiet()) ? *this : std::ostream::operator<<(val); }
    std::ostream& operator<< (const short& val )
    { return (quiet()) ? *this : std::ostream::operator<<(val); }
    std::ostream& operator<< (const unsigned short& val )
    { return (quiet()) ? *this : std::ostream::operator<<(val); }
    std::ostream& operator<< (const int& val )
    { return (quiet()) ? *this : std::ostream::operator<<(val); }
    std::ostream& operator<< (const unsigned int& val )
    { return (quiet()) ? *this : std::ostream::operator<<(val); }
    std::ostream& operator<< (const long& val )
    { return (quiet()) ? *this : std::ostream::operator<<(val); }
    std::ostream& operator<< (const unsigned long& val )
    { return (quiet()) ? *this : std::ostream::operator<<(val); }
    std::ostream& operator<< (const float& val )
    { return (quiet()) ? *this : std::ostream::operator<<(val); }
    std::ostream& operator<< (const double& val )
    { return (quiet()) ? *this : std::ostream::operator<<(val); }
    std::ostream& operator<< (const long double& val )
    { return (quiet()) ? *this : std::ostream::operator<<(val); }

    std::ostream& operator<< (std::streambuf* sb )
    { return (quiet()) ? *this : std::ostream::operator<<(sb); }
    
    std::ostream& operator<< (std::ios& ( *pf )(std::ios&))
    { return (quiet()) ? *this : std::ostream::operator<<(pf); }
    std::ostream& operator<< (std::ios_base& ( *pf )(std::ios_base&))
    { return (quiet()) ? *this : std::ostream::operator<<(pf); }

    
  private:
    FlexOStreamBuf sb_;
};
}
}

#endif
