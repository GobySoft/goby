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

#ifndef STRING20100713H
#define STRING20100713H

#include <string>
#include <vector>
#include <boost/lexical_cast.hpp>
#include <iostream>
#include <limits>
#include <boost/utility.hpp>
#include <boost/type_traits.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/static_assert.hpp>
#include <boost/mpl/and.hpp>
#include <boost/mpl/logical.hpp>
#include <boost/numeric/conversion/cast.hpp>

namespace goby
{

    namespace util
    {   
        /// \name String
        //@{
        
        template<typename To>
            typename boost::enable_if<boost::is_fundamental<To>, To>::type
            _as_from_string(const std::string& from)
        {
            try { return boost::lexical_cast<To>(from); }
            catch(boost::bad_lexical_cast&)
            {
                // return NaN or maximum value supported by the type
                return std::numeric_limits<To>::has_quiet_NaN
                    ? std::numeric_limits<To>::quiet_NaN()
                    : std::numeric_limits<To>::max();
            }
        }
        
        template<typename To>
            typename boost::disable_if<boost::is_fundamental<To>, To>::type
            _as_from_string(const std::string& from)
        {
            try { return boost::lexical_cast<To>(from); }
            catch(boost::bad_lexical_cast&)
            {
                return To();
            }
        }

        template <>
            inline bool _as_from_string<bool>(const std::string& from)
        {
            return (boost::iequals(from, "true") || boost::iequals(from, "1"));
        }        

        template <>
            inline std::string _as_from_string<std::string>(const std::string& from)
        {
            return from;
        }
        
        template<typename To, typename From>
            std::string _as_to_string(const From& from)
        {
            try { return boost::lexical_cast<std::string>(from); }
            catch(boost::bad_lexical_cast&)
            {
                return std::string();
            }
        }
        
        /// specialization of as() for bool -> string
        template <>
            inline std::string _as_to_string<std::string, bool>(const bool& from)
        {
            return from ? "true" : "false";
        }

        template <>
            inline std::string _as_to_string<std::string, std::string>(const std::string& from)
        {
            return from;
        }
        
        template<typename To, typename From>
            typename boost::disable_if<boost::is_same<To,From>, To>::type
            _as_numeric(const From& from)
        {
            try { return boost::numeric_cast<To>(from); }
            catch(boost::bad_numeric_cast&)
            {
                // return NaN or maximum value supported by the type
                return std::numeric_limits<To>::has_quiet_NaN
                    ? std::numeric_limits<To>::quiet_NaN()
                    : std::numeric_limits<To>::max();
            }
        }        
        
        template<typename To, typename From>
            typename boost::enable_if<boost::is_same<To,From>, To>::type
            _as_numeric(const From& from)
        {
            return from;
        }

        template<typename To>
            To as(const std::string& from)
        {
            return _as_from_string<To>(from);
        }

        template<typename To, typename From>
            typename boost::enable_if<boost::is_same<To, std::string>, To>::type
            as(const From& from)
        {
            return _as_to_string<To,From>(from);
        }

        template<typename To, typename From>
            typename boost::enable_if<boost::mpl::and_<boost::is_arithmetic<To>,
            boost::is_arithmetic<From> >, To>::type
            as(const From& from)
        {
            return _as_numeric<To, From>(from);
        }
        
        /// remove all blanks from string s    
        inline void stripblanks(std::string& s)
        {
            for(std::string::iterator it=s.end()-1; it!=s.begin()-1; --it)
            {    
                if(isspace(*it))
                    s.erase(it);
            }
        }
        
        /// find `key` in `str` and if successful put it in out
        /// and return true
        /// deal with these basic forms:
        /// str = foo=1,bar=2,pig=3
        /// str = foo=1,bar={2,3,4,5},pig=3
        /// \param out string to return value in
        /// \param str haystack to search
        /// \param key needle to find
        /// \return true if key is in str, false otherwise
        inline bool val_from_string(std::string& out, const std::string& str, const std::string & key)
        {
            // str:  foo=1,bar={2,3,4,5},pig=3,cow=yes
            // two cases:
            // key:  bar
            // key:  pig
            
            out.erase();

            // str:  foo=1,bar={2,3,4,5},pig=3,cow=yes
            //  start_pos  ^             ^
            std::string::size_type start_pos = str.find(std::string(key+"="));
        
            // deal with foo=bar,o=bar problem when looking for "o=" since
            // o is contained in foo

            // ok: beginning of string, end of string, comma right before start_pos
            while(!(start_pos == 0 || start_pos == std::string::npos || str[start_pos-1] == ','))
                start_pos = str.find(std::string(key+"="), start_pos+1);
        
            if(start_pos != std::string::npos)
            {
                // chopped:   bar={2,3,4,5},pig=3,cow=yes
                // chopped:   pig=3,cow=yes
                std::string chopped = str.substr(start_pos);

                // chopped:  bar={2,3,4,5},pig=3,cow=yes
                // equal_pos    ^         
                // chopped:  pig=3,cow=yes
                // equal_pos    ^         
                std::string::size_type equal_pos = chopped.find("=");

                // check for array
                bool is_array = (equal_pos+1 < chopped.length() && chopped[equal_pos+1] == '{');
            
                if(equal_pos != std::string::npos)
                {
                    // chopped_twice:  ={2,3,4,5},pig=3,cow=yes
                    // chopped_twice:  =pig=3,cow=yes              
                    std::string chopped_twice = chopped.substr(equal_pos);

                    // chopped_twice:  ={2,3,4,5},pig=3,cow=yes  
                    // end_pos                  ^     
                    // chopped_twice:  =pig=3,cow=yes
                    // end_pos               ^         
                    std::string::size_type end_pos =
                        (is_array) ? chopped_twice.find("}") : chopped_twice.find(",");

                    // out:  2,3,4,5
                    // out:  3
                    out = (is_array) ? chopped_twice.substr(2, end_pos-2) : chopped_twice.substr(1, end_pos-1);

                    return true;
                }
            }

            return false;        
        }        

        /// variation of val_from_string() for arbitrary return type
        /// \return false if `key` not in `str` OR if `out` is not of proper type T
        template<typename T>
            inline bool val_from_string(T& out, const std::string& str, const std::string & key)        
        {
            std::string s;
            if(!val_from_string(s, str, key)) return false;
            
            try { out = boost::lexical_cast<T>(s); }
            catch (boost::bad_lexical_cast&)
            { return false; }

            return true;            
        }
        

        /// specialization of val_from_string for boolean `out`
        inline bool val_from_string(bool& out, const std::string& str, const std::string & key)
        {
            std::string s;
            if(!val_from_string(s, str, key)) return false;

            out = as<bool>(s);
            return true;            
        }        
        //@}
    }
}
#endif

