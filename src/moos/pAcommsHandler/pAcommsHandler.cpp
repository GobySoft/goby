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
#include <boost/bind.hpp>
#include <boost/numeric/conversion/cast.hpp>
#include <boost/foreach.hpp>

#include "goby/acomms/libdccl/dccl_constants.h"
#include "goby/acomms/bind.h"

#include "pAcommsHandler.h"
#include "goby/moos/lib_tes_util/tes_string.h"
#include "goby/moos/lib_tes_util/binary.h"

using namespace goby::util::tcolor;
using goby::acomms::operator<<;

CpAcommsHandler::CpAcommsHandler()
    : modem_id_(1),
      moos_dccl_(this,
                 &queue_manager_,
                 DEFAULT_NO_ENCODE,
                 DEFAULT_NO_DECODE),
      queue_manager_(&logger()),
      driver_(&logger()),
      mac_(&logger())
{
    // bind the lower level pieces of goby-acomms together
    goby::acomms::bind(driver_, queue_manager_, mac_);

    // bind our methods to the rest of the goby-acomms callbacks
//    driver_.set_callback_in_raw(&CpAcommsHandler::modem_raw_in, this);
//    driver_.set_callback_out_raw(&CpAcommsHandler::modem_raw_out, this);
    driver_.set_callback_range_reply(&CpAcommsHandler::modem_range_reply, this);    

    queue_manager_.set_callback_receive(&CpAcommsHandler::queue_incoming_data, this);
    queue_manager_.set_callback_receive_ccl(&CpAcommsHandler::queue_incoming_data, this);
    queue_manager_.set_callback_ack(&CpAcommsHandler::queue_ack, this);
    queue_manager_.set_callback_data_on_demand(&CpAcommsHandler::queue_on_demand, this);
    queue_manager_.set_callback_queue_size_change(&CpAcommsHandler::queue_qsize, this);
    queue_manager_.set_callback_expire(&CpAcommsHandler::queue_expire, this);

    moos_dccl_.set_callback_pack(&goby::acomms::QueueManager::push_message, &queue_manager_);
}

CpAcommsHandler::~CpAcommsHandler()
{}

void CpAcommsHandler::loop()
{
    moos_dccl_.iterate();
    driver_.do_work();
    mac_.do_work();
    queue_manager_.do_work();    
}


void CpAcommsHandler::do_subscriptions()
// RegisterVariables: register for variables we want to get mail for
{
    // subscribe for publishes to the incoming hex
    typedef std::pair<std::string, goby::acomms::QueueKey> P;
    BOOST_FOREACH(const P& p, out_moos_var2queue_)
    {
        // *if* we're not doing the encoding, add the variable
        if(p.second.type() != goby::acomms::queue_dccl || !moos_dccl_.encode(p.second.id()))
            subscribe(p.first, &CpAcommsHandler::handle_message_push, this);
    }
        
    // raw feed for internal modem driver
    subscribe(MOOS_VAR_NMEA_OUT, &CpAcommsHandler::handle_nmea_out_request, this);

    // ping
    subscribe(MOOS_VAR_COMMAND_RANGING, &CpAcommsHandler::handle_ranging_request, this);
    
    subscribe(MOOS_VAR_CYCLE_UPDATE, &CpAcommsHandler::handle_mac_cycle_update, this);
    subscribe(MOOS_VAR_POLLER_UPDATE, &CpAcommsHandler::handle_mac_cycle_update, this);
        
    subscribe(MOOS_VAR_MEASURE_NOISE_REQUEST, &CpAcommsHandler::handle_measure_noise_request, this);
    
    moos_dccl_.register_variables();
}


//
// Mail Handlers
// 
void CpAcommsHandler::handle_nmea_out_request(const CMOOSMsg& msg)
{
    if(msg.GetSource()!=GetAppName())
    {
        goby::util::NMEASentence nmea(msg.GetString(), goby::util::NMEASentence::VALIDATE);
        driver_.write(nmea);
    }
}

void CpAcommsHandler::handle_ranging_request(const CMOOSMsg& msg)
{
    goby::acomms::protobuf::ModemRangingRequest request_msg;
    parse_for_moos(msg.GetString(), &request_msg);
    logger() << "ranging request: " << request_msg << std::endl;
    driver_.handle_initiate_ranging(request_msg);
}

void CpAcommsHandler::handle_message_push(const CMOOSMsg& msg)
{
    goby::acomms::protobuf::ModemDataTransmission new_message;
    parse_for_moos(msg.GetString(), &new_message);
    
    goby::acomms::QueueKey& qk = out_moos_var2queue_[msg.GetKey()];
    
    if(new_message.data().empty())
        logger() << warn << "message is either empty or contains invalid data" << std::endl;
    else if(!(qk.type() == goby::acomms::queue_dccl && !moos_dccl_.queue(qk.id())))
        queue_manager_.push_message(out_moos_var2queue_[msg.GetKey()], new_message);
}


void CpAcommsHandler::handle_mac_cycle_update(const CMOOSMsg& msg)
{
    const std::string& s = msg.GetString();
    
    logger() << "got update for MAC: " << s << std::endl;

    std::string dest;
    if(!tes::val_from_string(dest, s, "destination"))
        return;

    if(!tes::stricmp(dest, boost::lexical_cast<std::string>(modem_id_)))
        return;
    
    enum Type { replace, add, remove };
    Type type;

    std::string stype;
    if(!tes::val_from_string(stype, s, "update_type"))
        return;

    if(tes::stricmp(stype, "replace"))
    {
        mac_.clear_all_slots();
        type = replace;
        logger() << "type is replace" << std::endl;
    }
    else if(tes::stricmp(stype, "add"))
    {
        type = add;
        logger() << "type is add" << std::endl;
    }
    else if(tes::stricmp(stype, "remove"))
    {
        type = remove;
        logger() << "type is remove" << std::endl;
    }
    
    else
        return;

    int i = 1;
    std::string si = boost::lexical_cast<std::string>(i);
    std::string poll_type, src_id, dest_id, rate, wait;
    while((tes::val_from_string(poll_type, s, std::string("slot_" + si + "_type")) &&  // preferred
           tes::val_from_string(src_id, s, std::string("slot_" + si + "_from")) &&
           tes::val_from_string(dest_id, s, std::string("slot_" + si + "_to")) &&  
           tes::val_from_string(rate, s, std::string("slot_" + si + "_rate")) &&
           tes::val_from_string(wait, s, std::string("slot_" + si + "_wait")))
          ||
          (tes::val_from_string(poll_type, s, std::string("poll_type" + si)) && // legacy
           tes::val_from_string(src_id, s, std::string("poll_from_id" + si)) &&
           tes::val_from_string(dest_id, s, std::string("poll_to_id" + si)) &&
           tes::val_from_string(rate, s, std::string("poll_rate" + si)) &&
           tes::val_from_string(wait, s, std::string("poll_wait" + si))))
    {
        
        
        goby::acomms::Slot::SlotType slot_type = goby::acomms::Slot::slot_notype;
        if(tes::stricmp(poll_type, "data"))
            slot_type = goby::acomms::Slot::slot_data;
        else if(tes::stricmp(poll_type, "ping"))
            slot_type = goby::acomms::Slot::slot_ping;
        else if(tes::stricmp(poll_type, "remus_lbl"))
            slot_type = goby::acomms::Slot::slot_remus_lbl;
        
        
        bool fail = false;
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
            goby::acomms::Slot slot(s, d, r, slot_type, t);
            if(type == replace || type == add)
                mac_.add_slot(slot);
            else
                mac_.remove_slot(slot);
        }
        
        ++i;
        si = boost::lexical_cast<std::string>(i);
    }
}


void CpAcommsHandler::handle_measure_noise_request(const CMOOSMsg& msg)
{
    if(msg.IsDouble())
        driver_.measure_noise(msg.GetDouble());
    else
        driver_.measure_noise(boost::lexical_cast<unsigned>(msg.GetString()));
    
}

//
// Callbacks from goby libraries
//
void CpAcommsHandler::queue_qsize(goby::acomms::QueueKey qk, unsigned size)
{
    typedef std::pair<std::string, goby::acomms::QueueKey> P;
    BOOST_FOREACH(const P& p, out_moos_var2queue_)
    {
        if(qk == p.second)
        {
            outbox(std::string("ACOMMS_QSIZE_" + p.first), size);
            return;
        }
    }    
}
void CpAcommsHandler::queue_expire(goby::acomms::QueueKey qk, const goby::acomms::protobuf::ModemDataExpire& message)
{
    std::stringstream ss;
    ss << message;
    outbox(MOOS_VAR_EXPIRE, ss.str());
}

void CpAcommsHandler::queue_incoming_data(goby::acomms::QueueKey key, const goby::acomms::protobuf::ModemDataTransmission& message)
{
    std::string serialized;
    serialize_for_moos(&serialized, message);        
    CMOOSMsg m(MOOS_NOTIFY, MOOS_VAR_INCOMING_DATA, serialized, -1);
    m.m_sOriginatingCommunity = boost::lexical_cast<std::string>(message.base().src());    
    outbox(m);

    // we know what this type is
    if(in_queue2moos_var_.count(key))
    {
        // post message and set originating community to modem id 
        CMOOSMsg m_specific(MOOS_NOTIFY, in_queue2moos_var_[key], serialized, -1);
        m_specific.m_sOriginatingCommunity = boost::lexical_cast<std::string>(message.base().src());    
        outbox(m_specific);
    
        logger() << group("q_in") << "published received data to "
                 << in_queue2moos_var_[key] << ": " << message << std::endl;
        
        if(moos_dccl_.decode(key.id()) && key.type() == goby::acomms::queue_dccl)
            moos_dccl_.unpack(key.id(), message);
    }
}


bool CpAcommsHandler::queue_on_demand(goby::acomms::QueueKey key, const goby::acomms::protobuf::ModemDataRequest& request_msg, goby::acomms::protobuf::ModemDataTransmission* data_msg)
{
    moos_dccl_.pack(key.id(), data_msg);
    return true;
}


void CpAcommsHandler::queue_ack(goby::acomms::QueueKey key, const goby::acomms::protobuf::ModemDataAck& message)
{
    std::string serialized;
    serialize_for_moos(&serialized, message);
    
    outbox(MOOS_VAR_ACK, serialized);
}


void CpAcommsHandler::modem_raw_in(std::string s)
{
    boost::trim(s);
    if(s.length() < MAX_MOOS_PACKET) outbox(MOOS_VAR_NMEA_IN, s);
}

void CpAcommsHandler::modem_raw_out(std::string s)
{
    boost::trim(s);
    if(s.length() < MAX_MOOS_PACKET) outbox(MOOS_VAR_NMEA_OUT, s);
}

void CpAcommsHandler::modem_range_reply(const goby::acomms::protobuf::ModemRangingReply& message)
{
    logger() << "got range reply: " << message << std::endl;    
    std::string serialized;
    serialize_for_moos(&serialized, message);
    outbox(MOOS_VAR_RANGING, serialized);
}


//
// READ CONFIGURATION
//

void CpAcommsHandler::read_configuration(CProcessConfigReader& config)
{
    mac_.add_flex_groups(logger());
    moos_dccl_.add_flex_groups();
    queue_manager_.add_flex_groups(logger());
    driver_.add_flex_groups(logger());
    
    moos_dccl_.read_parameters(config);

    if (!config.GetValue("modem_id", modem_id_))
        logger() << die  << "modem_id not specified in .moos file." << std::endl;
    
    if(modem_id_ == goby::acomms::BROADCAST_ID)
        logger() << die << "modem_id = " << goby::acomms::BROADCAST_ID << " is reserved for broadcast messages. You must specify a modem_id != " << goby::acomms::BROADCAST_ID << " for this vehicle." << std::endl;
    
    queue_manager_.set_modem_id(modem_id_);
    mac_.set_modem_id(modem_id_);
        
    // inform the MOOSDB of the modem id (or vehicle id as it's sometimes referred to)
    outbox("VEHICLE_ID", modem_id_);
    outbox("MODEM_ID", modem_id_);    
    
    read_driver_parameters(config);
    
    // read configuration for the (MOOS) message queues
    read_queue_parameters(config);    
}


void CpAcommsHandler::read_driver_parameters(CProcessConfigReader& config)
{
    std::cout << "Reading driver parameters..." << std::endl;

    // Read whether using a hydroid gateway buoy
    bool is_hydroid_gateway = false;
    if (!config.GetConfigurationParam("hydroid_gateway_enabled", is_hydroid_gateway))
        logger() << group("mm_out") << "Hydroid Buoy flag not set, using default of " << std::boolalpha << is_hydroid_gateway << std::endl;

    // Read the hydroid gateway buoy index
    int hydroid_gateway_id = 1;
    if(is_hydroid_gateway)
    {
        if (!config.GetConfigurationParam("hydroid_gateway_id", hydroid_gateway_id))
            logger() << group("mm_out") << warn << "Hydroid Gateway ID not set, using default of " << hydroid_gateway_id << std::endl;
        
        driver_.set_hydroid_gateway_prefix(hydroid_gateway_id);
    }

    std::string connection_type;
    config.GetConfigurationParam("connection_type", connection_type);

    goby::acomms::ModemDriverBase::ConnectionType type = goby::acomms::ModemDriverBase::CONNECTION_SERIAL;

    if(tes::stricmp(connection_type, "tcp_as_client"))
        type = goby::acomms::ModemDriverBase::CONNECTION_TCP_AS_CLIENT;
    else if(tes::stricmp(connection_type, "tcp_as_server"))
        type = goby::acomms::ModemDriverBase::CONNECTION_TCP_AS_SERVER;
    else if(tes::stricmp(connection_type, "dual_udp_broadcast"))
        type = goby::acomms::ModemDriverBase::CONNECTION_DUAL_UDP_BROADCAST;

    driver_.set_connection_type(type);
    if(type == goby::acomms::ModemDriverBase::CONNECTION_SERIAL)
    {
        // read information about serial port (name & baud) and set them
        std::string serial_port_name;
        if (!config.GetConfigurationParam("serial_port", serial_port_name))
            logger() << die << "no serial_port set." << std::endl;    
        driver_.set_serial_port(serial_port_name);
        
        unsigned baud;
        if (!config.GetConfigurationParam("baud", baud))
            logger() << group("mm_out") << warn << "no baud rate set, using default of " << driver_.serial_baud() << " ($CCCFG,BR1,3 or $CCCFG,BR2,3)" << std::endl;
        else
            driver_.set_serial_baud(baud);
    }
    else if(type == goby::acomms::ModemDriverBase::CONNECTION_TCP_AS_CLIENT)
    {
        // read information about serial port (name & baud) and set them
        std::string network_address;
        if (!config.GetConfigurationParam("network_address", network_address))
            logger() << die << "no network address set." << std::endl; 
        driver_.set_tcp_server(network_address);
        
        unsigned network_port;
        if (!config.GetConfigurationParam("network_port", network_port))
            logger() << die << "no network port set." << std::endl; 
        driver_.set_tcp_port(network_port);
    }
    else if(type == goby::acomms::ModemDriverBase::CONNECTION_TCP_AS_SERVER)
    {
        unsigned network_port;
        if (!config.GetConfigurationParam("network_port", network_port))
            logger() << die << "no network port set." << std::endl; 
        driver_.set_tcp_port(network_port);        
    }    
    
    std::vector<std::string> cfg;

    bool cfgall = false;
    if (!config.GetConfigurationParam("cfg_to_defaults", cfgall))
        logger()  << group("mm_out") << "not setting all CFG values to factory default "
               << "before setting our CFG values. consider using cfg_to_defaults=true if you can." << std::endl;
    
    if(cfgall && !is_hydroid_gateway) // Hydroid gateway breaks if you run CCCFG,ALL! They use a 4800 baud while the WHOI default is 19200. You will need to open the buoy if this happens
        cfg.push_back("ALL,0");

    cfg.push_back(std::string("SRC," + boost::lexical_cast<std::string>(modem_id_)));
    
    std::list<std::string> params;
    if(config.GetConfiguration(GetAppName(), params))
    {
        BOOST_FOREACH(std::string& value, params)
        {
            boost::trim(value);
            std::string key = MOOSChomp(value, "=");

            if(tes::stricmp(key, "cfg"))
            {
                logger() << group("mm_out") << "adding CFG value: " << value << std::endl;
                cfg.push_back(value);
            }
        }
    }

    driver_.set_cfg(cfg);
    
    std::string mac;
    if (!config.GetConfigurationParam("mac", mac))
        logger() << group("mac") << "`mac` not specified, using no medium access control. you must provide an external MAC, or set `mac = slotted`, `mac=fixed_slotted` or `mac = polled` in the .moos file" << std::endl;

    if(mac == "polled") mac_.set_type(goby::acomms::mac_polled);
    else if(mac == "slotted") mac_.set_type(goby::acomms::mac_auto_decentralized);
    else if(mac == "fixed_slotted") mac_.set_type(goby::acomms::mac_fixed_decentralized);
    
    read_internal_mac_parameters(config);
    
        
    driver_.startup();
    mac_.startup();
}

void CpAcommsHandler::read_internal_mac_parameters(CProcessConfigReader& config)
{
    if(mac_.type() == goby::acomms::mac_auto_decentralized)
    {   
        unsigned slot_time = 15;
        if (!config.GetConfigurationParam("slot_time", slot_time))
            logger() << group("mac") << warn << "slot_time not specified in .moos file, using default of " << slot_time << std::endl;
        
        mac_.set_slot_time(slot_time);
        
        std::string rate = "0";
        if (!config.GetConfigurationParam("rate", rate))
            logger() << group("mac") << warn << "message rate not specified in .moos file, using default of " << rate << std::endl;
        
        if(tes::stricmp(rate, "auto"))
            rate = "-1";
        
        mac_.set_rate(rate);

        unsigned expire_cycles = 15;
        if (!config.GetConfigurationParam("expire_cycles", expire_cycles))
            logger() << group("mac") << warn << "expire_cycles not specified in .moos file, using default of " << expire_cycles << std::endl;
        
        mac_.set_expire_cycles(expire_cycles);

    }
}

void CpAcommsHandler::read_queue_parameters(CProcessConfigReader& config)
{    
    std::list<std::string> params;
    if(!config.GetConfiguration(GetAppName(), params))
        return;
    
    BOOST_FOREACH(std::string& value, params)
    {
        boost::trim(value);
        std::string key = MOOSChomp(value, "=");


        if(tes::stricmp(key, "send") || tes::stricmp(key, "receive"))
            logger() << die << "`send =` and `receive =` configuration options are no longer available. all data must be either a DCCL or CCL message. use message XML files to encode DCCL and configure queuing with the <queue/> block. add XML files with `message_file = `" << std::endl;
        
        if(tes::stricmp(key, "send_CCL"))
        {
            // break up the parameter into a vector of individual strings
            std::vector<std::string> send_params;
            boost::split(send_params, value, boost::is_any_of(","));
                
            if(send_params.size() < 2)
                logger() << warn << "invalid send configuration line: " << value << std::endl;
            else
            {
                std::string moos_var_name = boost::trim_copy(send_params[0]);

                unsigned id;
                goby::util::hex_string2number(send_params[1], id);
                out_moos_var2queue_[moos_var_name] = goby::acomms::QueueKey(goby::acomms::queue_ccl, id);
                    
                goby::acomms::QueueConfig q_cfg;                    
                switch (send_params.size())
                {
                    case 8: q_cfg.set_ttl(send_params[7]);
                    case 7: q_cfg.set_value_base(send_params[6]);
                    case 6: q_cfg.set_newest_first(send_params[5]);
                    case 5: q_cfg.set_max_queue(send_params[4]);
                    case 4: q_cfg.set_blackout_time(send_params[3]);
                    case 3: q_cfg.set_ack(send_params[2]);
                    case 2: q_cfg.set_id(id);
                        q_cfg.set_type(goby::acomms::queue_ccl);
                        q_cfg.set_name(boost::trim_copy(send_params[0]));
                    default: break;
                }
                queue_manager_.add_queue(q_cfg);
            }
        }
        else if(tes::stricmp(key, "receive_CCL"))
        {
            std::vector<std::string> receive_params;
            boost::split(receive_params, value, boost::is_any_of(","));

            if(receive_params.size() < 2)
                logger() << warn << "invalid receive configuration line: " + value << std::endl;
            else
            {
                std::string moos_var_name = boost::trim_copy(receive_params[0]);
                // could be internal ID or CCL id
                std::string variable_id = boost::trim_copy(receive_params[1]);

                unsigned id;
                goby::util::hex_string2number(variable_id, id);
                in_queue2moos_var_[goby::acomms::QueueKey(goby::acomms::queue_ccl, id)] = moos_var_name;

            }
        }
    }
    
    std::set<unsigned> ids = moos_dccl_.dccl_codec().all_message_ids();
    BOOST_FOREACH(unsigned id, ids)
    {
        goby::acomms::QueueKey key(goby::acomms::queue_dccl, id);
        out_moos_var2queue_[moos_dccl_.dccl_codec().get_outgoing_hex_var(id)] = key;
        in_queue2moos_var_[key] = moos_dccl_.dccl_codec().get_incoming_hex_var(id);
    }    
}
