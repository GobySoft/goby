// copyright 2008, 2009, 2010 t. schneider tes@mit.edu
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

#include "message_var_float.h"

#include "goby/util/sci.h"

goby::acomms::DCCLMessageVarFloat::DCCLMessageVarFloat(double max /*= std::numeric_limits<double>::max()*/, double min /*= 0*/, double precision /*= 0*/)
    : DCCLMessageVar(),
      max_(max),
      min_(min),
      precision_(precision),
      max_delta_(acomms::NaN)
{ }

int goby::acomms::DCCLMessageVarFloat::calc_total_size() const
{
    if(using_delta_differencing())
        // key frame + N-1 delta frames
        return key_size() + (array_length_-1)*delta_size();
    else
        // size of array * bits / element
        return array_length_*key_size();
}
        
void goby::acomms::DCCLMessageVarFloat::initialize_specific()
{
    // flip max and min if needed
    if(max_ < min_)
    {
        double tmp = max_;
        max_ = min_;
        min_ = tmp;
    } 

    if(using_delta_differencing() && max_delta_ < 0)
        max_delta_ = -max_delta_;
}
        
boost::dynamic_bitset<unsigned char> goby::acomms::DCCLMessageVarFloat::encode_specific(const DCCLMessageVal& v)
{
    double r;
    if(!v.get(r) || (r < min() || r > max())) return boost::dynamic_bitset<unsigned char>();

    // delta-differencing
    if(is_delta())
    {
        r += max_delta_ - double(key_val_);
        if((r > 2*max_delta_)) return boost::dynamic_bitset<unsigned char>();
    }
    else
    {
        r -= min_;
    }
    
    r *= pow(10.0, double(precision_));
    r = util::unbiased_round(r, 0);

    return boost::dynamic_bitset<unsigned char>(calc_size(), static_cast<unsigned long>(r)+1);
}        
        
goby::acomms::DCCLMessageVal goby::acomms::DCCLMessageVarFloat::decode_specific(boost::dynamic_bitset<unsigned char>& b)
{            
    unsigned long t = b.to_ulong();
    if(!t) return DCCLMessageVal();
                
    --t;
    if(is_delta())
        return cast(double(t) / (pow(10.0, double(precision_))) - double(max_delta_) + double(key_val_), precision_);
    else
        return cast(double(t) / (pow(10.0, double(precision_))) + min_, precision_);
}

void goby::acomms::DCCLMessageVarFloat::get_display_specific(std::stringstream& ss) const
{
    ss << "\t\t[min, max] = [" << min_ << "," << max_ << "]" << std::endl;
    ss << "\t\tprecision: {" << precision_ << "}" << std::endl;   

    if(using_delta_differencing())
    {
        ss << "\t\tmax_delta: {" << max_delta_ << "}" << std::endl;
        ss << "\t\tdelta element size [bits]: [" << delta_size() << "]" << std::endl;
    }
}        
