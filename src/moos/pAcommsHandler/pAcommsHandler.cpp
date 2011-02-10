// t. schneider tes@mit.edu 06.05.08
// ocean engineering graudate student - mit / whoi joint program
// massachusetts institute of technology (mit)
// laboratory for autonomous marine sensing systems (lamss)
// 
// this is pAcommsHandler.cpp, part of pAcommsHandler 
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

#include <cctype>

#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/numeric/conversion/cast.hpp>
#include <boost/foreach.hpp>

#include "pAcommsHandler.h"
#include "goby/util/sci.h"

using namespace goby::util::tcolor;
using goby::acomms::operator<<;
using goby::util::as;
using goby::util::glogger;
using google::protobuf::uint32;
using goby::acomms::NaN;
using goby::acomms::DCCLMessageVal;

pAcommsHandlerConfig CpAcommsHandler::cfg_;

CpAcommsHandler::CpAcommsHandler()
    : TesMoosApp(&cfg_),
      dccl_(&glogger()),
      queue_manager_(&glogger()),
      mac_(&glogger())
{
    process_configuration();

    // bind the lower level pieces of goby-acomms together
    if(driver_)
    {
        goby::acomms::bind(*driver_, queue_manager_);
        goby::acomms::bind(mac_, *driver_);

// bind our methods to the rest of the goby-acomms signals
        goby::acomms::connect(&driver_->signal_all_outgoing,
                              this, &CpAcommsHandler::modem_raw_out);
        goby::acomms::connect(&driver_->signal_all_incoming,
                              this, &CpAcommsHandler::modem_raw_in);    
        goby::acomms::connect(&driver_->signal_range_reply,
                              this, &CpAcommsHandler::modem_range_reply);
    }
    
    
    goby::acomms::connect(&queue_manager_.signal_receive,
                          this, &CpAcommsHandler::queue_incoming_data);
    goby::acomms::connect(&queue_manager_.signal_receive_ccl,
                          this, &CpAcommsHandler::queue_incoming_data);
    goby::acomms::connect(&queue_manager_.signal_ack,
                          this, &CpAcommsHandler::queue_ack);
    goby::acomms::connect(&queue_manager_.signal_data_on_demand,
                          this, &CpAcommsHandler::queue_on_demand);
    goby::acomms::connect(&queue_manager_.signal_queue_size_change,
                          this, &CpAcommsHandler::queue_qsize);
    goby::acomms::connect(&queue_manager_.signal_expire,
                          this, &CpAcommsHandler::queue_expire);

    do_subscriptions();
}

CpAcommsHandler::~CpAcommsHandler()
{}

void CpAcommsHandler::loop()
{
    dccl_loop();
    if(driver_) driver_->do_work();
    mac_.do_work();
    queue_manager_.do_work();    
}


void CpAcommsHandler::dccl_loop()
{
    std::set<unsigned> ids;
    if(dccl_.is_time_trigger(ids))
    {
        BOOST_FOREACH(unsigned id, ids)
        {
            goby::acomms::protobuf::ModemDataTransmission mm;
            pack(id, &mm);
        }
    }
    if(cfg_.tcp_share_enable() && tcp_share_server_)
    {
        std::string s;
        while(!(s = tcp_share_server_->readline()).empty())
        {
            goby::acomms::protobuf::ModemDataTransmission msg;
            parse_for_moos(s, &msg);
            
            glogger() << group("tcp") << "incoming: " << msg << std::endl;
            
            // post for others to use as needed in MOOS

            goby::acomms::DCCLHeaderDecoder decoder(msg.data());
            unsigned dccl_id = decoder[goby::acomms::HEAD_DCCL_ID]; 
            std::string in_var = dccl_.get_incoming_hex_var(dccl_id);
            
            publish(in_var,s);
            unpack(msg);
        }
    }
    
}


void CpAcommsHandler::do_subscriptions()
// RegisterVariables: register for variables we want to get mail for
{        
    // ping
    subscribe(MOOS_VAR_COMMAND_RANGING, &CpAcommsHandler::handle_ranging_request, this);

    // update comms cycle
    subscribe(MOOS_VAR_CYCLE_UPDATE, &CpAcommsHandler::handle_mac_cycle_update, this);
    subscribe(MOOS_VAR_POLLER_UPDATE, &CpAcommsHandler::handle_mac_cycle_update, this);

    
    std::set<std::string> enc_vars, dec_vars;

    BOOST_FOREACH(unsigned id, dccl_.all_message_ids())
    {
        std::set<std::string> vars = dccl_.get_pubsub_encode_vars(id);
        enc_vars.insert(vars.begin(), vars.end());
        
        vars = dccl_.get_pubsub_decode_vars(id);
        dec_vars.insert(vars.begin(), vars.end());       
    }
    BOOST_FOREACH(const std::string& s, dec_vars)
    {
        if(!s.empty())
        {
            subscribe(s, &CpAcommsHandler::dccl_inbox, this);
            
            glogger() << group("dccl_dec")
                  <<  "registering (dynamic) for decoding related hex var: {"
                  << s << "}" << std::endl;
        }
    }
    BOOST_FOREACH(const std::string& s, enc_vars)
    {
        if(!s.empty())
        {
            subscribe(s, &CpAcommsHandler::dccl_inbox, this);
            
            glogger() << group("dccl_dec") <<  "registering (dynamic) for encoding related hex var: {" << s << "}" << std::endl;
        }
    }

    glogger() << group("dccl_enc") << dccl_.brief_summary() << std::endl;
}


//
// Mail Handlers
//


void CpAcommsHandler::dccl_inbox(const CMOOSMsg& msg)
{            
    const std::string& key = msg.GetKey(); 	
    bool is_str = msg.IsString();
    const std::string& sval = msg.GetString();
    
    std::set<unsigned> ids;    
    unsigned id;
    if(dccl_.is_publish_trigger(ids, key, sval))
    {
        BOOST_FOREACH(unsigned id, ids)
        {
            goby::acomms::protobuf::ModemDataTransmission mm;
            pack(id, &mm);
        }
    }
    else if(dccl_.is_incoming(id, key) && is_str && msg.GetSource() != GetAppName())
    {
        goby::acomms::protobuf::ModemDataTransmission mm;
        mm.ParseFromString(sval);
        unpack(mm);
    }
}

void CpAcommsHandler::handle_ranging_request(const CMOOSMsg& msg)
{
    goby::acomms::protobuf::ModemRangingRequest request_msg;
    parse_for_moos(msg.GetString(), &request_msg);
    glogger() << "ranging request: " << request_msg << std::endl;
    if(driver_) driver_->handle_initiate_ranging(&request_msg);
}

void CpAcommsHandler::handle_message_push(const CMOOSMsg& msg)
{
    goby::acomms::protobuf::ModemDataTransmission new_message;
    parse_for_moos(msg.GetString(), &new_message);
    
    goby::acomms::protobuf::QueueKey& qk = out_moos_var2queue_[msg.GetKey()];
    
    if(new_message.data().empty())
        glogger() << warn << "message is either empty or contains invalid data" << std::endl;
    else if(!(qk.type() == goby::acomms::protobuf::QUEUE_DCCL))
        queue_manager_.push_message(out_moos_var2queue_[msg.GetKey()], new_message);
}


void CpAcommsHandler::handle_mac_cycle_update(const CMOOSMsg& msg)
{
    const std::string& s = msg.GetString();
    
    glogger() << "got update for MAC: " << s << std::endl;

    std::string dest;
    if(!goby::util::val_from_string(dest, s, "destination"))
        return;

    if(!goby::util::stricmp(dest, boost::lexical_cast<std::string>(cfg_.modem_id())))
        return;
    
    enum Type { replace, add, remove };
    Type type;

    std::string stype;
    if(!goby::util::val_from_string(stype, s, "update_type"))
        return;

    if(goby::util::stricmp(stype, "replace"))
    {
        mac_.clear_all_slots();
        type = replace;
        glogger() << "type is replace" << std::endl;
    }
    else if(goby::util::stricmp(stype, "add"))
    {
        type = add;
        glogger() << "type is add" << std::endl;
    }
    else if(goby::util::stricmp(stype, "remove"))
    {
        type = remove;
        glogger() << "type is remove" << std::endl;
    }
    
    else
        return;

    int i = 1;
    std::string si = boost::lexical_cast<std::string>(i);
    std::string poll_type, src_id, dest_id, rate, wait;
    while((goby::util::val_from_string(poll_type, s, std::string("slot_" + si + "_type")) &&  // preferred
           goby::util::val_from_string(src_id, s, std::string("slot_" + si + "_from")) &&
           goby::util::val_from_string(dest_id, s, std::string("slot_" + si + "_to")) &&  
           goby::util::val_from_string(rate, s, std::string("slot_" + si + "_rate")) &&
           goby::util::val_from_string(wait, s, std::string("slot_" + si + "_wait")))
          ||
          (goby::util::val_from_string(poll_type, s, std::string("poll_type" + si)) && // legacy
           goby::util::val_from_string(src_id, s, std::string("poll_from_id" + si)) &&
           goby::util::val_from_string(dest_id, s, std::string("poll_to_id" + si)) &&
           goby::util::val_from_string(rate, s, std::string("poll_rate" + si)) &&
           goby::util::val_from_string(wait, s, std::string("poll_wait" + si))))
    {
        
        bool fail = false;
        
        goby::acomms::protobuf::SlotType slot_type;
        if(goby::util::stricmp(poll_type, "data"))
            slot_type = goby::acomms::protobuf::SLOT_DATA;
        else if(goby::util::stricmp(poll_type, "ping"))
            slot_type = goby::acomms::protobuf::SLOT_PING;
        else if(goby::util::stricmp(poll_type, "remus_lbl"))
            slot_type = goby::acomms::protobuf::SLOT_REMUS_LBL;
        else
            fail = true;
        
        unsigned s, d, r, t;
        try
        {
            s = boost::lexical_cast<unsigned>(src_id);
            d = boost::lexical_cast<int>(dest_id);
            r = boost::lexical_cast<unsigned>(rate);
            t = boost::lexical_cast<unsigned>(wait);
        }
        catch(...)
        {
            fail = true;
        }

        if(!fail)
        {
            goby::acomms::protobuf::Slot slot;
            slot.set_src(s);
            slot.set_dest(d);
            slot.set_rate(r);
            slot.set_type(slot_type);
            slot.set_slot_seconds(t);
            if(type == replace || type == add)
                mac_.add_slot(slot);
            else
                mac_.remove_slot(slot);
        }
        
        ++i;
        si = boost::lexical_cast<std::string>(i);
    }
}

//
// Callbacks from goby libraries
//
void CpAcommsHandler::queue_qsize(goby::acomms::protobuf::QueueKey qk, unsigned size)
{
    typedef std::pair<std::string, goby::acomms::protobuf::QueueKey> P;
    BOOST_FOREACH(const P& p, out_moos_var2queue_)
    {
        if(qk == p.second)
        {
            publish(std::string("ACOMMS_QSIZE_" + p.first), size);
            return;
        }
    }    
}
void CpAcommsHandler::queue_expire(goby::acomms::protobuf::QueueKey qk, const goby::acomms::protobuf::ModemDataExpire& message)
{
    std::stringstream ss;
    ss << message;
    publish(MOOS_VAR_EXPIRE, ss.str());
}

void CpAcommsHandler::queue_incoming_data(goby::acomms::protobuf::QueueKey key, const goby::acomms::protobuf::ModemDataTransmission& message)
{
    std::string serialized;
    serialize_for_moos(&serialized, message);        
    CMOOSMsg m(MOOS_NOTIFY, MOOS_VAR_INCOMING_DATA, serialized, -1);
    m.m_sOriginatingCommunity = boost::lexical_cast<std::string>(message.base().src());    
    publish(m);

    // we know what this type is
    if(in_queue2moos_var_.count(key))
    {
        // post message and set originating community to modem id 
        CMOOSMsg m_specific(MOOS_NOTIFY, in_queue2moos_var_[key], serialized, -1);
        m_specific.m_sOriginatingCommunity = boost::lexical_cast<std::string>(message.base().src());    
        publish(m_specific);
    
        glogger() << group("q_in") << "published received data to "
                 << in_queue2moos_var_[key] << ": " << message << std::endl;
        
        if(key.type() == goby::acomms::protobuf::QUEUE_DCCL)
            unpack(message);
    }
}


void CpAcommsHandler::queue_on_demand(goby::acomms::protobuf::QueueKey key, const goby::acomms::protobuf::ModemDataRequest& request_msg, goby::acomms::protobuf::ModemDataTransmission* data_msg)
{
    pack(key.id(), data_msg);
}


void CpAcommsHandler::queue_ack(goby::acomms::protobuf::QueueKey key, const goby::acomms::protobuf::ModemDataAck& message)
{
    std::string serialized;
    serialize_for_moos(&serialized, message);
    
    publish(MOOS_VAR_ACK, serialized);
}


void CpAcommsHandler::modem_raw_in(const goby::acomms::protobuf::ModemMsgBase& base_msg)
{
    if(base_msg.raw().length() < MAX_MOOS_PACKET)
        publish(MOOS_VAR_NMEA_IN, base_msg.raw());
}

void CpAcommsHandler::modem_raw_out(const goby::acomms::protobuf::ModemMsgBase& base_msg)
{
    std::string out;
    serialize_for_moos(&out, base_msg);
        
    if(base_msg.raw().length() < MAX_MOOS_PACKET)
        publish(MOOS_VAR_NMEA_OUT, base_msg.raw());
    
}

void CpAcommsHandler::modem_range_reply(const goby::acomms::protobuf::ModemRangingReply& message)
{
    glogger() << "got range reply: " << message << std::endl;    
    std::string serialized;
    serialize_for_moos(&serialized, message);
    publish(MOOS_VAR_RANGING, serialized);
}


//
// READ CONFIGURATION
//

void CpAcommsHandler::process_configuration()
{
    // create driver object
    switch(cfg_.driver_type())
    {
        case pAcommsHandlerConfig::DRIVER_WHOI_MICROMODEM:
            driver_ = new goby::acomms::MMDriver(&glogger());
            break;

        case pAcommsHandlerConfig::DRIVER_NONE: break;
    }

    // add groups to flexostream human terminal output
    mac_.add_flex_groups(&glogger());
    dccl_.add_flex_groups(&glogger());
    queue_manager_.add_flex_groups(&glogger());
    if(driver_) driver_->add_flex_groups(&glogger());
    glogger().add_group("tcp", goby::util::Colors::green, "tcp share");
    
    if(cfg_.has_modem_id_lookup_path())
        glogger() << modem_lookup_.read_lookup_file(cfg_.modem_id_lookup_path()) << std::endl;
    else
        glogger() << warn << "no modem_id_lookup_path in moos file. this is required for conversions between modem_id and vehicle name / type." << std::endl;
    
    glogger() << "reading in geodesy information: " << std::endl;
    if (!cfg_.common().has_lat_origin() || !cfg_.common().has_lon_origin())
    {
        glogger() << die << "no lat_origin or lon_origin specified in configuration. this is required for geodesic conversion" << std::endl;
    }
    else
    {
        if(geodesy_.Initialise(cfg_.common().lat_origin(), cfg_.common().lon_origin()))
            glogger() << "success!" << std::endl;
        else
            glogger() << die << "could not initialize geodesy" << std::endl;
    }

    // check and propogate modem id
    if(cfg_.modem_id() == goby::acomms::BROADCAST_ID)
        glogger() << die << "modem_id = " << goby::acomms::BROADCAST_ID << " is reserved for broadcast messages. You must specify a modem_id != " << goby::acomms::BROADCAST_ID << " for this vehicle." << std::endl;
    
    publish("MODEM_ID", cfg_.modem_id());    
    
    cfg_.mutable_queue_cfg()->set_modem_id(cfg_.modem_id());
    cfg_.mutable_mac_cfg()->set_modem_id(cfg_.modem_id());
    cfg_.mutable_dccl_cfg()->set_modem_id(cfg_.modem_id());
    cfg_.mutable_driver_cfg()->set_modem_id(cfg_.modem_id());
    
    // merge message files: user can specify message files in either the queue cfg or the dccl cfg
    cfg_.mutable_dccl_cfg()->mutable_message_file()->MergeFrom(cfg_.queue_cfg().message_file());
    cfg_.mutable_queue_cfg()->mutable_message_file()->MergeFrom(cfg_.dccl_cfg().message_file());

    // check finalized configuration
    glogger() << cfg_ << std::endl;
    cfg_.CheckInitialized();

    // start goby-acomms classes
    if(driver_) driver_->startup(cfg_.driver_cfg());    
    mac_.startup(cfg_.mac_cfg());
    queue_manager_.set_cfg(cfg_.queue_cfg());
    dccl_.set_cfg(cfg_.dccl_cfg());    

    // initialize maps between incoming / outgoing MOOS variables and DCCL ids
    std::set<unsigned> ids = dccl_.all_message_ids();
    BOOST_FOREACH(unsigned id, ids)
    {
        goby::acomms::protobuf::QueueKey key;
        key.set_type(goby::acomms::protobuf::QUEUE_DCCL);
        key.set_id(id);
        out_moos_var2queue_[dccl_.get_outgoing_hex_var(id)] = key;
        in_queue2moos_var_[key] = dccl_.get_incoming_hex_var(id);
    }

    // tcp share
    if(cfg_.tcp_share_enable())
    {
        glogger() << group("tcp") << "tcp_share_port is: " << cfg_.tcp_share_port() << std::endl;
        
        tcp_share_server_ = new goby::util::TCPServer(cfg_.tcp_share_port());
        tcp_share_server_->start(); 
        glogger() << group("tcp") << "starting TCP server on: " << cfg_.tcp_share_port() << std::endl;        
    }

    BOOST_FOREACH(const std::string& full_ip, cfg_.tcp_share_to_ip())
    {
        if(!cfg_.tcp_share_enable())
        {            
            glogger() << group("tcp") << "tcp_share_in_address given but tcp_enable is false" << std::endl;
        }
        else
        {
            std::string ip;
            unsigned port = cfg_.tcp_share_port();

            std::string::size_type colon_pos = full_ip.find(":");
            if(colon_pos == std::string::npos)
                ip = full_ip;
            else
            {
                ip = full_ip.substr(0, colon_pos);
                port = boost::lexical_cast<unsigned>(full_ip.substr(colon_pos+1));
            }
            IP ip_and_port(ip, port);
            tcp_share_map_[ip_and_port] = new goby::util::TCPClient(ip, port);
            tcp_share_map_[ip_and_port]->start();
            glogger() << group("tcp") << "starting TCP client to "<<  ip << ":" << port << std::endl;
        }
    }
    
    // set up algorithms
    dccl_.add_algorithm("power_to_dB", boost::bind(&CpAcommsHandler::alg_power_to_dB, this, _1));
    dccl_.add_algorithm("dB_to_power", boost::bind(&CpAcommsHandler::alg_dB_to_power, this, _1));

    dccl_.add_adv_algorithm("TSD_to_soundspeed", boost::bind(&CpAcommsHandler::alg_TSD_to_soundspeed, this, _1, _2));
    dccl_.add_adv_algorithm("subtract", boost::bind(&CpAcommsHandler::alg_subtract, this, _1, _2));
    dccl_.add_adv_algorithm("add", boost::bind(&CpAcommsHandler::alg_add, this, _1, _2));
    
    dccl_.add_algorithm("to_lower", boost::bind(&CpAcommsHandler::alg_to_lower, this, _1));
    dccl_.add_algorithm("to_upper", boost::bind(&CpAcommsHandler::alg_to_upper, this, _1));
    dccl_.add_algorithm("angle_0_360", boost::bind(&CpAcommsHandler::alg_angle_0_360, this, _1));
    dccl_.add_algorithm("angle_-180_180", boost::bind(&CpAcommsHandler::alg_angle_n180_180, this, _1));
    
    dccl_.add_adv_algorithm("lat2utm_y", boost::bind(&CpAcommsHandler::alg_lat2utm_y, this, _1, _2));
    dccl_.add_adv_algorithm("lon2utm_x", boost::bind(&CpAcommsHandler::alg_lon2utm_x, this, _1, _2));
    dccl_.add_adv_algorithm("utm_x2lon", boost::bind(&CpAcommsHandler::alg_utm_x2lon, this, _1, _2));
    dccl_.add_adv_algorithm("utm_y2lat", boost::bind(&CpAcommsHandler::alg_utm_y2lat, this, _1, _2));
    dccl_.add_algorithm("modem_id2name", boost::bind(&CpAcommsHandler::alg_modem_id2name, this, _1));
    dccl_.add_algorithm("modem_id2type", boost::bind(&CpAcommsHandler::alg_modem_id2type, this, _1));
    dccl_.add_algorithm("name2modem_id", boost::bind(&CpAcommsHandler::alg_name2modem_id, this, _1));
}


void CpAcommsHandler::pack(unsigned dccl_id, goby::acomms::protobuf::ModemDataTransmission* modem_message)
{
    // encode the message and notify the MOOSDB
    std::map<std::string, std::vector<DCCLMessageVal> >& in = repeat_buffer_[dccl_id];

    // first entry
    if(!repeat_count_.count(dccl_id))
        repeat_count_[dccl_id] = 0;

    ++repeat_count_[dccl_id];        
    
    BOOST_FOREACH(const std::string& moos_var, dccl_.get_pubsub_src_vars(dccl_id))
    {
        const CMOOSMsg& moos_msg = dynamic_vars()[moos_var];
        
        bool is_dbl = valid(moos_msg) ? moos_msg.IsDouble() : false;
        bool is_str = valid(moos_msg) ? moos_msg.IsString() : false;
        double dval = valid(moos_msg) ? moos_msg.GetDouble() : NaN;
        std::string sval = valid(moos_msg) ? moos_msg.GetString() : "";
        
        if(is_str)
            in[moos_var].push_back(sval);
        else if(is_dbl)
            in[moos_var].push_back(dval);
        else
            in[moos_var].push_back(DCCLMessageVal());
    }

    // send this message out the door
    if(repeat_count_[dccl_id] == dccl_.get_repeat(dccl_id))
    {
        try
        {
            // encode
            dccl_.pubsub_encode(dccl_id, modem_message, in);

            std::string out_var = dccl_.get_outgoing_hex_var(dccl_id);
            
            std::string serialized;
            serialize_for_moos(&serialized, *modem_message);        
            publish(MOOS_VAR_OUTGOING_DATA, serialized);
            publish(out_var, serialized);
            
            goby::acomms::protobuf::QueueKey key;
            key.set_type(goby::acomms::protobuf::QUEUE_DCCL);
            key.set_id(dccl_id);
            queue_manager_.push_message(key, *modem_message);
            
            handle_tcp_share(modem_message);
        }
        catch(std::exception& e)
        {
            glogger() << group("dccl_enc") << warn << "could not encode message: " << *modem_message << ", reason: " << e.what() << std::endl;
        }

        // flush buffer
        repeat_buffer_[dccl_id].clear();
        // reset counter
        repeat_count_[dccl_id] = 0;
    }
    else
    {
        glogger() << group("dccl_enc") << "finished buffering part " << repeat_count_[dccl_id] << " of " <<  dccl_.get_repeat(dccl_id) << " for repeated message: " << dccl_.id2name(dccl_id) << ". Nothing has been sent yet." << std::endl;
    }
    
}

void CpAcommsHandler::unpack(goby::acomms::protobuf::ModemDataTransmission modem_message)
{
    handle_tcp_share(&modem_message);

    try
    {
        if(modem_message.base().dest() != cfg_.modem_id() && modem_message.base().dest() != goby::acomms::BROADCAST_ID)
        {
            glogger() << group("dccl_dec") << "ignoring message for modem_id " << modem_message.base().dest() << std::endl;
            return;
        }
        
        std::multimap<std::string, DCCLMessageVal> out;
        
        dccl_.pubsub_decode(modem_message, &out);
        
        typedef std::pair<std::string, DCCLMessageVal> P;
        BOOST_FOREACH(const P& p, out)
        {
            if(p.second.type() == goby::acomms::cpp_double)
            {
                double dval = p.second;
                CMOOSMsg m(MOOS_NOTIFY, p.first, dval, -1);
                m.m_sOriginatingCommunity = boost::lexical_cast<std::string>(modem_message.base().src());
                publish(m);
            }
            else
            {
                std::string sval = p.second;
                CMOOSMsg m(MOOS_NOTIFY, p.first, sval, -1);
                m.m_sOriginatingCommunity = boost::lexical_cast<std::string>(modem_message.base().src());
                publish(m);   
            }
        }
    }
    catch(std::exception& e)
    {
        glogger() << group("dccl_dec") << warn << "could not decode message: " << modem_message << ", reason: " << e.what() << std::endl;
    }    
}



void CpAcommsHandler::handle_tcp_share(goby::acomms::protobuf::ModemDataTransmission* modem_message)
{
    goby::acomms::DCCLHeaderDecoder decoder(modem_message->data());
    unsigned dccl_id = decoder[goby::acomms::HEAD_DCCL_ID];
    
    if(cfg_.tcp_share_enable() && dccl_.manip_manager().has(dccl_id, goby::acomms::protobuf::MessageFile::TCP_SHARE_IN))
    {
        typedef std::pair<IP, goby::util::TCPClient*> P;
        BOOST_FOREACH(const P&p, tcp_share_map_)
        {
            if(p.second->active())
            {
                std::stringstream ss;
                ss << p.second->local_ip() << ":" << cfg_.tcp_share_port();
                modem_message->AddExtension(pAcommsHandlerExtensions::seen_ip, ss.str());                
                
                std::string serialized;
                serialize_for_moos(&serialized, *modem_message);

                bool ip_seen = false;
                for(int i = 0, n = modem_message->ExtensionSize(pAcommsHandlerExtensions::seen_ip); i < n; ++i)
                {
                    if(modem_message->GetExtension(pAcommsHandlerExtensions::seen_ip, i) == p.first.ip_and_port())
                        ip_seen = true;
                }
                
                
                if(!ip_seen)
                {
                    glogger() << group("tcp") << "dccl_id: " << dccl_id << ": " << *modem_message << " to " << p.first.ip << ":" << p.first.port << std::endl;
                    p.second->write(serialized + "\r\n");
                }
                else
                {
                    glogger() << group("tcp") << p.first.ip << ":" << p.first.port << " has already seen this message, so not sending." << std::endl;
                }
            }
            else
                glogger() << group("tcp") << warn << p.first.ip << ":" << p.first.port << " is not connected." << std::endl;
        }
    }    
}


//
// DCCL ALGORITHMS
//


void CpAcommsHandler::alg_power_to_dB(DCCLMessageVal& val_to_mod)
{
    val_to_mod = 10*log10(double(val_to_mod));
}

void CpAcommsHandler::alg_dB_to_power(DCCLMessageVal& val_to_mod)
{
    val_to_mod = pow(10.0, double(val_to_mod)/10.0);
}

// applied to "T" (temperature), references are "S" (salinity), then "D" (depth)
void CpAcommsHandler::alg_TSD_to_soundspeed(DCCLMessageVal& val,
                           const std::vector<DCCLMessageVal>& ref_vals)
{
    val.set(goby::util::mackenzie_soundspeed(val, ref_vals[0], ref_vals[1]), 3);
}

// operator-=
void CpAcommsHandler::alg_subtract(DCCLMessageVal& val,
                                 const std::vector<DCCLMessageVal>& ref_vals)
{
    double d = val;
    BOOST_FOREACH(const::DCCLMessageVal& mv, ref_vals)
        d -= double(mv);
    
    val.set(d, val.precision());
}

// operator+=
void CpAcommsHandler::alg_add(DCCLMessageVal& val,
                            const std::vector<DCCLMessageVal>& ref_vals)
{
    double d = val;
    BOOST_FOREACH(const::DCCLMessageVal& mv, ref_vals)
        d += double(mv);
    val.set(d, val.precision());
}



void CpAcommsHandler::alg_to_upper(DCCLMessageVal& val_to_mod)
{
    val_to_mod = boost::algorithm::to_upper_copy(std::string(val_to_mod));
}

void CpAcommsHandler::alg_to_lower(DCCLMessageVal& val_to_mod)
{
    val_to_mod = boost::algorithm::to_lower_copy(std::string(val_to_mod));
}

void CpAcommsHandler::alg_angle_0_360(DCCLMessageVal& angle)
{
    double a = angle;
    while (a < 0)
        a += 360;
    while (a >=360)
        a -= 360;
    angle = a;
}

void CpAcommsHandler::alg_angle_n180_180(DCCLMessageVal& angle)
{
    double a = angle;
    while (a < -180)
        a += 360;
    while (a >=180)
        a -= 360;
    angle = a;
}

void CpAcommsHandler::alg_lat2utm_y(DCCLMessageVal& mv,
                                  const std::vector<DCCLMessageVal>& ref_vals)
{
    double lat = mv;
    double lon = ref_vals[0];
    double x = NaN;
    double y = NaN;
        
    if(!isnan(lat) && !isnan(lon)) geodesy_.LatLong2LocalUTM(lat, lon, y, x);        
    mv = y;
}

void CpAcommsHandler::alg_lon2utm_x(DCCLMessageVal& mv,
                                  const std::vector<DCCLMessageVal>& ref_vals)
{
    double lon = mv;
    double lat = ref_vals[0];
    double x = NaN;
    double y = NaN;

    if(!isnan(lat) && !isnan(lon)) geodesy_.LatLong2LocalUTM(lat, lon, y, x);
    mv = x;
}


void CpAcommsHandler::alg_utm_x2lon(DCCLMessageVal& mv,
                                  const std::vector<DCCLMessageVal>& ref_vals)
{
    double x = mv;
    double y = ref_vals[0];

    double lat = NaN;
    double lon = NaN;
    if(!isnan(y) && !isnan(x)) geodesy_.UTM2LatLong(x, y, lat, lon);    
    mv = lon;
}

void CpAcommsHandler::alg_utm_y2lat(DCCLMessageVal& mv,
                                  const std::vector<DCCLMessageVal>& ref_vals)
{
    double y = mv;
    double x = ref_vals[0];
    
    double lat = NaN;
    double lon = NaN;
    if(!isnan(x) && !isnan(y)) geodesy_.UTM2LatLong(x, y, lat, lon);    
    mv = lat;
}


 void CpAcommsHandler::alg_modem_id2name(DCCLMessageVal& in)
 {
    bool is_numeric = true;
    BOOST_FOREACH(const char c, std::string(in))
    {
        if(!isdigit(c))
        {
            is_numeric = false;
            break;
        }
    }
    if(is_numeric) in = modem_lookup_.get_name_from_id(boost::lexical_cast<unsigned>(std::string(in)));
 }
 
 void CpAcommsHandler::alg_modem_id2type(DCCLMessageVal& in)
 {
    bool is_numeric = true;
    BOOST_FOREACH(const char c, std::string(in))
    {
        if(!isdigit(c))
        {
            is_numeric = false;
            break;
        }    
    }
    if(is_numeric) in = modem_lookup_.get_type_from_id(boost::lexical_cast<unsigned>(std::string(in)));

 }

void CpAcommsHandler::alg_name2modem_id(DCCLMessageVal& in)
{
    std::stringstream ss;
    ss << modem_lookup_.get_id_from_name(in);
    
    in = ss.str();
}
