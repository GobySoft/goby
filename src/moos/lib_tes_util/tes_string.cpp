// t. schneider tes@mit.edu 10.03.08
// ocean engineering graudate student - mit / whoi joint program
// massachusetts institute of technology (mit)
// laboratory for autonomous marine sensing systems (lamss)      
// 
// this is tes_string.cpp
//
// this is a set of helper functions for dealing with strings
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

#include "tes_string.h"

using namespace std;

namespace tes {
    
// "explodes" a string on a delimiter into a vector of strings
    vector<string> explode(string s, char d, bool do_stripblanks)
    {
        vector<string> rs;
        string::size_type pos;
    
        pos = s.find(d);
        while(pos != string::npos)
        {
            string p = s.substr(0, pos);
            if(do_stripblanks)
                p = stripblanks(p);
            
            
            rs.push_back(p);
            
            if (pos+1 < s.length())
                s = s.substr(pos+1);
            else
                return rs;
            pos = s.find(d);
        }
        
        // last piece
        if(do_stripblanks)
            s = stripblanks(s);
        
        rs.push_back(s);
        
        return rs;
    }

    string stripblanks(string s)
    {
        for(string::iterator it=s.end()-1; it!=s.begin()-1; --it)
        {    
            if(isspace(*it))
                s.erase(it);
        }
    
        return s;
    }

    bool stricmp(const std::string & s1, const std::string & s2)
    {
        return((s1.size() == s2.size()) &&
               equal(s1.begin(), s1.end(), s2.begin(), charicmp));
    }

// mimics MBUtil version but using streams
    string doubleToString(const double & in, int precision)
    {
        stringstream ss;
        ss << setprecision(precision) << in;
        return ss.str();
    }

    string intToString(const int & in)
    {
        stringstream ss;
        ss << in;
        return ss.str();
    }
    
    // returns file extension from a string (anything from last .)
    string get_extension(const std::string & s)
    {
        string::size_type pos = s.find_last_of(".");

        if(pos != string::npos)
            return s.substr(pos);
        else
            return "";
    }


    string word_wrap(std::string s, int width, const std::string & delim)
    {
        std::string out;

        while((int)s.length() > width)
        {            
            string::size_type pos_newline = s.find("\n");
            string::size_type pos_delim = s.substr(0, width).find_last_of(delim);
            if((int)pos_newline < width)
            {
                out += s.substr(0, pos_newline);
                s = s.substr(pos_newline+1);
            }
            else if (pos_delim != string::npos)
            {
                out += s.substr(0, pos_delim+1);
                s = s.substr(pos_delim+1);
            }
            else
            {
                out += s.substr(0, width);
                s = s.substr(width);
            }
            out += "\n";        
        }
        out += s;
        
        return out;
    }

    // find `key` in `str` and if successful put it in out
    // and return true
    bool val_from_string(std::string & out, const std::string & str, const std::string & key)
    {
        out.erase();
        
        std::string::size_type start_pos = str.find(string(key+"="));
        
        // deal with foo=bar,o=bar problem when looking for "o=" since
        // o is contained in foo
        while(!(start_pos == 0 || start_pos == std::string::npos || str[start_pos-1] == ','))
            start_pos = str.find(string(key+"="), start_pos+1);

        
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
    
    
}
