// copyright 2009 t. schneider tes@mit.edu
// 
// this file is part of the Dynamic Compact Control Language (DCCL),
// the goby-acomms codec. goby-acomms is a collection of libraries 
// for acoustic underwater networking
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
{ sval_ = sval; type_ = cpp_string; }
void dccl::MessageVal::set(double dval, int precision /* = 15 */)
{ dval_ = dval; type_ = cpp_double; precision_ = precision; }
void dccl::MessageVal::set(long lval)
{ lval_ = lval; type_ = cpp_long; }
void dccl::MessageVal::set(bool bval)
{ bval_ = bval; type_ = cpp_bool; }


bool dccl::MessageVal::val(std::string& s) const
{
    std::stringstream ss;
    switch(type_)
    {
        case cpp_string:
            s = sval_;
            return true;            

        case cpp_double:
            ss << std::fixed << std::setprecision(precision_) << dval_;
            s = ss.str();
            return true;            

        case cpp_long:
            s = boost::lexical_cast<std::string>(lval_);
            return true;

        case cpp_bool:
            s = (bval_) ? "true" : "false";
            return true;

        default:
            return false;
    }
}

bool dccl::MessageVal::val(bool& b) const
{
    switch(type_)
    {
        case cpp_string:
            if(tes_util::stricmp(sval_, "true") || tes_util::stricmp(sval_, "1"))
                b = true;
            else if(tes_util::stricmp(sval_, "false") || tes_util::stricmp(sval_, "0"))
                b = false;
            else
                return false;
            return true;
            
        case cpp_double:
            try { b = boost::numeric_cast<bool>(dval_); }
            catch(...) { return false; }
            return true;

        case cpp_long:
            try { b = boost::numeric_cast<bool>(lval_); }
            catch(...) { return false; }
            return true;

        case cpp_bool:
            b = bval_;
            return true;

        default:
            return false;
    }
}    

bool dccl::MessageVal::val(long& t) const
{
    switch(type_)
    {
        case cpp_string:
            try
            {
                double d = boost::lexical_cast<double>(sval_);
                t = boost::numeric_cast<long>(tes_util::sci_round(d, 0));
            }
            catch (...)
            {
                if(tes_util::stricmp(sval_, "true"))
                    t = 1;
                else if(tes_util::stricmp(sval_, "false"))
                    t = 0;
                else
                    return false;
            }
            return true;

        case cpp_double:
            try { t = boost::numeric_cast<long>(tes_util::sci_round(dval_, 0)); }
            catch(...) { return false; }
            return true;

        case cpp_long:
            t = lval_;
            return true;

        case cpp_bool:
            t = (bval_) ? 1 : 0;
            return true;

        default:
            return false;
    }
}        
        
bool dccl::MessageVal::val(double& d) const
{
    switch(type_)
    {
        case cpp_string:
            try { d = boost::lexical_cast<double>(sval_); }
            catch (boost::bad_lexical_cast &)
            {
                if(tes_util::stricmp(sval_, "true"))
                    d = 1;
                else if(tes_util::stricmp(sval_, "false"))
                    d = 0;
                else
                    return false;
            }
            return true;

        case cpp_double:
            d = dval_;
            return true;

        case cpp_long:
            try { d = boost::numeric_cast<double>(lval_); }
            catch (...) { return false; }
            return true;

        case cpp_bool:
            d = (bval_) ? 1 : 0;
            return true;

        default:
            return false;

    }
}
