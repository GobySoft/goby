// t. schneider tes@mit.edu 07.31.08
// ocean engineering graudate student - mit / whoi joint program
// massachusetts institute of technology (mit)
// laboratory for autonomous marine sensing systems (lamss)      
// 
// this is binary.cpp 
//
// this is a set of helper functions for dealing with binary data
//
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


#include "binary.h"

namespace tes
{
    
// converts a char (byte) array into a hex string where
// c = pointer to array of char
// s = reference to string to put char into as hex
// n = length of c
// first two hex chars in s are the 0 index in c
    bool char_array2hex_string(const unsigned char * c, std::string & s, const unsigned int n)
    {
        std::stringstream ss;
        for (unsigned int i=0; i<n; i++)
            ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<unsigned int>(c[i]);
        
        s = ss.str();
        return true;
    }

// turns a string of hex chars ABCDEF into a character array reading
// each byte 0xAB,0xCD, 0xEF, etc.
    bool hex_string2char_array(unsigned char * c, const std::string s, const unsigned int n)
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
    std::string long2binary_string(unsigned long l, unsigned short bits)
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

    std::string binary_string2hex_string(const std::string & bs)
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

    // only works on whole byte string
    std::string hex_string2binary_string(const std::string & bs)
    {
        int bytes = bs.length()/2;
        unsigned char c[bytes];
    
        hex_string2char_array(c, bs, bytes);

        std::string hs;
    
        for (size_t i = 0; i < (size_t)bytes; i++)
            hs += long2binary_string((unsigned long)c[i], 8);

        return hs;
    }
}
