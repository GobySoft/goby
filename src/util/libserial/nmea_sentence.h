// copyright 2009 t. schneider tes@mit.edu
//           2010 c. murphy    cmurphy@whoi.edu
// 
// this file is part of serial, a library for handling serial
// communications.
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

#ifndef NMEASentence20091211H
#define NMEASentence20091211H

#include <exception>
#include <vector>
#include <cstdio>

#include <boost/algorithm/string.hpp>
#include <boost/assign.hpp>
#include <boost/lexical_cast.hpp>

#include "util/tes_utils.h"

namespace serial
{    
    class NMEASentence : public std::vector<std::string>
    {
      public:
        enum strategy { IGNORE, VALIDATE, REQUIRE };
    
        NMEASentence(std::string s, strategy cs_strat);

        NMEASentence() : std::vector<std::string>() {}

        // Bare message, no checksum or \r\n
        std::string message_no_cs() const;

        // Includes checksum, but no \r\n
        std::string message() const;

        // Includes checksum and \r\n
        std::string message_cr_nl() const { return message() + "\r\n"; }

        // first two talker (CC)
        std::string talker_id() const 
        { return empty() ? "" : front().substr(1, 2); }

        // last three (CFG)
        std::string sentence_id() const
        { return empty() ? "" : front().substr(3); }

        template<typename T>
            T as(int i) { return boost::lexical_cast<T>(at(i)); }
    
        template<typename T>
            void push_back(T t)
        { std::vector<std::string>::push_back(boost::lexical_cast<std::string>(t)); }
    
        static unsigned char checksum(const std::string& s);
    };
}
// overloaded <<
inline std::ostream& operator<< (std::ostream& out, const serial::NMEASentence& nmea)
{ out << nmea.message(); return out; }

#endif
