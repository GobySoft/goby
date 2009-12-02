// t. schneider tes@mit.edu 12.23.08
// ocean engineering graduate student - mit / whoi joint program
// massachusetts institute of technology (mit)
// laboratory for autonomous marine sensing systems (lamss)
// 
// this is message_algorithms.cpp, part of libdccl
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

#include <boost/foreach.hpp>

#include "message_algorithms.h"
#include "message_val.h"

dccl::AlgorithmPerformer* dccl::AlgorithmPerformer::inst_ = NULL;

// singleton class, use this to get pointer
dccl::AlgorithmPerformer* dccl::AlgorithmPerformer::getInstance()
{
    if(inst_ == NULL)
        inst_ = new AlgorithmPerformer();

    return(inst_);
}

void dccl::AlgorithmPerformer::algorithm(MessageVal& in, const std::string& algorithm, const std::map<std::string,MessageVal>& vals)
{
    std::vector<std::string>params = acomms_util::explode(algorithm, ':', true);
    std::string alg = params[0];

    std::string s;
    double d;
    long l;
    bool b;

    // short form for simple algorithms
    if (str_map1_.count(alg) && in.val(s))
    {
        str_map1_.find(alg)->second(s);
        in.set(s);
    }
    else if (dbl_map1_.count(alg) && in.val(d))
    {
        dbl_map1_.find(alg)->second(d);
        in.set(d);
    }
    else if (long_map1_.count(alg) && in.val(l))
    {
        long_map1_.find(alg)->second(l);
        in.set(l);
    }
    else if (bool_map1_.count(alg) && in.val(b))
    {
        bool_map1_.find(alg)->second(b);
        in.set(b);
    }
    // longer form for more demanding algorithms
    else if (str_map3_.count(alg) && in.val(s))
    {
        str_map3_.find(alg)->second(s, params, vals);
        in.set(s);
    }
    else if (dbl_map3_.count(alg) && in.val(d))
    {
        dbl_map3_.find(alg)->second(d, params, vals);
        in.set(d);
    }
    else if (long_map3_.count(alg) && in.val(l))
    {
        long_map3_.find(alg)->second(l, params, vals);
        in.set(l);
    }
    else if (bool_map3_.count(alg) && in.val(b))
    {
        bool_map3_.find(alg)->second(b, params, vals);
        in.set(b);
    }    
}
