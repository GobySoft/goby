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
#include "message.h"

using goby::acomms::NaN;

void goby::acomms::DCCLPublish::initialize(DCCLMessage& msg)
{
    repeat_ = msg.repeat();

    // check and add all publish names grabbed by the xml parser
    BOOST_FOREACH(const std::string& name, names_)
        add_message_var(msg.name2message_var(name));

    
    // add names for any <all/> publishes and empty std::vector for algorithms
    if(use_all_names_)
    {
        BOOST_FOREACH(boost::shared_ptr<DCCLMessageVar> mv, msg.header())
        {
            // ignore header pieces not explicitly overloaded by the <name> tag
            if(!mv->name().empty() && !(mv->name()[0] == '_'))
            {
                add_message_var(mv);
                // add an empty std::vector for algorithms (no algorithms allowed for <all/> tag)
                add_algorithms(std::vector<std::string>());
            }
        }
        
        BOOST_FOREACH(boost::shared_ptr<DCCLMessageVar> mv, msg.layout())
        {
            add_message_var(mv);
            // add an empty std::vector for algorithms (no algorithms allowed for <all/> tag)
            add_algorithms(std::vector<std::string>());
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
            
            for(unsigned i = 0,
                    n = (repeat_ > 1) ? 1 : message_vars_[j]->array_length();
                i < n;
                ++i)
            {                
                ++format_count;
                std::stringstream ss;

                if(m > 1 && n > 1 && i == 0) ss << "{";
                if(i) ss << ",";
                
                ss << "%" << format_count << "%";

                if(m > 1 && n > 1 && i+1 == n) ss << "}";
                format_str += ss.str();
            }
        }
        format_ = format_str;
    } 
}


void goby::acomms::DCCLPublish::fill_format(const std::map<std::string,std::vector<DCCLMessageVal> >& vals,
                                            std::string& key,
                                            std::string& value,
                                            unsigned repeat_index)
{
    std::string filled_value;
    // format is a boost library class for replacing printf and its ilk
    boost::format f;
    
    // tack on the dest variable with a space separator (no space allowed in dest var
    // so we can use format on that too
    std::string input_format = var_ + " " + format_;

    try
    {            
        f.parse(input_format);
            
        // iterate over the message_vars and fill in the format field
        for (std::vector<boost::shared_ptr<DCCLMessageVar> >::size_type k = 0, o = message_vars_.size(); k < o; ++k)
        {
            std::vector<DCCLMessageVal> vm = vals.find(message_vars_[k]->name())->second;
            for(std::vector<DCCLMessageVal>::size_type i = (repeat_ > 1) ? repeat_index : 0,
                    n = (repeat_ > 1) ? repeat_index + 1 : vm.size();
                i < n;
                ++i)
            {
                // special case when repeating and variable has a single entry, repeat
                // that entry over all the publishes (this is used for the header
                std::vector<DCCLMessageVal>::size_type eff_index = (repeat_ > 1 && vm.size() == 1) ? 0 : i;
                
                std::vector<std::string>::size_type num_algs = algorithms_[k].size();

                // only run algorithms once on a given variable
                for(std::vector<std::string>::size_type l = 0; l < num_algs; ++l)
                    ap_->algorithm(vm[eff_index], i, algorithms_[k][l], vals);
                
                std::string s =  vm[eff_index];
                f % s;
            }
        }

        filled_value = f.str();
    }
    catch (std::exception& e)
    {
        throw std::runtime_error(std::string(e.what() + (std::string)"\n decode failed. check format string for this <publish />: \n" + get_display()));
    }

    // split filled_value back into variable and value
    std::vector<std::string> split_vec;
    boost::split(split_vec, filled_value, boost::is_any_of(" "));
        
    key = split_vec.at(0);
    value = split_vec.at(1);
        
    for(std::vector<std::string>::size_type it = 2, n = split_vec.size(); it < n; ++it)
        value += " " + split_vec.at(it);
}

    

void goby::acomms::DCCLPublish::write_publish(const std::map<std::string,std::vector<DCCLMessageVal> >& vals,
                                              std::multimap<std::string,DCCLMessageVal>& pubsub_vals)

{
    for(unsigned i = 0, n = repeat_;
        i < n;
        ++i)
    {        
        std::string out_var, out_val;
        fill_format(vals, out_var, out_val, i);
        
        // user sets to string
        if(type_ == cpp_string)
        {
            pubsub_vals.insert(std::pair<std::string, DCCLMessageVal>(out_var, out_val));
            continue;
        }
        
        // pass through a DCCLMessageVal to do the type conversions
        DCCLMessageVal mv = out_val;
        double out_dval = mv;
        if(type_ == cpp_double)
        {
            pubsub_vals.insert(std::pair<std::string, DCCLMessageVal>(out_var, out_dval));
            continue;
        }
        long out_lval = mv;    
        if(type_ == cpp_long)
        {
            pubsub_vals.insert(std::pair<std::string, DCCLMessageVal>(out_var, out_lval));
            continue;
            
        }
        bool out_bval = mv;
        if(type_ == cpp_bool)
        {
            pubsub_vals.insert(std::pair<std::string, DCCLMessageVal>(out_var, out_bval));
            continue;
        }
        
        // see if our value is numeric
        bool is_numeric = true;
        try { boost::lexical_cast<double>(out_val); }
        catch (boost::bad_lexical_cast &) { is_numeric = false; }
        
        if(!is_numeric)
            pubsub_vals.insert(std::pair<std::string, DCCLMessageVal>(out_var, out_val));
        else
            pubsub_vals.insert(std::pair<std::string, DCCLMessageVal>(out_var, out_dval));
    }
}

    
std::string goby::acomms::DCCLPublish::get_display() const
{
    std::stringstream ss;
    
    ss << "\t(" << type_to_string(type_);
    ss << ")moos_var: {" << var_ << "}" << std::endl;
    ss << "\tvalue: \"" << format_ << "\"" << std::endl;
    ss << "\tmessage_vars:" << std::endl;
    for (std::vector<boost::shared_ptr<DCCLMessageVar> >::size_type j = 0, m = message_vars_.size(); j < m; ++j)
    {
        ss << "\t\t" << (j+1) << ": " << message_vars_[j]->name();

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
std::ostream& goby::acomms::operator<< (std::ostream& out, const DCCLPublish& publish)
{
    out << publish.get_display();
    return out;
}
