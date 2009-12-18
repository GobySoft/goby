// copyright 2009 t. schneider tes@mit.edu
//
// this file is part of flex-ostream, a terminal display library
// that provides an ostream with both terminal display and file logging
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

#ifndef TermColor20091211H
#define TermColor20091211H

#include <sstream>

#include <boost/assign.hpp>
#include <boost/foreach.hpp>

// defines a number of stream operators that implement colors
namespace termcolor
{    
    const std::string esc_red = "\33[31m";
    const std::string esc_lt_red = "\33[91m";
    const std::string esc_green = "\33[32m";
    const std::string esc_lt_green = "\33[92m";
    const std::string esc_yellow = "\33[33m";
    const std::string esc_lt_yellow = "\33[93m";
    const std::string esc_blue = "\33[34m";
    const std::string esc_lt_blue ="\33[94m";
    const std::string esc_magenta = "\33[35m";
    const std::string esc_lt_magenta = "\33[95m";
    const std::string esc_cyan = "\33[36m";
    const std::string esc_lt_cyan = "\33[96m";
    const std::string esc_white = "\33[37m";
    const std::string esc_lt_white = "\33[97m";
    const std::string esc_nocolor = "\33[0m";
    
    inline std::ostream& red(std::ostream& os) 
    { return(os << esc_red); }

    inline std::ostream& lt_red(std::ostream& os) 
    { return(os << esc_lt_red); }

    inline std::ostream& green(std::ostream& os) 
    { return(os << esc_green); }

    inline std::ostream& lt_green(std::ostream& os) 
    { return(os << esc_lt_green); }

    inline std::ostream& yellow(std::ostream& os) 
    { return(os << esc_yellow); }

    inline std::ostream& lt_yellow(std::ostream& os) 
    { return(os << esc_lt_yellow); }

    inline std::ostream& blue(std::ostream& os) 
    { return(os << esc_blue); }

    inline std::ostream& lt_blue(std::ostream& os) 
    { return(os << esc_lt_blue); }

    inline std::ostream& magenta(std::ostream& os) 
    { return(os << esc_magenta); }

    inline std::ostream& lt_magenta(std::ostream& os) 
    { return(os << esc_lt_magenta); }

    inline std::ostream& cyan(std::ostream& os)
    { return(os << esc_cyan); }

    inline std::ostream& lt_cyan(std::ostream& os) 
    { return(os << esc_lt_cyan); }
 
    inline std::ostream& white(std::ostream& os) 
    { return(os << esc_white); }
  
    inline std::ostream& lt_white(std::ostream& os) 
    { return(os << esc_lt_white); }

    inline std::ostream& nocolor(std::ostream& os) 
    { return(os << esc_nocolor); }

    namespace enums
    {
        enum Color { nocolor,
                      red,     lt_red,
                      green,   lt_green,
                      yellow,  lt_yellow,
                      blue,    lt_blue,
                      magenta, lt_magenta,
                      cyan,    lt_cyan,
                      white,   lt_white };
    }
    
    class TermColor
    {
      public:
        TermColor()
        {
            boost::assign::insert(colors_map_)
                ("nocolor",enums::nocolor)
                ("red",enums::red)         ("lt_red",enums::lt_red)
                ("green",enums::green)     ("lt_green",enums::lt_green)
                ("yellow",enums::yellow)   ("lt_yellow",enums::lt_yellow)
                ("blue",enums::blue)       ("lt_blue",enums::lt_blue)
                ("magenta",enums::magenta) ("lt_magenta",enums::lt_magenta)
                ("cyan",enums::cyan)       ("lt_cyan",enums::lt_cyan)
                ("white",enums::white)     ("lt_white",enums::lt_white);

            boost::assign::insert(esc_code_map_)
                (esc_nocolor,enums::nocolor)
                (esc_red,enums::red)         (esc_lt_red,enums::lt_red)
                (esc_green,enums::green)     (esc_lt_green,enums::lt_green)
                (esc_yellow,enums::yellow)   (esc_lt_yellow,enums::lt_yellow)
                (esc_blue,enums::blue)       (esc_lt_blue,enums::lt_blue)
                (esc_magenta,enums::magenta) (esc_lt_magenta,enums::lt_magenta)
                (esc_cyan,enums::cyan)       (esc_lt_cyan,enums::lt_cyan)
                (esc_white,enums::white)     (esc_lt_white,enums::lt_white);

        }

        enums::Color from_str(const std::string& s)
        {
	  return colors_map_[s];
        }

        // red -> "red"
        std::string str_from_col(const enums::Color& c)
        {
	  typedef std::pair<std::string, enums::Color> P;
	    BOOST_FOREACH(const P& p, colors_map_)
	    {
	      if(p.second == c)
		return p.first;
	    }
	  return "nocolor";
        }

        // "\33[31m" -> red
        enums::Color from_esc_code(const std::string& s)
        {
	  return esc_code_map_[s];
        }

        // red -> "\33[31m"
        std::string esc_code_from_col(const enums::Color& c)
        {
	  typedef std::pair<std::string, enums::Color> P;
	    BOOST_FOREACH(const P& p, esc_code_map_)
	    {
	      if(p.second == c)
		return p.first;
	    }
	  return esc_nocolor;
        }
        
        // "red" -> "\33[31m"
        std::string esc_code_from_str(const std::string& s)
        {
	  return esc_code_from_col(from_str(s));
        }
 
	


	// use BOOST bimap, which isn't out until 1.35
       /* // "red" -> red */
        /* enums::Color from_str(const std::string& s) */
        /* { */
        /*     try { return colors_map_.left.at(s); } */
        /*     catch(...) { return enums::nocolor; }             */
        /* } */

        /* // red -> "red" */
        /* std::string str_from_col(const enums::Color& c) */
        /* { */
        /*     try { return colors_map_.right.at(c); } */
        /*     catch(...) { return "nocolor"; } */
        /* }         */

        /* // "\33[31m" -> red */
        /* enums::Color from_esc_code(const std::string& s) */
        /* { */
        /*     try { return esc_code_map_.left.at(s); } */
        /*     catch(...) { return enums::nocolor; } */
        /* } */

        /* // red -> "\33[31m" */
        /* std::string esc_code_from_col(const enums::Color& c) */
        /* { */
        /*     try { return esc_code_map_.right.at(c); } */
        /*     catch(...) { return esc_nocolor; } */
        /* } */
        
        /* // "red" -> "\33[31m" */
        /* std::string esc_code_from_str(const std::string& s) */
        /* { */
        /*     try { return esc_code_from_col(from_str(s)); } */
        /*     catch(...) { return esc_nocolor; } */
        /* } */

        
      private:        
	// not until BOOST 1.35...
        //boost::bimap<std::string, enums::Color> colors_map_;
        //boost::bimap<std::string, enums::Color> esc_code_map_;
	std::map<std::string, enums::Color> colors_map_;
	std::map<std::string, enums::Color> esc_code_map_;

    };
    
    
}

#endif
