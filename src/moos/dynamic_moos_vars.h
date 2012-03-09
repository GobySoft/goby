
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

        std::map<std::string, CMOOSMsg>& all() { return vars; }
        
    private:
        std::map<std::string, CMOOSMsg> vars;

    };
}

inline bool valid(const CMOOSMsg& m)
{ return m.GetTime() != -1; }    
    
#endif
