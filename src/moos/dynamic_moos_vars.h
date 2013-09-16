// Copyright 2009-2013 Toby Schneider (https://launchpad.net/~tes)
//                     Massachusetts Institute of Technology (2007-)
//                     Woods Hole Oceanographic Institution (2007-)
//                     Goby Developers Team (https://launchpad.net/~goby-dev)
// 
//
// This file is part of the Goby Underwater Autonomy Project MOOS Interface Library
// ("The Goby MOOS Library").
//
// The Goby MOOS Library is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// The Goby MOOS Library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Goby.  If not, see <http://www.gnu.org/licenses/>.


#ifndef DYNAMICMOOSVARSH
#define DYNAMICMOOSVARSH

#include "MOOS/libMOOS/Comms/MOOSMsg.h"

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

        std::map<std::string, CMOOSMsg>& all() { return vars; }
        
    private:
        std::map<std::string, CMOOSMsg> vars;

    };
}

inline bool valid(const CMOOSMsg& m)
{ return m.GetTime() != -1; }    
    
#endif
