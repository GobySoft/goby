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

#include <exception>

#include <boost/foreach.hpp>

#include "message_val.h"

void dccl::MessageVal::init()
{
    sval_ = "";
    dval_ = 0;
    lval_ = 0;
    bval_ = false;
    precision_ = MAX_DBL_PRECISION;
    type_ = cpp_notype;
}


dccl::MessageVal::MessageVal()
{ init(); }


dccl::MessageVal::MessageVal(const std::string& s)
{
    init();
    sval_ = s;
    type_ = cpp_string;
}


dccl::MessageVal::MessageVal(const char* s)
{
    init();
    sval_ = s;
    type_ = cpp_string;
}


dccl::MessageVal::MessageVal(double d, int p /* = MAX_DBL_PRECISION*/ )
{
    init();
    dval_ = d;
    precision_ = p;
    type_ = cpp_double;
}

dccl::MessageVal::MessageVal(float f)
{
    init();
    dval_ = f;
    type_ = cpp_double;
}

dccl::MessageVal::MessageVal(long l)
{
    init();
    lval_ = l;
    type_ = cpp_long;
}        

dccl::MessageVal::MessageVal(int i)
{
    init();
    lval_ = i;
    type_ = cpp_long;
}        

dccl::MessageVal::MessageVal(bool b)
{
    init();
    bval_ = b;
    type_ = cpp_bool;
}        


dccl::MessageVal::MessageVal(const std::vector<MessageVal>& vm)
{
    if(vm.size() != 1)
        throw(std::runtime_error("vector cast to MessageVal failed: vector is not size 1"));
    else
        *this = vm[0];
}


void dccl::MessageVal::set(std::string sval)
{ sval_ = sval; type_ = cpp_string; }
void dccl::MessageVal::set(double dval, int precision /* = MAX_DBL_PRECISION */)
{ dval_ = dval; type_ = cpp_double; precision_ = precision; }
void dccl::MessageVal::set(long lval)
{ lval_ = lval; type_ = cpp_long; }
void dccl::MessageVal::set(bool bval)
{ bval_ = bval; type_ = cpp_bool; }


bool dccl::MessageVal::get(std::string& s) const
{
    std::stringstream ss;
    switch(type_)
    {
        case cpp_string:
            s = sval_;
            return true;            

        case cpp_double:
            if((log10(dval_) + precision_) <= MAX_DBL_PRECISION) 
                ss << std::fixed << std::setprecision(precision_) << dval_;
            else
                ss << std::setprecision(precision_) << dval_;
            
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

bool dccl::MessageVal::get(bool& b) const
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

bool dccl::MessageVal::get(long& t) const
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
        
bool dccl::MessageVal::get(double& d) const
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




dccl::MessageVal::operator double() const
{
    double d;
    if(get(d)) return d;
    else return acomms::NaN;
}


dccl::MessageVal::operator bool() const
{
    bool b;
    if(get(b)) return b;
    else return false;
}

dccl::MessageVal::operator std::string() const
{
    std::string s;
    if(get(s)) return s;
    else return "";
}

dccl::MessageVal::operator long() const
{
    long l;
    if(get(l)) return l;
    else return 0;
}

dccl::MessageVal::operator int() const
{
    return long(*this);
}

dccl::MessageVal::operator unsigned() const
{
    return long(*this);
}

dccl::MessageVal::operator float() const
{
    return double(*this);
}

dccl::MessageVal::operator std::vector<MessageVal>() const
{
    return std::vector<MessageVal>(1, *this);
}

bool dccl::MessageVal::operator==(const MessageVal& mv) const
{
    switch(mv.type_)
    {
        case cpp_string: return mv == sval_;
        case cpp_double: return mv == dval_;
        case cpp_long:   return mv == lval_;
        case cpp_bool:   return mv == bval_;
        default:         return false;
    }
}

bool dccl::MessageVal::operator==(const std::string& s)  const
{
    std::string us;
    return get(us) && us == s;
}

bool dccl::MessageVal::operator==(double d) const
{
    double us;
    return get(us) && us == d;
}

bool dccl::MessageVal::operator==(long l) const
{
    long us;
    return get(us) && us == l;
}

bool dccl::MessageVal::operator==(bool b) const
{
    bool us;
    return get(us) && us == b;
}        



std::ostream& dccl::operator<<(std::ostream& os, const dccl::MessageVal& mv)
{
    switch(mv.type_)
    {
        case cpp_string: return os << "std::string: " << mv.sval_;
        case cpp_double: return os << "double: " << std::fixed << std::setprecision(mv.precision_) << mv.dval_;
        case cpp_long:   return os << "long: " << mv.lval_;                
        case cpp_bool:   return os << "bool: " << std::boolalpha << mv.bval_;
        default:         return os << "{empty}";
    }
}

    
std::ostream& dccl::operator<<(std::ostream& os, const std::vector<dccl::MessageVal>& vm)
{
    int j=0;
    BOOST_FOREACH(const dccl::MessageVal& m, vm)
    {
        if(j) os << ",";
        os << m;
        ++j;        
    }
    return os;
}
