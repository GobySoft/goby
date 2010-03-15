// copyright 2009 t. schneider tes@mit.edu
// ocean engineering graudate student - mit / whoi joint program
// massachusetts institute of technology (mit)
// laboratory for autonomous marine sensing systems (lamss)    
// 
// this file is part of goby-util, a collection of utility libraries
//
// this is an almagamation of various utilities
// i have written for miscellaneous tasks
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

#ifndef TES_UTILS20091211H
#define TES_UTILS20091211H

#include <cmath>
#include <string>
#include <sstream>
#include <vector>
#include <bitset>
#include <iomanip>
#include <iostream>

#include <boost/dynamic_bitset.hpp>

namespace tes_util
{
    //
    // SCIENCE
    //
    
    // round 'r' to 'dec' number of decimal places
// we want no upward bias so
// round 5 up if odd next to it, down if even
    inline double sci_round(double r, double dec)
    {
        double ex = pow(10, dec);
        double final = floor(r * ex);
        double s = (r * ex) - final;

        // remainder less than 0.5 or even number next to it
        if (s < 0.5 || (s==0.5 && !(static_cast<unsigned long>(final)&1)))
            return final / ex;
        else 
            return (final+1) / ex;
    }

    //
    // BINARY ENCODING
    //

    
    // converts a char (byte) array into a hex string where
// c = pointer to array of char
// s = reference to string to put char into as hex
// n = length of c
// first two hex chars in s are the 0 index in c
    inline bool char_array2hex_string(const unsigned char * c, std::string & s, const unsigned int n)
    {
        std::stringstream ss;
        for (unsigned int i=0; i<n; i++)
            ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<unsigned int>(c[i]);
        
        s = ss.str();
        return true;
    }

// turns a string of hex chars ABCDEF into a character array reading
// each byte 0xAB,0xCD, 0xEF, etc.
    inline bool hex_string2char_array(unsigned char * c, const std::string s, const unsigned int n)
    {
        for (unsigned int i = 0; i<n; i++)
        {
            std::stringstream ss;
            unsigned int in;
            ss <<  s.substr(2*i, 2);
            ss >> std::hex >> in;
            c[i] = static_cast<unsigned char>(in); 
        }
        return true;
    }

// return a string represented the binary value of 'l' for 'bits' number of bits
// reads MSB -> LSB
    inline std::string long2binary_string(unsigned long l, unsigned short bits)
    {
        char s [bits+1];
        for (unsigned int i=0; i<bits; i++)
        {
            s[bits-i-1] = (l&1) ? '1' : '0';
            l >>= 1;
        }
        s[bits] = '\0';    
        return (std::string)s;
    }
    
    inline std::string binary_string2hex_string(const std::string & bs)
    {
        std::string hs;
        unsigned int bytes = (unsigned int)(ceil(bs.length()/8.0));
        unsigned char c[bytes];
    
        for(size_t i = 0; i < bytes; ++i)
        {
            std::bitset<8> b(bs.substr(i*8,8));    
            c[i] = (char)b.to_ulong();
        }

        char_array2hex_string(c, hs, bytes);

        return hs;
    }

    inline std::string dyn_bitset2hex_string(const boost::dynamic_bitset<unsigned char>& bits, unsigned trim_to_bytes_size = 0)
    {
        std::stringstream binary;
        binary << bits;
        std::string out = tes_util::binary_string2hex_string(binary.str());

        if(trim_to_bytes_size)
            return out.substr(out.length() - trim_to_bytes_size*2);
        else
            return out;
    }
    
    // only works on whole byte string
    inline std::string hex_string2binary_string(const std::string & bs)
    {
        int bytes = bs.length()/2;
        unsigned char c[bytes];
    
        hex_string2char_array(c, bs, bytes);

        std::string hs;
    
        for (size_t i = 0; i < (size_t)bytes; i++)
            hs += long2binary_string((unsigned long)c[i], 8);

        return hs;
    }


    inline boost::dynamic_bitset<unsigned char> hex_string2dyn_bitset(const std::string & hs, unsigned bits_size = 0)
    {
        boost::dynamic_bitset<unsigned char> bits;
        std::stringstream bin_str;
        bin_str << hex_string2binary_string(hs);       
        bin_str >> bits;

        if(bits_size) bits.resize(bits_size);        
        return bits;
    }

    
    template <typename T> bool hex_string2number(const std::string & s, T & t)
    {
        std::stringstream ss;
        ss << s;
        ss >> std::hex >> t;
        return !ss.fail();        
    }

    template <typename T> bool number2hex_string(std::string & s, const T & t, unsigned int width = 2)
    {
        std::stringstream ss;
        ss << std::hex << std::setw(width) << std::setfill('0') << static_cast<unsigned int>(t);
        s  = ss.str();
        return !ss.fail();        
    }


    //
    // STRING PARSING
    //

    
    inline void stripblanks(std::string& s)
    {
        for(std::string::iterator it=s.end()-1; it!=s.begin()-1; --it)
        {    
            if(isspace(*it))
                s.erase(it);
        }
    }
    

// "explodes" a string on a delimiter into a vector of strings
    inline std::vector<std::string> explode(std::string s, char d, bool do_stripblanks)
    {
        std::vector<std::string> rs;
        std::string::size_type pos;
    
        pos = s.find(d);
        while(pos != std::string::npos)
        {
            std::string p = s.substr(0, pos);
            if(do_stripblanks)
                stripblanks(p);
            
            
            rs.push_back(p);
            
            if (pos+1 < s.length())
                s = s.substr(pos+1);
            else
                return rs;
            pos = s.find(d);
        }
        
        // last piece
        if(do_stripblanks)
            stripblanks(s);
        
        rs.push_back(s);
        
        return rs;
    }

    inline bool charicmp(char a, char b) { return(tolower(a) == tolower(b)); }
    inline bool stricmp(const std::string & s1, const std::string & s2)
    {
        return((s1.size() == s2.size()) &&
               equal(s1.begin(), s1.end(), s2.begin(), charicmp));
    }


    // find `key` in `str` and if successful put it in out
    // and return true
    inline bool val_from_string(std::string & out, const std::string & str, const std::string & key)
    {
        out.erase();
        
        std::string::size_type start_pos = str.find(std::string(key+"="));
        
        // deal with foo=bar,o=bar problem when looking for "o=" since
        // o is contained in foo
        while(!(start_pos == 0 || start_pos == std::string::npos || str[start_pos-1] == ','))
            start_pos = str.find(std::string(key+"="), start_pos+1);

        
        if(start_pos != std::string::npos)
        {
            
            std::string chopped = str.substr(start_pos);
            
            std::string::size_type equal_pos = chopped.find("=");

            if(equal_pos != std::string::npos)
            {
                std::string chopped_twice = chopped.substr(equal_pos);
                std::string::size_type comma_pos = chopped_twice.find(",");
                out = chopped_twice.substr(1, comma_pos-1);
                return true;
            }
        }

        return false;        
    }
    
    inline bool string2bool(const std::string & in)
    { return (stricmp(in, "true") || stricmp(in, "1")) ? true : false; }
    

}

#endif
