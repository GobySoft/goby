// t. schneider tes@mit.edu 3.29.09
// ocean engineering graduate student - mit / whoi joint program
// massachusetts institute of technology (mit)
// laboratory for autonomous marine sensing systems (lamss)
// 
// this is message_val.cpp, part of libdccl
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

#include "message_val.h"

void dccl::MessageVal::set(std::string sval)
{ sval_ = sval; sval_set_ = true; }        
void dccl::MessageVal::set(double dval, int precision /* = 15 */)
{ dval_ = dval; dval_set_ = true; precision_ = precision; }
void dccl::MessageVal::set(long lval)
{ lval_ = lval; lval_set_ = true; }
void dccl::MessageVal::set(bool bval)
{ bval_ = bval; bval_set_ = true; }




bool dccl::MessageVal::val(std::string& s) const 
{
    if(sval_set_)
        s = sval_;
    else if(dval_set_)
    {
        std::stringstream ss;
        ss << std::fixed << std::setprecision(precision_) << dval_;
        s = ss.str();
    }            
    else if(lval_set_)
        s = boost::lexical_cast<std::string>(lval_);
    else if(bval_set_)
        s = (bval_) ? "true" : "false";
    else
        return false;
            
    return true;            
}

bool dccl::MessageVal::val(bool& b) const
{
    if(bval_set_)
        b = bval_;
    else if(lval_set_)
    {
        try { b = boost::numeric_cast<bool>(lval_); }
        catch(...) { return false; }        
    }
    else if(dval_set_)
    {
        try { b = boost::numeric_cast<bool>(dval_); }
        catch(...) { return false; }        
    }
    else if(sval_set_)
    {
        if(acomms_util::stricmp(sval_, "true") || acomms_util::stricmp(sval_, "1"))
            b = true;
        else if(acomms_util::stricmp(sval_, "false") || acomms_util::stricmp(sval_, "0"))
            b = false;
        else
            return false;
    }
    else
        return false;

    return true;
}    

bool dccl::MessageVal::val(long& t) const
{
    if(lval_set_)
        t = lval_;
    else if(dval_set_)
    {
        try { t = boost::numeric_cast<long>(acomms_util::sci_round(dval_, 0)); }
        catch(...) { return false; }        
    }    
    else if(bval_set_)
        t = (bval_) ? 1 : 0;
    else if(sval_set_)
    {
        try
        {
            double d = boost::lexical_cast<double>(sval_);
            t = boost::numeric_cast<long>(acomms_util::sci_round(d, 0));
        }
        catch (...)
        {
            if(acomms_util::stricmp(sval_, "true"))
                t = 1;
            else if(acomms_util::stricmp(sval_, "false"))
                t = 0;
            else
                return false;
        }  
    }
    else
        return false;

    return true;            
}        
        
bool dccl::MessageVal::val(double& d) const
{
    if(dval_set_)
        d = dval_;
    else if(lval_set_)
    {
        try { d = boost::numeric_cast<double>(lval_); }
        catch (...) { return false; }
    }
    else if(bval_set_)
        d = (bval_) ? 1 : 0;
    else if(sval_set_)
    {
        try { d = boost::lexical_cast<double>(sval_); }
        catch (boost::bad_lexical_cast &)
        {
            if(acomms_util::stricmp(sval_, "true"))
                d = 1;
            else if(acomms_util::stricmp(sval_, "false"))
                d = 0;
            else
                return false;
        }  
    }
    else
        return false;

    return true;            
}

dccl::DCCLCppType dccl::MessageVal::type()
{
    if(dval_set_)
        return cpp_double;
    else if(sval_set_)
        return cpp_string;
    else if(bval_set_)
        return cpp_bool;
    else if(lval_set_)
        return cpp_long;
    else
        return cpp_notype;
}
