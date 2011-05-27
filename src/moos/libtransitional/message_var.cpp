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

#include "goby/util/string.h"
#include "goby/moos/libmoos_util/moos_string.h"

#include "message_var.h"
#include "message.h"
#include "message_val.h"
#include "goby/acomms/libdccl/dccl_constants.h"
#include "message_algorithms.h"

goby::transitional::DCCLMessageVar::DCCLMessageVar()
    : array_length_(1),
      is_key_frame_(true),
      source_set_(false),
      ap_(DCCLAlgorithmPerformer::getInstance())
{ }

void goby::transitional::DCCLMessageVar::initialize(const DCCLMessage& msg)
{
    // add trigger_var_ as source_var for any message_vars without a source
    if(!source_set_)
        source_var_ = msg.trigger_var();

    BOOST_FOREACH(const std::string& alg, algorithms_)
        ap_->check_algorithm(alg, msg);
    
    initialize_specific();

}

void goby::transitional::DCCLMessageVar::set_defaults(std::map<std::string,std::vector<DCCLMessageVal> >& vals, unsigned modem_id, unsigned id)
{
    vals[name_].resize(array_length_);    

    std::vector<DCCLMessageVal>& vm = vals[name_];

    for(std::vector<DCCLMessageVal>::size_type i = 0, n = vm.size(); i < n; ++i)
        set_defaults_specific(vm[i], modem_id, id);

}


    
void goby::transitional::DCCLMessageVar::var_pre_encode(
    const std::map<std::string,std::vector<DCCLMessageVal> >& in_vals,
    std::map<std::string,std::vector<DCCLMessageVal> >& out_vals)
{    
    // ensure that every DCCLMessageVar has the full number of (maybe blank) DCCLMessageVals
    out_vals[name_].resize(array_length_);
    
    // modify the original vals to be used before running algorithms and encoding
    for(std::vector<DCCLMessageVal>::size_type i = 0, n = out_vals[name_].size(); i < n; ++i)
        pre_encode(out_vals[name_][i]);
    
    std::vector<DCCLMessageVal>& vm = out_vals[name_];
    
    for(std::vector<DCCLMessageVal>::size_type i = 0, n = vm.size(); i < n; ++i)
    {
        for(std::vector<std::string>::size_type j = 0, m = algorithms_.size(); j < m; ++j)
            ap_->algorithm(vm[i], i, algorithms_[j], in_vals);
    }
    
}

void goby::transitional::DCCLMessageVar::var_post_decode(
    const std::map<std::string,std::vector<DCCLMessageVal> >& in_vals,
    std::map<std::string,std::vector<DCCLMessageVal> >& out_vals)
{    
    // modify the original vals to be used before running algorithms and encoding
    out_vals[name_].resize(array_length_);
    for(std::vector<DCCLMessageVal>::size_type i = 0, n = out_vals[name_].size(); i < n; ++i)
        post_decode(out_vals[name_][i]);
}



void goby::transitional::DCCLMessageVar::read_pubsub_vars(std::map<std::string,std::vector<DCCLMessageVal> >& vals,
                                        const std::map<std::string,std::vector<DCCLMessageVal> >& in)
{
    const std::map<std::string, std::vector<goby::transitional::DCCLMessageVal> >::const_iterator it =
        in.find(source_var_);
    
    if(it != in.end())
    {
        const std::vector<DCCLMessageVal>& vm = it->second;

        BOOST_FOREACH(DCCLMessageVal val, vm)
        {
            switch(val.type())
            {
                case cpp_string:
                    val = parse_string_val(val);
                    break;
                    
                default:
                    break;
            }

            // if we're expecting a vector,
            // split up vector quantities and add to vector
            if(array_length_ > 1)
            {
                std::string sval = val;
                std::vector<std::string> vec;
                boost::split(vec, sval, boost::is_any_of(","));
                BOOST_FOREACH(const std::string& s, vec)
                    vals[name_].push_back(s);
            }
            else // otherwise just use the value as is
            {
                vals[name_] = val;
            }
        }        
    }
}


// deal with cases where key=value exists within the string
std::string goby::transitional::DCCLMessageVar::parse_string_val(const std::string& sval)
{
    std::string pieceval;

    // is the parameter part of the std::string (as opposed to being the std::string)
    // that is, in_str is true if "key=value" is part of the string, rather
    // than the std::string simply being "value"
    bool in_str = false;
        
    // see if the parameter is *in* the string, if so put it in pieceval
    // use source_key if specified, otherwise try the name
    std::string subkey = (source_key_ == "") ? name_ : source_key_;
        
    in_str = moos::val_from_string(pieceval, sval, subkey);        
    //pick the substring from the string
    if(in_str)
        return pieceval;
    else
        return sval;
}


void goby::transitional::DCCLMessageVar::write_schema_to_dccl2(std::ofstream* proto_file, int sequence_number)
{
    *proto_file << "\t";

    if(array_length_ == 1)
        *proto_file << "optional ";
    else
        *proto_file  << "repeated ";

    std::string enum_name;
    if(type() == dccl_enum)
    {
        enum_name = name_ + "Enum";
        enum_name[0] = toupper(enum_name[0]);
        *proto_file << enum_name;
    }
    else
    {
        *proto_file << type_to_protobuf_type(type());
    }

    *proto_file << " " << name_ << " = " << sequence_number;


    int count = 0;
    try
    {
        double max_tmp = max();

        *proto_file << (count ? ", " : " [");
        ++count;
        *proto_file << "(dccl.max)=" << max_tmp;
    }
    catch(...) { }
    try
    {
        double min_tmp = min();
        *proto_file << (count ? ", " : " [");
        ++count;
        *proto_file << "(dccl.min)=" << min_tmp;
    }
    catch(...) { }
    try
    {
        int precision_tmp = precision();
        *proto_file << (count ? ", " : " [");
        ++count;
        *proto_file << "(dccl.precision)=" << precision_tmp;
    }
    catch(...) { }
    try
    {
        unsigned max_length_tmp = max_length();
        *proto_file << (count ? ", " : " [");
        ++count;
        *proto_file << "(dccl.max_length)=" << max_length_tmp;        
    }
    catch(...) { }
    try
    {
        unsigned num_bytes_tmp = num_bytes();
        *proto_file << (count ? ", " : " [");
        ++count;
        *proto_file << "(dccl.max_length)=" << num_bytes_tmp;        
    }
    catch(...) { }
    try
    {
        std::string static_val_tmp = static_val();
        *proto_file << (count ? ", " : " [");
        ++count;
        *proto_file << "(dccl.static_value)=\"" << static_val_tmp << "\", " << "(dccl.codec)=\"_static\"";      
    }
    catch(...) { }

    if(array_length_ != 1)
    {
        *proto_file << (count ? ", " : " [");
        ++count;
        *proto_file << "(dccl.max_repeat)=" << array_length_;        

    }

    
    std::string more_options = additional_option_extensions();
    if(!more_options.empty())
    {
        *proto_file << (count ? ", " : " [");
        ++count;
        *proto_file << more_options;
    }
    
    
 
    if(count)
        *proto_file << "]";

    *proto_file << ";" <<  std::endl;

    if(type() == dccl_enum)
    {
        *proto_file << "\t" << "enum " << enum_name << "{ \n";

        int enum_value = 0;
        BOOST_FOREACH(const std::string& e, *enums())
            *proto_file << "\t\t " << boost::to_upper_copy(name_) << "_" << e << " = " << enum_value++ << "; \n";

        *proto_file << "\t} \n";
    }
}

