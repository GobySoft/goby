// t. schneider tes@mit.edu 07.02.09
// ocean engineering graudate student - mit / whoi joint program
// massachusetts institute of technology (mit)
// laboratory for autonomous marine sensing systems (lamss)
// 
// this is dynamic_moos_vars.hpp
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

#ifndef DYNAMICMOOSVARSH
#define DYNAMICMOOSVARSH

#include "MOOSLIB/MOOSLib.h"

namespace tes
{
    class DynamicMOOSVars
    {
    public:
        const CMOOSMsg & get_moos_var(const std::string& s)
        { return vars[s]; }
        
        const CMOOSMsg & operator[](const std::string& s)
        { return vars[s]; }
    
        // read the whole list
        void update_moos_vars(const MOOSMSG_LIST& NewMail)
            {
                for(MOOSMSG_LIST::const_iterator p = NewMail.begin(), n = NewMail.end(); p != n; ++p)
                    vars[p->GetKey()] = *p;
            }

        // update a single variable at a time
        void update_moos_vars(const CMOOSMsg& msg)
            { vars[msg.GetKey()] = msg; }
    
    private:
        std::map<std::string, CMOOSMsg> vars;

    };
}

inline bool valid(const CMOOSMsg& m)
{ return m.GetTime() != -1; }    
    
#endif
