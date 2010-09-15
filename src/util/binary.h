// copyright 2010 t. schneider tes@mit.edu
// ocean engineering graudate student - mit / whoi joint program
// massachusetts institute of technology (mit)
// laboratory for autonomous marine sensing systems (lamss)    
// 
// this file is part of goby-util, a collection of utility libraries
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

#ifndef BINARY20100713H
#define BINARY20100713H

#include <iomanip>
#include <cmath>
#include <sstream>

#include <boost/dynamic_bitset.hpp>
#include <bitset>

namespace goby
{
    namespace util
    {
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
            std::string out = binary_string2hex_string(binary.str());

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

        template <typename T> std::string number2hex_string(const T & t, unsigned int width = 2)
        {
            std::string s;
            number2hex_string(s,t,width);
            return s;
        }
    }
}

#endif
