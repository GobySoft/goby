// copyright 2008, 2009 t. schneider tes@mit.edu
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

#include "util/tes_utils.h"

#include "message_var.h"
#include "message_val.h"
#include "dccl_constants.h"
#include "message_algorithms.h"

dccl::MessageVar::MessageVar() : max_(1e300),
                                 min_(0.0),
                                 max_length_(0),
                                 precision_(0),
                                 source_set_(false),
                                 expected_delta_(acomms_util::NaN),
                                 delta_var_(false),
                                 ap_(AlgorithmPerformer::getInstance())
{ }

void dccl::MessageVar::read_dynamic_vars(std::map<std::string,MessageVal>& vals,
                                         const std::map<std::string, MessageVal>& in)
{
    const std::map<std::string, dccl::MessageVal>::const_iterator it =
        in.find(source_var_);

    if(it != in.end())
    {
        MessageVal val = it->second;
        
        switch(val.type())
        {
            case cpp_string: 
                val = parse_string_val(val);
                break;
                
            default:
                break;

        }
        vals[name_] = val;
    }
}


void dccl::MessageVar::initialize(const std::string& message_name, const std::string& trigger_var, bool delta_encode)
{
    // add trigger_var_ as source_var for any message_vars without a source
    if(!source_set_)
        source_var_ = trigger_var;
    
//    if(source_var_ == "" && type_ != dccl_static)
//    {
//        throw std::runtime_error("(Error) missing source moos variable for a layout parameter in the message above.");
//    }
    
    // flip max and min if needed
    if(max_ < min_)
    {
        double tmp = max_;
        max_ = min_;
        min_ = tmp;
        std::cout << "(Warning) max < min for field {" << name_ << "} in message {" << message_name << "}. max and min switched." << std::endl;
    } 

    if(type_ == dccl_int || type_ == dccl_float)
    {        
        // needs to be positive value
        expected_delta_ = abs(expected_delta_);
        if(isnan(expected_delta_) && delta_encode)
        {
            // use default of 10% of range
            expected_delta_ = 0.1 * (max_ - min_);
        }
    }
}


// return size (in bits) of a message portion
int dccl::MessageVar::calc_size() const
{
    switch(type_)
    {
        // +2: one since you need to store zero. e.g., 0->255 is 256 values, other to store "not_specified" value
        case dccl_int:    return ceil(log(max()-min()+2)/log(2));            
        case dccl_bool:   return 1 + 1;

        // eight bits per byte (or character (ASCII))
        case dccl_hex:
        case dccl_string: return max_length_*acomms_util::BITS_IN_BYTE;

        case dccl_float:  return ceil(log((max()-min())*pow(10.0,static_cast<double>(precision_))+2)/log(2));
        case dccl_static: return 0;
            // +1 for overflow (that is, value is not in enums list)
        case dccl_enum:   return ceil(log(enums_.size()+1)/log(2));
    }

    return 0;
}

std::string dccl::MessageVar::get_display() const
{
    std::stringstream ss;    
    
    ss << "\t" << name_ << " (" << type_to_string(type_) << "):" << std::endl;
    
    
    for(std::vector<std::string>::size_type j = 0, m = algorithms_.size(); j < m; ++j)
    {
        if(!j)
            ss << "\t\talgorithm(s): ";
        else
            ss << ", ";
        ss << algorithms_[j];
        if (j==(m-1))
            ss << std::endl;
    }
    
    if(type_ != dccl_static)
    {
        if(source_var_ != "")
        {
            ss << "\t\t" << "source: {";
            ss << source_var_;
            ss  << "}";
            if(source_key_ != "")
                ss << " key: " << source_key_;

            
            ss << std::endl;

        }
    }
    else
    {
        ss << "\t\t" << "value: \"" << static_val_ << "\"" << std::endl;
    }
        
    if (type_ == dccl_int || type_ == dccl_float)
    {
        ss << "\t\t[min, max] = [" << min_ << "," << max_ << "]" << std::endl;
        if(!isnan(expected_delta_))
           ss << "\t\texpected_delta: {" << expected_delta_ << "}" << std::endl;
    }
    
    
    if (type_ == dccl_float)
        ss << "\t\tprecision: {" << precision_ << "}" << std::endl;   
    if (type_ == dccl_string)
        ss << "\t\tmax_length: {" << max_length_ << "}" << std::endl;            
    if (type_ == dccl_enum)
    {
        ss << "\t\tvalues:{"; 
        for (std::vector<std::string>::size_type j = 0, m = enums_.size(); j < m; ++j)
        {
            if(j)
                ss << ",";
            ss << enums_[j];
        }
            
        ss << "}" << std::endl;
    }
        

    ss << "\t\tsize [bits]: [" << calc_size() << "]" << std::endl;

    return ss.str();
}


// deal with cases where key=value exists within the string
std::string dccl::MessageVar::parse_string_val(const std::string& sval)
{
    std::string pieceval;

    // is the parameter part of the std::string (as opposed to being the std::string)
    // that is, in_str is true if "key=value" is part of the string, rather
    // than the std::string simply being "value"
    bool in_str = false;
        
    // see if the parameter is *in* the string, if so put it in pieceval
    // use source_key if specified, otherwise try the name
    std::string subkey = (source_key_ == "") ? name_ : source_key_;
        
    in_str = tes_util::val_from_string(pieceval, sval, subkey);        
    //pick the substring from the string
    if(in_str)
        return pieceval;
    else
        return sval;

}

void dccl::MessageVar::var_encode(std::map<std::string,MessageVal>& vals, boost::dynamic_bitset<>& bits)
{
    MessageVal v = vals[name_];
        
    // run the algorithms!
    for(std::vector<std::string>::size_type i = 0, n = algorithms_.size(); i < n; ++i)
    {
        ap_->algorithm(v, algorithms_[i], vals);
    }
    
    // in bits
    unsigned int size = calc_size();
    boost::dynamic_bitset<>::size_type bits_size = bits.size();

    bool b;
    std::string s;
    long t;
    double r;
    switch(type_)
    {
        case dccl_static:
            break;
            
        
        case dccl_bool:
            bits <<= size;
            if(v.get(b))
                bits |= boost::dynamic_bitset<>(bits_size, ((b) ? 1 : 0) + 1);
            break;

        case dccl_hex:
            v.get(s);
            
            // max_length_ is max bytes            
            if(s.length() == bytes2nibs(max_length_))
            {
                bits <<= size;
                bits |= tes_util::hex_string2dyn_bitset(s, bits_size);                
                break;
            }
            else if(s.length() < bytes2nibs(max_length_))
                throw(std::runtime_error(std::string("Passed hex value (" + s + ") is too short. Should be " + boost::lexical_cast<std::string>(max_length_) +  " bytes")));
            else if(s.length() > bytes2nibs(max_length_))
                throw(std::runtime_error(std::string("Passed hex value (" + s + ") is too long. Should be " + boost::lexical_cast<std::string>(max_length_) +  " bytes")));

            break;
            
        case dccl_string:
            v.get(s);
            
            // tack on null terminators (probably a byte of zeros in ASCII)
            s += std::string(max_length_, '\0');
            
            // one byte per char
            for (size_t j = 0; j < (size_t)max_length_; ++j)
            {
                bits <<= acomms_util::BITS_IN_BYTE;
                bits |= boost::dynamic_bitset<>(bits_size, s[j]);;
            }

            break;

        case dccl_int:
            bits <<= size;    

            if(v.get(t) && !(t < min() || t > max()))
            {
                t -= static_cast<long>(min());
                bits |= boost::dynamic_bitset<>(bits_size, static_cast<unsigned long>(t)+1);
            }
            break;

        case dccl_float:
            bits <<= size;
            
            if(v.get(r) && !(r < min() || r > max()))
            {
                r = (r-min())*pow(10.0, static_cast<double>(precision_));
                
                r = tes_util::sci_round(r, 0);
                
                bits |= boost::dynamic_bitset<>(bits_size, static_cast<unsigned long>(r)+1);
            }
            

            break;

        case dccl_enum:
            
            bits <<= size;

            if(v.get(s))
            {
                // find the iterator within the std::vector of enumerator values for *this* enumerator value
                std::vector<std::string>::iterator pos;
                pos = find(enums_.begin(), enums_.end(), s);
                
                // now convert that iterator into a number (think traditional array index)
                unsigned long t = (unsigned long)distance(enums_.begin(), pos);

                if(pos == enums_.end())
                    t = 0;
                else
                    ++t;
                
                bits |= boost::dynamic_bitset<>(bits_size, t);
            }

            break;
    }
}

void dccl::MessageVar::var_decode(std::map<std::string,MessageVal>& vals, boost::dynamic_bitset<>& bits)
{   
    MessageVal v;
        
    unsigned int size = calc_size();

    boost::dynamic_bitset<>::size_type bits_size = bits.size();
    
    char s[max_length_+1];

    std::string hex_s;

    unsigned long t;
    boost::dynamic_bitset <> mask(bits_size);
    std::stringstream smask;
    smask << std::string(size, '1');
    smask >> mask;
    mask.resize(bits_size);
    
    switch(type_)
    {
        case dccl_static:
            v.set(static_val_);
            break;

        case dccl_hex:
            v.set(tes_util::dyn_bitset2hex_string(bits & mask, max_length_));
            bits >>= size;
            break;
            
        case dccl_string:
            s[max_length_] = '\0';
            
            for (size_t j = 0; j < max_length_; ++j)
            {
                s[max_length_-j-1] = (bits & boost::dynamic_bitset<>(bits_size, 0xff)).to_ulong();
                bits >>= acomms_util::BITS_IN_BYTE;
            }
            
            if(!std::string(s).empty())
                v.set(std::string(s));
            
            break;
            
        case dccl_float:
        case dccl_int:
        case dccl_enum:
        case dccl_bool:            
            t = (bits & mask).to_ulong();
            bits >>= size;
            
            if(t)
            {
                --t;

                if(type_ == dccl_bool)
                    v.set(bool((t)));
                else if(type_ == dccl_float)
                    v.set(static_cast<double>(t) / (pow(10.0, static_cast<double>(precision_))) + min(), precision_);
                else if(type_ == dccl_enum)
                    v.set(enums_[t]);
                else
                    v.set(static_cast<long>(t) + static_cast<long>(min()));
            }
            break;
    }

    vals[name_] = v;
}

std::ostream& dccl::operator<< (std::ostream& out, const MessageVar& mv)
{
    out << mv.get_display();
    return out;
}
