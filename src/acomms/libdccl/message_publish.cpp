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

using acomms_util::NaN;

void dccl::Publish::initialize(std::vector<MessageVar>& layout)
{
    // add names for any <all/> publishes and empty std::vector for algorithms
    if(use_all_names_)
    {
        BOOST_FOREACH(MessageVar& mv, layout)
        {
            add_name(mv.name());
            // add an empty std::vector for algorithms (no algorithms allowed for <all/> tag)
            add_algorithms(std::vector<std::string>());
        }
    }
        
    // add ids for all publish names
    BOOST_FOREACH(std::string& name, names_)
    {
        bool found_id = false;

        size_t k = 0;
        BOOST_FOREACH(MessageVar& mv, layout)
        {
            if(mv.name() == name)
            {
                add_id(k);
                found_id = true;
                break;
            }
            ++k;
        }
        if(!found_id)
        {
            throw std::runtime_error(std::string("DCCL: no such name \"" + name + "\" found in layout for publish value."));
        }
    }

    int format_count = 0;
    // add format if publish doesn't have one  
    if(!format_set_)
    {
        std::string format_str;
        for (std::vector<std::string>::size_type j = 0, m = names_.size(); j < m; ++j) 
        {
            //MessageVar& mv = layout[ids_[j]];
                
            if (m > 1)
            {
                if (j)
                    format_str += ",";

                // allows you to use the same message var twice but gives a unique name based on the algorithms used
                unsigned size = algorithms_[j].size();
                if(count(names_.begin(), names_.end(), names_[j]) > 1 && size )
                {
                    for(unsigned i = 0; i < size; ++i)
                        format_str += algorithms_[j][i];
                            
                    format_str += "(" + names_[j] + ")=";
                }
                else
                    format_str += names_[j] + "=";           
            }
            
            //DCCLType type = mv.type();
            ++format_count;
            std::stringstream ss;
            ss << "%" << format_count << "%";
            format_str += ss.str();
        }
        format_ = format_str;
    } 
}


void dccl::Publish::fill_format(std::map<std::string,MessageVal>& vals,
                                std::vector<MessageVar>& layout,
                                std::string& key,
                                std::string& value)
{
    std::string filled_value;
    // format is a boost library class for replacing printf and its ilk
    boost::format f;
        
    // tack on the moos variable with a space separator (no space allowed in moos var
    // so we can use format on that too
    std::string input_format = var_ + " " + format_;

    try
    {            
        f.parse(input_format);
            
        // iterate over the message_vars and fill in the format field
        for (std::vector<std::string>::size_type k = 0, o = names_.size(); k < o; ++k)
        {
            MessageVar& mv = layout[ids_[k]];
            DCCLType type = mv.type();
            MessageVal v = vals[names_[k]];

            std::vector<std::string>::size_type num_algs = algorithms_[k].size();
            for(std::vector<std::string>::size_type l = 0; l < num_algs; ++l)
                ap_->algorithm(v, algorithms_[k][l], vals);

            
            std::string s;
            switch(type)
            {
                default:
                    break;

                case dccl_int:
                case dccl_float:
                    if(!v.val(s))
                        s = "nan";
                    break;


                case dccl_static:
                case dccl_enum:
                case dccl_string:
                case dccl_bool:
                    if(!v.val(s))
                        s = "";
                    break;
                    
            }                
                
            f % s;
        }

        // try/catch here?
        filled_value = f.str();
    }
    catch (std::exception& e)
    {
        throw std::runtime_error(std::string(e.what() + (std::string)" decode failed. check format string for this <publish />: \n" + get_display()));
    }

    // split filled_value back into variable and value
    std::vector<std::string> split_vec;
    boost::split(split_vec, filled_value, boost::is_any_of(" "));
        
    key = split_vec.at(0);
    value = split_vec.at(1);
        
    for(std::vector<std::string>::size_type it = 2, n = split_vec.size(); it < n; ++it)
        value += " " + split_vec.at(it);
}

    

    
std::string dccl::Publish::get_display() const
{
    std::stringstream ss;
    
    ss << "\t(" << type_;
    ss << ")moos_var: {" << var_ << "}" << std::endl;
    ss << "\tvalue: \"" << format_ << "\"" << std::endl;
    ss << "\tmessage_vars:" << std::endl;
    for (std::vector<std::string>::size_type j = 0, m = names_.size(); j < m; ++j)
    {
        ss << "\t\t" << (j+1) << ": " << names_[j];

        for(std::vector<std::string>::size_type k = 0, o = algorithms_[j].size(); k < o; ++k)
        {
            if(!k)
                ss << " | algorithm(s): ";
            else
                ss << ", ";
            ss << algorithms_[j][k];                
        }
            
        ss << std::endl;
    }

    return ss.str();
}

// overloaded <<
std::ostream& dccl::operator<< (std::ostream& out, const Publish& publish)
{
    out << publish.get_display();
    return out;
}
