// copyright 2009 t. schneider tes@mit.edu
// 
// this file is part of goby-logger,
// the goby logging library
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

#ifndef LoggerManipulators20091110H
#define LoggerManipulators20091110H

#include <string>
#include <deque>
#include <ctime>

#include <boost/algorithm/string.hpp>

inline std::ostream & die(std::ostream & os)
{ return (os << "\33[31m(Error): \33[0m"); }

inline std::ostream & warn(std::ostream & os)
{ return (os << "\33[31m(Warning): \33[0m"); }

class Group
{
  public:
  Group(const std::string& name = "",
        const std::string& description = "",
        const std::string& color = "",
        const std::string& heartbeat = "")
      : name_(name),
        description_(description),
        color_(color),
        heartbeat_(heartbeat),
        enabled_(true)
        {}

    std::string name() const { return name_; }        
    std::string description() const { return description_; }        
    std::string color() const       { return color_; }        
    std::string heartbeat() const   { return heartbeat_; }
    bool enabled() const            { return enabled_; }

    void name(const std::string & s)        { name_ = s; }    
    void description(const std::string & s) { description_ = s; }    
    void color(const std::string & s)       { color_ = s; }        
    void heartbeat(const std::string & s)   { heartbeat_ = s; }
    void enabled(bool b)                    { enabled_ = b; }
    
  private:
    std::string name_;
    std::string description_;
    std::string color_;
    std::string heartbeat_;
    bool enabled_;
};

std::ostream & operator<< (std::ostream & os, const Group & g);

class GroupSetter
{
  public:
    explicit GroupSetter (const std::string& s) : group_(s) { }
    void operator()(std::ostream& os) const;

  private:
    std::string group_;
    
};

inline GroupSetter group(std::string n) 
{ return(GroupSetter(n)); }

inline std::ostream& operator<<(std::ostream& os, const GroupSetter & gs)
{ gs(os); return(os); }

#endif
