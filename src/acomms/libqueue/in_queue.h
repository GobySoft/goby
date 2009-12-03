// t. schneider tes@mit.edu 06.16.09
// ocean engineering graudate student - mit / whoi joint program
// massachusetts institute of technology (mit)
// laboratory for autonomous marine sensing systems (lamss)
// 
// this is in_queue.h
//
// see the readme file within this directory for information
// pertaining to usage and purpose of this script.
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


#ifndef InQueueH
#define InQueueH

#include <string>

class InQueue
{
  public:

  InQueue() : variable_(""),
        leave_header_(false)
        {}
    
  InQueue(std::string variable, bool leave_header) : variable_(variable),
        leave_header_(leave_header)
        {}

    
    std::string variable() { return variable_; }
    bool leave_header()    { return leave_header_; }

    void set_variable(std::string variable)  { variable_ = variable; }
    void set_leave_header(bool leave_header) { leave_header_ = leave_header; }
    
    
  private:
    std::string variable_;
    bool leave_header_;
    
};


    
#endif
