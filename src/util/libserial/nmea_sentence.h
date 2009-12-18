// copyright 2009 t. schneider tes@mit.edu
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

#include <boost/algorithm/string.hpp>
#include <boost/assign.hpp>
#include <boost/lexical_cast.hpp>

#include "util/tes_utils.h"

namespace serial
{    
    class NMEASentence
    {
      public:
        enum { STRICT = true, NOT_STRICT = false };
    
        NMEASentence(std::string s, bool strict = STRICT);    

        NMEASentence();
    
        void add_cs(std::string& s);
        unsigned char strip_cs(std::string& s);
    
        // checks for problems with the NMEA sentence
        void validate();
    
        // includes cs
        std::string message() const
        { return (valid_) ? message_ : "not_validated"; }

        // full message with \r\n
        std::string message_cr_nl() const 
        { return (valid_) ? std::string(message_ + "\r\n") : "not_validated"; }
    
        // no checksum
        std::string message_no_cs() const
        { return (valid_) ? message_no_cs_ : "not_validated"; }
    
        // talker CCCFG
        std::string talker() const
        { return (valid_) ? message_parts_[0].substr(1) : "not_validated"; }

        // first two talker CC
        std::string talker_front() const 
        { return (valid_) ? message_parts_[0].substr(1,2) : "not_validated"; }

        // last three CFG
        std::string talker_back() const
        { return (valid_) ? message_parts_[0].substr(3) : "not_validated"; }

        // all the parts
        std::vector<std::string>& parts()
        { return message_parts_; }
    
        std::string& operator[](std::vector<std::string>::size_type i) 
        { return message_parts_.at(i); }
    
        double part_as_double(std::vector<std::string>::size_type i) 
        { return boost::lexical_cast<double>(message_parts_.at(i)); }

        int part_as_int(std::vector<std::string>::size_type i) 
        { return boost::lexical_cast<int>(message_parts_.at(i)); }

    
        // all the parts
        bool empty()
        { return message_parts_.empty(); }
            
    
        template<typename T>
            void push_back(T t)
        { push_back(boost::lexical_cast<std::string>(t)); }
    
        void push_back(const std::string& s)
        {
            message_parts_.push_back(s);
            parts_to_message();
        }

    
      private:

        unsigned char calc_cs(const std::string& s);    

        // recalculates cs, and sets message_, message_no_cs_
        void update();
    
        // takes the parts and makes a message string out of it
        void parts_to_message();
    
    
    
      private:
        std::string message_;
        std::string message_no_cs_;    
        std::vector<std::string> message_parts_;
        unsigned char cs_;
        unsigned char given_cs_;
        bool valid_;
        bool strict_; // require cs on original message?

    };


}
// overloaded <<
inline std::ostream& operator<< (std::ostream& out, const serial::NMEASentence& nmea)
{ out << nmea.message(); return out; }

// compares the contents of the NMEASentences without regard to case
bool icmp_contents(serial::NMEASentence& n1, serial::NMEASentence& n2);

#endif

