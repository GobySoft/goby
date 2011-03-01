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

#include <boost/foreach.hpp>

#include "message_algorithms.h"
#include "message_val.h"
#include "message.h"
#include "dccl_exception.h"

#include "goby/util/string.h"

goby::acomms::DCCLAlgorithmPerformer* goby::acomms::DCCLAlgorithmPerformer::inst_ = 0;

// singleton class, use this to get pointer
goby::acomms::DCCLAlgorithmPerformer* goby::acomms::DCCLAlgorithmPerformer::getInstance()
{
    if(!inst_) inst_ = new DCCLAlgorithmPerformer();

    return(inst_);
}

void goby::acomms::DCCLAlgorithmPerformer::algorithm(DCCLMessageVal& in, unsigned array_index, const std::string& algorithm, const std::map<std::string,std::vector<DCCLMessageVal> >& vals)
{
    if(in.empty()) return;

// algo_name:ref_variable_name1:ref_variable_name2...
    
    std::vector<std::string>ref_vars = util::explode(algorithm, ':', true);

    std::string alg;
    std::vector<DCCLMessageVal> tied_vals;

    for(std::vector<std::string>::size_type i = 1, n = ref_vars.size();
        i < n;
        ++i)
    {
        std::map<std::string, std::vector<DCCLMessageVal> >::const_iterator it = vals.find(ref_vars[i]);
        if(it != vals.end())
        {
            if(array_index < it->second.size())
                tied_vals.push_back(it->second[array_index]);
            else
                tied_vals.push_back(it->second[0]);
        }
    }
    
    if(ref_vars.size() > 0)
        alg = ref_vars[0];
    
    // short form for simple algorithms
    if (adv_map1_.count(alg))
    { adv_map1_.find(alg)->second(in); }
    // longer form for more demanding algorithms
    else if (adv_map2_.count(alg))
    { adv_map2_.find(alg)->second(in, tied_vals); }

}

// check validity of algorithm name and references
void goby::acomms::DCCLAlgorithmPerformer::check_algorithm(const std::string& alg, const DCCLMessage& msg)
{
    if(alg.empty()) return;
    
    std::vector<std::string>ref_vars = util::explode(alg, ':', true);

    // check if the algorithm exists
    // but ignore if no algorithms loaded (to use for testing tools)
    if ((adv_map1_.size() || adv_map2_.size()) && !adv_map1_.count(ref_vars.at(0)) && !adv_map2_.count(ref_vars.at(0)))
        throw(DCCLException(std::string(msg.get_display() + "unknown algorithm defined: " + ref_vars.at(0))));

    
    for(std::vector<std::string>::size_type i = 1, n = ref_vars.size();
        i < n;
        ++i)
    {
        bool ref_found = false;
        BOOST_FOREACH(const boost::shared_ptr<DCCLMessageVar> mv, msg.header_const())
        {
            if(ref_vars[i] == mv->name())
            {
                ref_found =true;
                break;
            }
        }
        
        BOOST_FOREACH(const boost::shared_ptr<DCCLMessageVar> mv, msg.layout_const())
        {
            if(ref_vars[i] == mv->name())
            {
                ref_found =true;
                break;
            }
        }

        if(!ref_found)
            throw(DCCLException(std::string(msg.get_display() + "no such reference message variable " + ref_vars.at(i) + " used in algorithm: " + ref_vars.at(0))));
    }
}
