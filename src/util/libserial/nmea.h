// copyright 2009 t. schneider tes@mit.edu
// ocean engineering graudate student - mit / whoi joint program
// massachusetts institute of technology (mit)
// laboratory for autonomous marine sensing systems (lamss)
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

#ifndef NMEAH
#define NMEAH

#include <exception>
#include <vector>

#include <boost/algorithm/string.hpp>
#include <boost/assign.hpp>
#include <boost/lexical_cast.hpp>

#include "tes_utils.h"

// use for calls to constructor 
const bool STRICT = true;
const bool NOT_STRICT = false;

class NMEA
{
  public:
  NMEA(std::string s, bool strict = STRICT) : message_(s),
        message_no_cs_(s),
        cs_(0),
        valid_(false),
        strict_(strict)
        {
            // make message_, message_no_cs_, message_parts in sync
            update();
                        
            boost::split(message_parts_, message_no_cs_, boost::is_any_of(","));
        }

  NMEA() : message_(""),
        message_no_cs_(""),
        cs_(0),
        valid_(false),
        strict_(true)
        { }
    
    void add_cs(std::string & s)
    {
        std::string cs;
        unsigned char c[] = {calc_cs(s)};
        tes_util::char_array2hex_string(c, cs, 1);
        // modem requires CS to be uppercase...
        boost::to_upper(cs);
            
        s += "*" + cs;
    }

    unsigned char strip_cs(std::string & s)
    {
        std::string::size_type pos = s.find('*');
            
        // make sure it's in the right place
        if(pos == (s.length()-3))
        {
            std::string cs = s.substr(pos+1, 2);
            s = s.substr(0, pos);
            unsigned char c[1] = {0};
            tes_util::hex_string2char_array(c, cs, 1);
            return c[0];
        }
        else return 0;            
    }

    // checks for problems with the NMEA sentence
    void validate()
    {
        if(message_.empty())
            throw std::runtime_error("NMEA: no message: " + message_);

        std::string::size_type star_pos = message_.find('*');
        if(star_pos == std::string::npos)
            throw std::runtime_error("NMEA: no checksum: " + message_);

        std::string::size_type dollar_pos = message_.find('$');
        if(dollar_pos == std::string::npos)
            throw std::runtime_error("NMEA: no $: " + message_);

        if(strict_)
        {    
            if(cs_ != given_cs_)
                throw std::runtime_error("NMEA: bad checksum: " + message_);
        }
    
        if(message_parts_[0].length() != 6)
            throw std::runtime_error("NMEA: wrong talker length: " + message_);
            
        valid_ = true;
    }

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
    std::vector<std::string> & parts()
    { return message_parts_; }
    
    std::string & operator[](std::vector<std::string>::size_type i) 
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
    {
        push_back(boost::lexical_cast<std::string>(t));
    }
    
    void push_back(const std::string & s)
    {
        message_parts_.push_back(s);
        parts_to_message();
    }

    
  private:

    unsigned char calc_cs(const std::string & s)
    {
        if(s.empty())
            return 0;
            
        std::string::size_type star_pos = s.find('*');
        std::string::size_type dollar_pos = s.find('$');

        if(dollar_pos == std::string::npos)
            return 0;
        if(star_pos == std::string::npos)
            star_pos = s.length();
            
        unsigned char cs = 0;
        for(std::string::size_type i = dollar_pos+1, n = star_pos; i < n; ++i)
            cs ^= s[i];
        return cs;
    }

    // recalculates cs, and sets message_, message_no_cs_
    void update()
    {
        boost::trim(message_);
        boost::trim(message_no_cs_);
            
        if(message_.find('*') == std::string::npos && !strict_)
            add_cs(message_);
            
        given_cs_ = strip_cs(message_no_cs_);
            
        cs_ = calc_cs(message_);
    }

    // takes the parts and makes a message string out of it
    void parts_to_message()
    {
        message_.clear();
        for(std::vector<std::string>::const_iterator it = message_parts_.begin(),
                n = message_parts_.end();
            it != n;
            ++it)
            message_ += *it + ",";

        // kill last ","
        message_.erase(message_.end()-1);
            
        message_no_cs_ = message_;
        update();
    }
    
    
  private:
    std::string message_;
    std::string message_no_cs_;    
    std::vector<std::string> message_parts_;
    unsigned char cs_;
    unsigned char given_cs_;
    bool valid_;
    bool strict_; // require cs on original message?

};

// overloaded <<
inline std::ostream& operator<< (std::ostream& out, const NMEA& nmea)
{ out << nmea.message(); return out; }



// compares the contents of the NMEAs without regard to case
inline bool icmp_contents(NMEA & n1, NMEA & n2)
{
    if(n1.message_no_cs().length() < 7 || n2.message_no_cs().length() < 7)
        return false;
    else
        return tes_util::stricmp(n1.message_no_cs().substr(7), n2.message_no_cs().substr(7));
}    
    


#endif

