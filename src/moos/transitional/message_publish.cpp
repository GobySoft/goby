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

#include "message_publish.h"
#include "goby/acomms/dccl/dccl_exception.h"
#include "message.h"



void goby::transitional::DCCLPublish::initialize(const DCCLMessage& msg)
{
    repeat_ = msg.repeat();

    // check and add all publish names grabbed by the xml parser
    BOOST_FOREACH(const std::string& name, names_)
        add_message_var(msg.name2message_var(name));


    BOOST_FOREACH(const std::vector<std::string>& algs, algorithms_)
    {
        BOOST_FOREACH(const std::string& alg, algs)
            ap_->check_algorithm(alg, msg);
    }
    
    
    // add names for any <all/> publishes and empty std::vector for algorithms
    if(use_all_names_)
    {
        BOOST_FOREACH(const boost::shared_ptr<DCCLMessageVar> mv, msg.header_const())
        {
            // ignore header pieces not explicitly overloaded by the <name> tag
            if(!mv->name().empty() && !(mv->name()[0] == '_') &&
               !std::count(names_.begin(), names_.end(), mv->name()))
            {
                add_message_var(mv);
                // add an empty std::vector for algorithms (no algorithms allowed for <all/> tag)
                add_algorithms(std::vector<std::string>());
            }
        }
        
        BOOST_FOREACH(const boost::shared_ptr<DCCLMessageVar> mv, msg.layout_const())
        {
            if(!std::count(names_.begin(), names_.end(), mv->name()))
            {
                add_message_var(mv);
                // add an empty std::vector for algorithms (no algorithms allowed for <all/> tag)
                add_algorithms(std::vector<std::string>());
            }
        }
    }
    
    int format_count = 0;
    // add format if publish doesn't have one  
    if(!format_set_)
    {
        std::string format_str;
        for (std::vector<boost::shared_ptr<DCCLMessageVar> >::size_type j = 0, m = message_vars_.size(); j < m; ++j)
        {
            if (m > 1)
            {
                if (j)
                    format_str += ",";
                
                // allows you to use the same message var twice but gives a unique name based on the algorithms used
                unsigned size = algorithms_[j].size();
                if(count(message_vars_.begin(), message_vars_.end(), message_vars_[j]) > 1 && size )
                {
                    for(unsigned i = 0; i < size; ++i)
                        format_str += algorithms_[j][i];
                            
                    format_str += "(" + message_vars_[j]->name() + ")=";
                }
                else
                    format_str += message_vars_[j]->name() + "=";           
            }
            
            ++format_count;
            std::stringstream ss;

            unsigned n = (repeat_ > 1) ? 1 : message_vars_[j]->array_length();
            
            if(m > 1 && n > 1)
                ss << "{";
            
            ss << "%" << format_count << "%";
            
            if(m > 1 && n > 1)
                ss << "}";
            
            format_str += ss.str();
        }
        format_ = format_str;
    } 
}

