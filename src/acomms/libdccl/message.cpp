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

#include <boost/lexical_cast.hpp>
#include <boost/foreach.hpp>

#include "message.h"

goby::acomms::DCCLMessage::DCCLMessage():size_(0),
                         trigger_number_(1), 
                         body_bits_(0),
                         id_(0),
                         trigger_time_(0.0),
                         repeat_enabled_(false),
                         repeat_(1)
{
    header_.resize(DCCL_NUM_HEADER_PARTS);
    header_[head_ccl_id] =
        boost::shared_ptr<DCCLMessageVar>(new DCCLMessageVarCCLID());
    header_[head_dccl_id] =
        boost::shared_ptr<DCCLMessageVar>(new DCCLMessageVarDCCLID());
    header_[head_time] =
        boost::shared_ptr<DCCLMessageVar>(new DCCLMessageVarTime());
    header_[head_src_id] =
        boost::shared_ptr<DCCLMessageVar>(new DCCLMessageVarSrc());
    header_[head_dest_id] =
        boost::shared_ptr<DCCLMessageVar>(new DCCLMessageVarDest());
    header_[head_multimessage_flag] =
        boost::shared_ptr<DCCLMessageVar>(new DCCLMessageVarMultiMessageFlag());
    header_[head_broadcast_flag] =
        boost::shared_ptr<DCCLMessageVar>(new DCCLMessageVarBroadcastFlag());
    header_[head_unused] =
        boost::shared_ptr<DCCLMessageVar>(new DCCLMessageVarUnused());
}

    
// add a new message_var to the current messages vector
void goby::acomms::DCCLMessage::add_message_var(const std::string& type)
{
    if(type == "static")
        layout_.push_back(boost::shared_ptr<DCCLMessageVar>(new DCCLMessageVarStatic()));
    else if(type == "int")
        layout_.push_back(boost::shared_ptr<DCCLMessageVar>(new DCCLMessageVarInt()));
    else if(type == "string")
        layout_.push_back(boost::shared_ptr<DCCLMessageVar>(new DCCLMessageVarString()));
    else if(type == "float")
        layout_.push_back(boost::shared_ptr<DCCLMessageVar>(new DCCLMessageVarFloat()));
    else if(type == "enum")
        layout_.push_back(boost::shared_ptr<DCCLMessageVar>(new DCCLMessageVarEnum()));
    else if(type == "bool")
        layout_.push_back(boost::shared_ptr<DCCLMessageVar>(new DCCLMessageVarBool()));
    else if(type == "hex")
        layout_.push_back(boost::shared_ptr<DCCLMessageVar>(new DCCLMessageVarHex()));
}

// add a new publish, i.e. a set of parameters to publish
// upon receipt of an incoming (hex) message
void goby::acomms::DCCLMessage::add_publish()
{
    DCCLPublish p;
    publishes_.push_back(p);
}

// a number of tasks to perform after reading in an entire <message> from
// the xml file
void goby::acomms::DCCLMessage::preprocess()
{
    if(requested_bytes_total() <= bytes_head())
        throw(std::runtime_error(std::string("<size> must be larger than the header size of " + boost::lexical_cast<std::string>(bytes_head()))));

    // calculate number of repeated messages that will fit and put this in `repeat_`.
    if(repeat_enabled_)
    {
        BOOST_FOREACH(boost::shared_ptr<DCCLMessageVar> mv, layout_)
        {
        if(mv->array_length() != 1)
            throw(std::runtime_error("<repeat> is not allowed on messages with arrays (<array_length> not 1)"));
        }
        
        // crank up the repeat until we go over
        while(calc_total_size() <= requested_bits_body())
        {
            ++repeat_;
            set_repeat_array_length();
        }

        // back off one
        --repeat_;
        set_repeat_array_length();
    }

    body_bits_ = calc_total_size();

    // initialize header vars
    BOOST_FOREACH(boost::shared_ptr<DCCLMessageVar> mv, header_)
        mv->initialize(trigger_var_);

    
    if(body_bits_ > requested_bits_body() || repeat_ == 0)
    {
        throw std::runtime_error(std::string("DCCL: " + get_display() + "the message [" + name_ + "] will not fit within specified size. remove parameters, tighten bounds, or increase allowed size. details of the offending message are printed above."));
    }

    // iterate over publishes_
    BOOST_FOREACH(DCCLPublish& p, publishes_)
        p.initialize(*this);
    
    // set incoming_var / outgoing_var if not set
    if(in_var_ == "")
        in_var_ = "IN_" + boost::to_upper_copy(name_) + "_HEX_" + boost::lexical_cast<std::string>(size_) + "B";
    if(out_var_ == "")
        out_var_ = "OUT_" + boost::to_upper_copy(name_) + "_HEX_" + boost::lexical_cast<std::string>(size_) + "B";   
}

void goby::acomms::DCCLMessage::set_repeat_array_length()
{
    // set array_length_ for repeated messages for all DCCLMessageVars
    BOOST_FOREACH(boost::shared_ptr<DCCLMessageVar> mv, layout_)
        mv->set_array_length(repeat_);
}

unsigned goby::acomms::DCCLMessage::calc_total_size()
{
    unsigned body_bits = 0;
    // iterate over layout_
    BOOST_FOREACH(boost::shared_ptr<DCCLMessageVar> mv, layout_)
    {
        mv->initialize(trigger_var_);
        // calculate total bits for the message from the bits for each message_var
        body_bits += mv->calc_total_size();
    }
    return body_bits;
}



std::map<std::string, std::string> goby::acomms::DCCLMessage::message_var_names() const
{
    std::map<std::string, std::string> s;
    BOOST_FOREACH(const boost::shared_ptr<DCCLMessageVar> mv, layout_)
        s.insert(std::pair<std::string, std::string>(mv->name(), type_to_string(mv->type())));
    return s;
}

std::set<std::string> goby::acomms::DCCLMessage::get_pubsub_encode_vars()
{
    std::set<std::string> s = get_pubsub_src_vars();
    if(trigger_type_ == "publish")
        s.insert(trigger_var_);
    return s;
}

std::set<std::string> goby::acomms::DCCLMessage::get_pubsub_decode_vars()
{
    std::set<std::string> s;
    s.insert(in_var_);
    return s;
}
    
std::set<std::string> goby::acomms::DCCLMessage::get_pubsub_all_vars()
{
    std::set<std::string> s_enc = get_pubsub_encode_vars();
    std::set<std::string> s_dec = get_pubsub_decode_vars();

    std::set<std::string>& s = s_enc;        
    s.insert(s_dec.begin(), s_dec.end());
        
    return s;
}
    
std::set<std::string> goby::acomms::DCCLMessage::get_pubsub_src_vars()
{
    std::set<std::string> s;

    BOOST_FOREACH(boost::shared_ptr<DCCLMessageVar> mv, layout_)
        s.insert(mv->source_var());
    BOOST_FOREACH(boost::shared_ptr<DCCLMessageVar> mv, header_)
        s.insert(mv->source_var());
    
    return s;
}

    
void goby::acomms::DCCLMessage::body_encode(std::string& body, std::map<std::string, std::vector<DCCLMessageVal> >& in)
{
    boost::dynamic_bitset<unsigned char> body_bits(bytes2bits(used_bytes_body()));

    // 1. encode each variable into the bitset
    for (std::vector< boost::shared_ptr<DCCLMessageVar> >::iterator it = layout_.begin(),
             n = layout_.end();
         it != n;
         ++it)
    {
        (*it)->var_encode(in, body_bits);
    }
    
    // 2. bitset to string
    bitset2string(body_bits, body);

    // 3. strip all the ending zeros
    body.resize(body.find_last_not_of(char(0))+1);
}

void goby::acomms::DCCLMessage::body_decode(std::string& body, std::map<std::string, std::vector<DCCLMessageVal> >& out)
{
    boost::dynamic_bitset<unsigned char> body_bits(bytes2bits(used_bytes_body()));       
    
    // 3. resize the string to the proper size
    body.resize(used_bytes_body());
    
    // 2. convert string to bitset
    string2bitset(body_bits, body);
    
    // 1. pull the bits off the message in the reverse that they were put on
    for (std::vector< boost::shared_ptr<DCCLMessageVar> >::reverse_iterator it = layout_.rbegin(),
             n = layout_.rend();
         it != n;
         ++it)
    {
        (*it)->var_decode(out, body_bits);
    }
}

void goby::acomms::DCCLMessage::set_head_defaults(std::map<std::string, std::vector<DCCLMessageVal> >& in, unsigned modem_id)
{
    for (std::vector< boost::shared_ptr<DCCLMessageVar> >::iterator it = header_.begin(),
             n = header_.end();
         it != n;
         ++it)
    {
        (*it)->set_defaults(in, modem_id, id_);
    }
}

void goby::acomms::DCCLMessage::head_encode(std::string& head, std::map<std::string, std::vector<DCCLMessageVal> >& in)
{    
    boost::dynamic_bitset<unsigned char> head_bits(bytes2bits(DCCL_NUM_HEADER_BYTES));

    
    for (std::vector< boost::shared_ptr<DCCLMessageVar> >::iterator it = header_.begin(),
             n = header_.end();
         it != n;
         ++it)
    {
        (*it)->var_encode(in, head_bits);
    }
    
    bitset2string(head_bits, head);
}

void goby::acomms::DCCLMessage::head_decode(const std::string& head, std::map<std::string, std::vector<DCCLMessageVal> >& out)
{
    boost::dynamic_bitset<unsigned char> head_bits(bytes2bits(DCCL_NUM_HEADER_BYTES));
    string2bitset(head_bits, head);

    for (std::vector< boost::shared_ptr<DCCLMessageVar> >::reverse_iterator it = header_.rbegin(), n = header_.rend();
         it != n;
         ++it)
    {
        (*it)->var_decode(out, head_bits);
    }
}

boost::shared_ptr<goby::acomms::DCCLMessageVar> goby::acomms::DCCLMessage::name2message_var(const std::string& name)
{
    BOOST_FOREACH(boost::shared_ptr<DCCLMessageVar> mv, layout_)
    {
        if(mv->name() == name) return mv;
    }    
    BOOST_FOREACH(boost::shared_ptr<DCCLMessageVar> mv, header_)
    {
        if(mv->name() == name) return mv;
    }

    throw std::runtime_error(std::string("DCCL: no such name \"" + name + "\" found in <layout> or <header>"));
    
    return boost::shared_ptr<DCCLMessageVar>();
}

////////////////////////////
// VISUALIZATION
////////////////////////////

    
// a long visual display of all the parameters for a DCCLMessage
std::string goby::acomms::DCCLMessage::get_display() const
{
    const unsigned int num_stars = 20;

    bool is_moos = !trigger_type_.empty();
        
    std::stringstream ss;
    ss << std::string(num_stars, '*') << std::endl;
    ss << "message " << id_ << ": {" << name_ << "}" << std::endl;

    if(is_moos)
    {
        ss << "trigger_type: {" << trigger_type_ << "}" << std::endl;
    
    
        if(trigger_type_ == "publish")
        {
            ss << "trigger_var: {" << trigger_var_ << "}";
            if (trigger_mandatory_ != "")
                ss << " must contain string \"" << trigger_mandatory_ << "\"";
            ss << std::endl;
        }
        else if(trigger_type_ == "time")
        {
            ss << "trigger_time: {" << trigger_time_ << "}" << std::endl;
        }
    
        ss << "outgoing_hex_var: {" << out_var_ << "}" << std::endl;
        ss << "incoming_hex_var: {" << in_var_ << "}" << std::endl;
        
        if(repeat_enabled_)
            ss << "repeated " << repeat_ << " times." << std::endl;
        
    }
    
    ss << "requested size {bytes} [bits]: {" << requested_bytes_total() << "} [" << requested_bits_total() << "]" << std::endl;
    ss << "actual size {bytes} [bits]: {" << used_bytes_total() << "} [" << used_bits_total() << "]" << std::endl;

    ss << ">>>> HEADER <<<<" << std::endl;
    BOOST_FOREACH(const boost::shared_ptr<DCCLMessageVar> mv, header_)
        ss << *mv;
    
    ss << ">>>> LAYOUT (message_vars) <<<<" << std::endl;

    BOOST_FOREACH(const boost::shared_ptr<DCCLMessageVar> mv, layout_)
        ss << *mv;
    
    if(is_moos)
    {

        ss << ">>>> PUBLISHES <<<<" << std::endl;
        
        BOOST_FOREACH(const DCCLPublish& p, publishes_)
            ss << p;
    }
    
    ss << std::string(num_stars, '*') << std::endl;

        
    return ss.str();
}

// a much shorter rundown of the Message parameters
std::string goby::acomms::DCCLMessage::get_short_display() const
{
    std::stringstream ss;

    ss << name_ <<  ": ";

    bool is_moos = !trigger_type_.empty();
    if(is_moos)
    {
        ss << "trig: ";
        
        if(trigger_type_ == "publish")
            ss << trigger_var_;
        else if(trigger_type_ == "time")
            ss << trigger_time_ << "s";
        ss << " | out: " << out_var_;
        ss << " | in: " << in_var_ << " | ";
    }

    ss << "size: {" << used_bytes_total() << "/" << requested_bytes_total() << "B} [" <<  used_bits_total() << "/" << requested_bits_total() << "b] | message var N: " << layout_.size() << std::endl;

    return ss.str();
}
    
// overloaded <<
std::ostream& goby::acomms::operator<< (std::ostream& out, const DCCLMessage& message)
{
    out << message.get_display();
    return out;
}
