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

#include "nmea_sentence.h"

serial::NMEASentence::NMEASentence(std::string s, bool strict /*= STRICT*/)
    : message_(s),
      message_no_cs_(s),
      cs_(0),
      valid_(false),
      strict_(strict)
{
    // make message_, message_no_cs_, message_parts in sync
    update();
    
    boost::split(message_parts_, message_no_cs_, boost::is_any_of(","));
}

serial::NMEASentence::NMEASentence()
    : message_(""),
      message_no_cs_(""),
      cs_(0),
      valid_(false),
      strict_(true)
{ }

void serial::NMEASentence::add_cs(std::string& s)
{
    std::string cs;
    unsigned char c[] = {calc_cs(s)};
    tes_util::char_array2hex_string(c, cs, 1);
    // modem requires CS to be uppercase...
    boost::to_upper(cs);
    
    s += "*" + cs;
}


unsigned char serial::NMEASentence::strip_cs(std::string& s)    
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


void serial::NMEASentence::validate()
{
    if(message_.empty())
        throw std::runtime_error("NMEASentence: no message: " + message_);
    
    std::string::size_type star_pos = message_.find('*');
    if(star_pos == std::string::npos)
        throw std::runtime_error("NMEASentence: no checksum: " + message_);
    
    std::string::size_type dollar_pos = message_.find('$');
    if(dollar_pos == std::string::npos)
        throw std::runtime_error("NMEASentence: no $: " + message_);
    
    if(strict_)
    {    
        if(cs_ != given_cs_)
            throw std::runtime_error("NMEASentence: bad checksum: " + message_);
    }
    
    if(message_parts_[0].length() != 6)
        throw std::runtime_error("NMEASentence: wrong talker length: " + message_);
    
    valid_ = true;
}

unsigned char serial::NMEASentence::calc_cs(const std::string& s)
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

void serial::NMEASentence::update()
{
    boost::trim(message_);
    boost::trim(message_no_cs_);
    
    if(message_.find('*') == std::string::npos && !strict_)
        add_cs(message_);
    
    given_cs_ = strip_cs(message_no_cs_);
    
    cs_ = calc_cs(message_);
}

void serial::NMEASentence::parts_to_message()
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

bool icmp_contents(serial::NMEASentence& n1, serial::NMEASentence& n2)
{
    if(n1.message_no_cs().length() < 7 || n2.message_no_cs().length() < 7)
        return false;
    else
        return tes_util::stricmp(n1.message_no_cs().substr(7), n2.message_no_cs().substr(7));
}    
    

