// t. schneider tes@mit.edu 07.31.08
// ocean engineering graudate student - mit / whoi joint program
// massachusetts institute of technology (mit)
// laboratory for autonomous marine sensing systems (lamss)      
// 
// this is binary.h 
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
#ifndef BINARYH
#define BINARYH

#include <iostream>
#include <stdio.h>
#include <cctype>
#include <string>
#include <bitset>
#include <cmath>
#include <iomanip>
#include <sstream>

namespace tes
{    
    bool hex_string2char_array(unsigned char * c, const std::string s, const unsigned int n);
    bool char_array2hex_string(const unsigned char * c, std::string & s, const unsigned int n);

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

    
    std::string long2binary_string(unsigned long l, unsigned short bits);
    std::string binary_string2hex_string(const std::string & s);
    std::string hex_string2binary_string(const std::string & bs);
}


#endif
