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
#include <stdexcept>
#include <vector>
#include <sstream>

#include "goby/util/string.h"

namespace goby
{
    namespace util
    {
        // simple exception class
        class bad_nmea_sentence : public std::runtime_error
        {
          public:
            bad_nmea_sentence(const std::string& s)
                : std::runtime_error(s)
            { }
        };
        
        
        class NMEASentence : public std::vector<std::string>
        {
          public:
            enum strategy { IGNORE, VALIDATE, REQUIRE };
            
            NMEASentence() {}
            NMEASentence(std::string s, strategy cs_strat = VALIDATE);

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
                T as(int i) const { return goby::util::as<T>(at(i)); }
            
            
            template<typename T>
                void push_back(T t)
            { std::vector<std::string>::push_back(goby::util::as<std::string>(t)); }
    
            static unsigned char checksum(const std::string& s);
        };
    }
}

// overloaded <<
inline std::ostream& operator<< (std::ostream& out, const goby::util::NMEASentence& nmea)
{ out << nmea.message(); return out; }

#endif
