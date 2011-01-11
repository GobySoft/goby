// t. schneider tes@mit.edu 07.31.08
// ocean engineering graudate student - mit / whoi joint program
// massachusetts institute of technology (mit)
// laboratory for autonomous marine sensing systems (lamss)      
// 
// this is tes_string.h
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

#include <string>
#include <cctype>
#include <iostream>
#include <vector>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <set>

#ifndef TESSTRINGH
#define TESSTRINGH

namespace tes
{    
    std::vector<std::string> explode(std::string, char, bool do_stripblanks = false);
    std::string stripblanks(std::string);
    inline bool charicmp(char a, char b) { return(tolower(a) == tolower(b)); }
    bool stricmp(const std::string & s1, const std::string & s2);
    std::string doubleToString(const double & in, int precision = 15);
    std::string intToString(const int & in);

    template <typename T> std::string boolToString(const T & in) { return (in) ? "true" : "false"; }

    template <typename T> std::string bool2string(const T & in) { return (in) ? "true" : "false"; }
    inline bool string2bool(const std::string & in) { return (tes::stricmp(in, "true") || tes::stricmp(in, "1")) ? true : false; }
    
    std::string get_extension(const std::string & s);
    std::string word_wrap(std::string s, int width, const std::string & delim);
    bool val_from_string(std::string & out, const std::string & str, const std::string & key);
}

#endif
