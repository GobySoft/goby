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

goby::transitional::DCCLMessageVarFloat::DCCLMessageVarFloat(double max /*= std::numeric_limits<double>::max()*/, double min /*= 0*/, double precision /*= 0*/)
    : DCCLMessageVar(),
      max_(max),
      min_(min),
      precision_(precision),
      max_delta_(acomms::NaN)
{ }

        
void goby::transitional::DCCLMessageVarFloat::initialize_specific()
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

void goby::transitional::DCCLMessageVarFloat::pre_encode(DCCLMessageVal& v)
{
    double r;
    if(v.get(r)) v = util::unbiased_round(r,precision_);
}

