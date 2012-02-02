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

#include "goby/util/as.h"
#include "goby/common/logger.h"
#include "message.h"
#include "goby/acomms/dccl/dccl.h"
#include "goby/acomms/dccl/dccl_exception.h"
#include "goby/acomms/dccl/dccl_field_codec.h"
#include "goby/acomms/dccl/dccl_field_codec_manager.h"

using goby::util::as;
using goby::glog;


goby::transitional::DCCLMessage::DCCLMessage():size_(0),
                         trigger_number_(1),
                         id_(0),
                         trigger_time_(0.0),
                         repeat_enabled_(false),
                         repeat_(1)
{
    header_.resize(DCCL_NUM_HEADER_PARTS);
    header_[HEAD_CCL_ID] =
        boost::shared_ptr<DCCLMessageVar>(new DCCLMessageVarCCLID());
    header_[HEAD_DCCL_ID] =
        boost::shared_ptr<DCCLMessageVar>(new DCCLMessageVarDCCLID());
    header_[HEAD_TIME] =
        boost::shared_ptr<DCCLMessageVar>(new DCCLMessageVarTime());
    header_[HEAD_SRC_ID] =
        boost::shared_ptr<DCCLMessageVar>(new DCCLMessageVarSrc());
    header_[HEAD_DEST_ID] =
        boost::shared_ptr<DCCLMessageVar>(new DCCLMessageVarDest());
    header_[HEAD_MULTIMESSAGE_FLAG] =
        boost::shared_ptr<DCCLMessageVar>(new DCCLMessageVarMultiMessageFlag());
    header_[HEAD_BROADCAST_FLAG] =
        boost::shared_ptr<DCCLMessageVar>(new DCCLMessageVarBroadcastFlag());
    header_[HEAD_UNUSED] =
        boost::shared_ptr<DCCLMessageVar>(new DCCLMessageVarUnused());
}

    
// add a new message_var to the current messages vector
void goby::transitional::DCCLMessage::add_message_var(const std::string& type)
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
void goby::transitional::DCCLMessage::add_publish()
{
    DCCLPublish p;
    publishes_.push_back(p);
}

// a number of tasks to perform after reading in an entire <message> from
// the xml file
void goby::transitional::DCCLMessage::preprocess()
{
    // calculate number of repeated messages that will fit and put this in `repeat_`.
    if(repeat_enabled_)
    {
        glog.is(goby::common::logger::WARN) && glog << "<repeat> is deprecated and ignored in this version of Goby. Simply post a message several times." << std::endl;
    }

//    body_bits_ = calc_total_size();

    // initialize header vars
    BOOST_FOREACH(boost::shared_ptr<DCCLMessageVar> mv, header_)
        mv->initialize(*this);
    BOOST_FOREACH(boost::shared_ptr<DCCLMessageVar> mv, layout_)
        mv->initialize(*this);
    
    // iterate over publishes_
    BOOST_FOREACH(DCCLPublish& p, publishes_)
        p.initialize(*this);
    
    // set incoming_var / outgoing_var if not set
    if(in_var_ == "")
        in_var_ = "IN_" + boost::to_upper_copy(name_) + "_HEX_" + as<std::string>(size_) + "B";
    if(out_var_ == "")
        out_var_ = "OUT_" + boost::to_upper_copy(name_) + "_HEX_" + as<std::string>(size_) + "B";

    
}

void goby::transitional::DCCLMessage::set_repeat_array_length()
{
    // set array_length_ for repeated messages for all DCCLMessageVars
    BOOST_FOREACH(boost::shared_ptr<DCCLMessageVar> mv, layout_)
        mv->set_array_length(repeat_);
}

unsigned goby::transitional::DCCLMessage::calc_total_size()
{
    boost::shared_ptr<acomms::DCCLFieldCodecBase> codec =
        acomms::DCCLFieldCodecManager::find(descriptor_, descriptor_->options().GetExtension(goby::msg).dccl().codec());
    unsigned u = 0;
    codec->base_max_size(&u, descriptor_, acomms::DCCLFieldCodecBase::BODY);
    return u;
}


std::map<std::string, std::string> goby::transitional::DCCLMessage::message_var_names() const
{
    std::map<std::string, std::string> s;
    BOOST_FOREACH(const boost::shared_ptr<DCCLMessageVar> mv, layout_)
        s.insert(std::pair<std::string, std::string>(mv->name(), type_to_string(mv->type())));
    return s;
}

    

void goby::transitional::DCCLMessage::set_head_defaults(std::map<std::string, std::vector<DCCLMessageVal> >& in, unsigned modem_id)
{
    for (std::vector< boost::shared_ptr<DCCLMessageVar> >::iterator it = header_.begin(),
             n = header_.end();
         it != n;
         ++it)
    {
        (*it)->set_defaults(in, modem_id, id_);
    }
}

boost::shared_ptr<goby::transitional::DCCLMessageVar> goby::transitional::DCCLMessage::name2message_var(const std::string& name) const 
{
    BOOST_FOREACH(boost::shared_ptr<DCCLMessageVar> mv, layout_)
    {
        if(mv->name() == name) return mv;
    }    
    BOOST_FOREACH(boost::shared_ptr<DCCLMessageVar> mv, header_)
    {
        if(mv->name() == name) return mv;
    }

    throw goby::acomms::DCCLException(std::string("DCCL: no such name \"" + name + "\" found in <layout> or <header>"));
    
    return boost::shared_ptr<DCCLMessageVar>();
}

// Added in Goby2 for transition to Protobuf structure
void goby::transitional::DCCLMessage::write_schema_to_dccl2(std::ofstream* proto_file, const goby::transitional::protobuf::QueueConfig& queue_cfg)
{
    
    *proto_file << "message " << name_ << " { " << std::endl;
    *proto_file << "\t" << "option (goby.msg).dccl.id = " << id_ << ";" << std::endl;
    *proto_file << "\t" << "option (goby.msg).dccl.max_bytes = " << size_ << ";" << std::endl;
    
    int sequence_number = 0;
    
    
    
    header_[HEAD_TIME]->write_schema_to_dccl2(proto_file, ++sequence_number);
    header_[HEAD_SRC_ID]->write_schema_to_dccl2(proto_file, ++sequence_number);
    header_[HEAD_DEST_ID]->write_schema_to_dccl2(proto_file, ++sequence_number);
    
    BOOST_FOREACH(boost::shared_ptr<DCCLMessageVar> mv, layout_)
        mv->write_schema_to_dccl2(proto_file, ++sequence_number);

    if(queue_cfg.has_ack())
        *proto_file << "\t" <<  "option (goby.msg).queue.ack = " << std::boolalpha << queue_cfg.ack() << ";" << std::endl;

    if(queue_cfg.has_blackout_time())
        *proto_file << "\t" <<  "option (goby.msg).queue.blackout_time = " << queue_cfg.blackout_time() << ";" << std::endl;

    if(queue_cfg.has_max_queue())
        *proto_file << "\t" <<  "option (goby.msg).queue.max_queue = " << queue_cfg.max_queue() << ";" << std::endl;

    if(queue_cfg.has_newest_first())
        *proto_file << "\t" <<  "option (goby.msg).queue.newest_first = " << std::boolalpha << queue_cfg.newest_first() << ";" << std::endl;
      
    if(queue_cfg.has_value_base())
        *proto_file << "\t" <<  "option (goby.msg).queue.value_base = " << queue_cfg.value_base() << ";" << std::endl;
    
    if(queue_cfg.has_ttl())
        *proto_file << "\t" <<  "option (goby.msg).queue.ttl = " << queue_cfg.ttl() << ";" << std::endl;
    
    *proto_file << "} " << std::endl;
}

