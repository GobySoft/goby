#ifndef CMOOSMsgOperatorH
#define CMOOSMsgOperatorH

#include "MOOSLIB/MOOSMsg.h"

template <typename _CharT, typename _Traits>
    static std::basic_ostream <_CharT, _Traits>&
    operator<< (std::basic_ostream <_CharT, _Traits>& OutputStream, const CMOOSMsg& Msg)
{
    OutputStream << "key=" << Msg.GetKey ();
    OutputStream << ", ";
    
    if (Msg.IsDouble ())
    {
        OutputStream << "double=" << Msg.GetDouble ();
    }
    else
    {
        OutputStream << "string=" << Msg.GetString ();
    }
    OutputStream << ", ";
    OutputStream << std::fixed << std::setprecision (2) << "time=" << Msg.GetTime ();
    return OutputStream;
}

#endif
