// t. schneider tes@mit.edu 9.16.09
// ocean engineering graduate student - mit / whoi joint program
// massachusetts institute of technology (mit)
// laboratory for autonomous marine sensing systems (lamss)
// 
// this is moos_dccl_codec.cpp
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
#include <boost/algorithm/string.hpp>
#include <boost/bind.hpp>

#include "moos_dccl_codec.h"
#include "goby/moos/lib_tes_util/tes_string.h"
#include "goby/util/sci.h"

using goby::acomms::NaN;
using namespace goby::util::tcolor;
using goby::acomms::DCCLMessageVal;
using goby::acomms::operator<<;

void MOOSDCCLCodec::inbox(const CMOOSMsg& msg)
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
    else if(dccl_.is_incoming(id, key) && is_str && msg.GetSource() != base_app_->appName())
    {
        goby::acomms::protobuf::ModemDataTransmission mm;
        mm.ParseFromString(sval);
        unpack(id, mm);
    }
}

void MOOSDCCLCodec::iterate()
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
    if(tcp_share_enable_ && tcp_share_server_)
    {
        std::string s;
        while(!(s = tcp_share_server_->readline()).empty())
        {
            boost::trim(s);
            
            goby::acomms::protobuf::ModemDataTransmission mm;
            mm.ParseFromString(s);
            
            unsigned dccl_id;
            goby::util::val_from_string(dccl_id, s, "dccl_id");
            std::string seen_str;
            goby::util::val_from_string(seen_str, s, "seen");
            std::set<std::string> seen_set;
            boost::split(seen_set, seen_str, boost::is_any_of(","));
            
            base_app_->logger() << group("tcp") << "incoming: dccl_id: "  << dccl_id << ": " << mm << std::endl;

            // post for others to use as needed in MOOS
            std::string in_var = dccl_.get_incoming_hex_var(dccl_id);

            std::string serialized;
            serialize_for_moos(&serialized, mm);        
            
            base_app_->outbox(in_var,serialized);
            
            unpack(dccl_id, mm, seen_set);
        }
    }
    
}

void MOOSDCCLCodec::read_parameters(CProcessConfigReader& processConfigReader)
{
    base_app_->logger().add_group("tcp", goby::util::Colors::green, "tcp share");

    
    std::string path;
    if (!processConfigReader.GetValue("modem_id_lookup_path", path))
        base_app_->logger() << warn << "no modem_id_lookup_path in moos file. this is required for modem_id conversion" << std::endl;
    else
        base_app_->logger() << modem_lookup_.read_lookup_file(path) << std::endl;

    std::string crypto_passphrase;
    if (processConfigReader.GetConfigurationParam("crypto_passphrase", crypto_passphrase))
    {
        dccl_.set_crypto_passphrase(crypto_passphrase);
        base_app_->logger() << "cryptography enabled with given passphrase" << std::endl;
    }    

    if (!processConfigReader.GetValue("modem_id", modem_id_))
    	base_app_->logger() << die << "modem_id not specified in .moos file." << std::endl;
    dccl_.set_modem_id(modem_id_);
    
    // look for latitude, longitude global variables
    double latOrigin, longOrigin;
    base_app_->logger() << "reading in geodesy information: " << std::endl;
    if (!processConfigReader.GetValue("LatOrigin", latOrigin) || !processConfigReader.GetValue("LongOrigin", longOrigin))
    {
        base_app_->logger() << die << "no LatOrigin and/or LongOrigin in moos file. this is required for geodesic conversion" << std::endl;
    }
    else
    {
        if(geodesy_.Initialise(latOrigin, longOrigin))
            base_app_->logger() << "success!" << std::endl;
        else
            base_app_->logger() << die << "could not initialize geodesy" << std::endl;
    }

    processConfigReader.GetConfigurationParam("tcp_share_enable", tcp_share_enable_);
    base_app_->logger() << group("tcp") << "tcp_share_enable is " << std::boolalpha << tcp_share_enable_ << std::endl;

    if(tcp_share_enable_)
    {
        if(!processConfigReader.GetConfigurationParam("tcp_share_port", tcp_share_port_))
            base_app_->logger() << group("tcp") << warn << "tcp_share_port not specified. using: " <<  tcp_share_port_ << std::endl;
        else
            base_app_->logger() << group("tcp") << "tcp_share_port is: " << tcp_share_port_ << std::endl;

        tcp_share_server_ = new goby::util::TCPServer(tcp_share_port_);
        tcp_share_server_->start();
        base_app_->logger() << group("tcp") << "starting TCP server on: " << tcp_share_port_ << std::endl;        
    }
    

   
    
    std::string schema = "";
    if (!processConfigReader.GetConfigurationParam("schema", schema))
    {
        base_app_->logger() << group("dccl_enc") <<  warn << "no schema specified. xml error checking disabled!" << std::endl;
    }    
    else
    {
        base_app_->logger() << group("dccl_enc") << "using schema located at: [" << schema << "]" << std::endl;
    }
    
    std::list<std::string> params;
    if(processConfigReader.GetConfiguration(processConfigReader.GetAppName(), params))
    {
        BOOST_FOREACH(const std::string& s, params)
        {
            std::vector<std::string> parts;
            boost::split(parts, s, boost::is_any_of("="));
            std::string value, key, manipulator;            

            switch(parts.size())
            {
                case 3:
                    key = parts[0];
                    manipulator = parts[1];
                    value = parts[2];                    
                    break;                    
                case 2:
                    key = parts[0];
                    value = parts[1];
                    break;
            }
            
            boost::trim(value);
            boost::trim(key);
            boost::trim(manipulator);
            
            if(tes::stricmp(key, "message_file"))
            {
                base_app_->logger() << group("dccl_enc") <<  "trying to parse file: " << value << std::endl;

                // parse the message file
                std::set<unsigned> ids = dccl_.add_xml_message_file(value, schema);
                if(queue_manager_) queue_manager_->add_xml_queue_file(value, schema);

                BOOST_FOREACH(unsigned id, ids)
                {
                    if(!manipulator.empty())
                    {
                        base_app_->logger() << group("dccl_enc") << "manipulators for id " << id << ": " << manipulator << std::endl;
                        if(manipulator.find("no_encode") != std::string::npos)
                            no_encode_.insert(id);
                        if(manipulator.find("no_decode") != std::string::npos)
                            no_decode_.insert(id);
                        if(manipulator.find("no_queue") != std::string::npos)
                            no_queue_.insert(id);
                        if(manipulator.find("loopback") != std::string::npos)
                            loopback_.insert(id);
                        if(manipulator.find("on_demand") != std::string::npos)
                        {
                            on_demand_.insert(id);
                            // this is queued automatically by libqueue, so
                            // don't duplicate
                            no_queue_.insert(id);
                            if(queue_manager_)
                            {
                                goby::acomms::protobuf::QueueKey key;
                                key.set_type(goby::acomms::protobuf::QUEUE_DCCL);
                                key.set_id(id);
                                queue_manager_->set_on_demand(key);
                            }
                            
                        }
                        if(manipulator.find("tcp_share_in") != std::string::npos)
                            tcp_share_.insert(id);
                    }
                }
                base_app_->logger() << group("dccl_enc") << "success!" << std::endl;
            }
            else if(tes::stricmp(key, "encode"))
                all_no_encode_ = !tes::stricmp(value, "true");
            else if(tes::stricmp(key, "decode"))
                all_no_decode_ = !tes::stricmp(value, "true");
            else if(tes::stricmp(key, "global_initializer") ||
                    tes::stricmp(key, "global_initializer.double") ||
                    tes::stricmp(key, "global_initializer.string")) 
            {
                // global_initializer = moos_var = other_value
                
                std::string& moos_var = manipulator;

                bool is_dbl = tes::stricmp(key, "global_initializer.double") ? true : false;

                std::string init_value;
                if (!processConfigReader.GetValue(value, init_value))
                    base_app_->logger() << group("dccl_enc") << warn << "cannot find global initialize variable: "  << value << std::endl;
                else
                {                    
                    base_app_->logger() << group("dccl_enc") << "processing global initialized variable: " << moos_var << ": " << init_value << std::endl;
                    
                    if(is_dbl)
                        initializes_.push_back(CMOOSMsg('N', moos_var, boost::lexical_cast<double>(init_value)));
                    else
                        initializes_.push_back(CMOOSMsg('N', moos_var, init_value));
                }
            }
            else if(tes::stricmp(key, "tcp_share_in_address"))
            {
                if(!tcp_share_enable_)
                    base_app_->logger() << group("tcp") << "tcp_share_in_address given but tcp_enable is false" << std::endl;
                else
                {
                    std::string ip;
                    unsigned port = tcp_share_port_;

                    std::string::size_type colon_pos = value.find(":");
                    if(colon_pos == std::string::npos)
                        ip = value;
                    else
                    {
                        ip = value.substr(0, colon_pos);
                        port = boost::lexical_cast<unsigned>(value.substr(colon_pos+1));
                    }
                    IP ip_and_port(ip, port);
                    tcp_share_map_[ip_and_port] = new goby::util::TCPClient(ip, port);
                    tcp_share_map_[ip_and_port]->start();
                    base_app_->logger() << group("tcp") << "starting TCP client to "<<  ip << ":" << port << std::endl;
                }   
            }
        }
    }

    // set up algorithms
    dccl_.add_algorithm("power_to_dB", boost::bind(&MOOSDCCLCodec::alg_power_to_dB, this, _1));
    dccl_.add_algorithm("dB_to_power", boost::bind(&MOOSDCCLCodec::alg_dB_to_power, this, _1));


    dccl_.add_adv_algorithm("TSD_to_soundspeed", boost::bind(&MOOSDCCLCodec::alg_TSD_to_soundspeed, this, _1, _2));
    dccl_.add_adv_algorithm("subtract", boost::bind(&MOOSDCCLCodec::alg_subtract, this, _1, _2));
    dccl_.add_adv_algorithm("add", boost::bind(&MOOSDCCLCodec::alg_add, this, _1, _2));
    
    dccl_.add_algorithm("to_lower", boost::bind(&MOOSDCCLCodec::alg_to_lower, this, _1));
    dccl_.add_algorithm("to_upper", boost::bind(&MOOSDCCLCodec::alg_to_upper, this, _1));
    dccl_.add_algorithm("angle_0_360", boost::bind(&MOOSDCCLCodec::alg_angle_0_360, this, _1));
    dccl_.add_algorithm("angle_-180_180", boost::bind(&MOOSDCCLCodec::alg_angle_n180_180, this, _1));
    
    dccl_.add_adv_algorithm("lat2utm_y", boost::bind(&MOOSDCCLCodec::alg_lat2utm_y, this, _1, _2));
    dccl_.add_adv_algorithm("lon2utm_x", boost::bind(&MOOSDCCLCodec::alg_lon2utm_x, this, _1, _2));
    dccl_.add_adv_algorithm("utm_x2lon", boost::bind(&MOOSDCCLCodec::alg_utm_x2lon, this, _1, _2));
    dccl_.add_adv_algorithm("utm_y2lat", boost::bind(&MOOSDCCLCodec::alg_utm_y2lat, this, _1, _2));
    dccl_.add_algorithm("modem_id2name", boost::bind(&MOOSDCCLCodec::alg_modem_id2name, this, _1));
    dccl_.add_algorithm("modem_id2type", boost::bind(&MOOSDCCLCodec::alg_modem_id2type, this, _1));
    dccl_.add_algorithm("name2modem_id", boost::bind(&MOOSDCCLCodec::alg_name2modem_id, this, _1));
}

void MOOSDCCLCodec::pack(unsigned dccl_id, goby::acomms::protobuf::ModemDataTransmission* modem_message)
{    
    if(!encode(dccl_id))
        return;    


    // encode the message and notify the MOOSDB
    std::map<std::string, std::vector<DCCLMessageVal> >& in = repeat_buffer_[dccl_id];

    // first entry
    if(!repeat_count_.count(dccl_id))
        repeat_count_[dccl_id] = 0;

    ++repeat_count_[dccl_id];        
    
    BOOST_FOREACH(const std::string& moos_var, dccl_.get_pubsub_src_vars(dccl_id))
    {
        const CMOOSMsg& moos_msg = base_app_->dynamic_vars()[moos_var];
        
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
        // encode
        dccl_.pubsub_encode(dccl_id, modem_message, in);
        // flush buffer
        repeat_buffer_[dccl_id].clear();
        // reset counter
        repeat_count_[dccl_id] = 0;
        
        
        std::string out_var = dccl_.get_outgoing_hex_var(dccl_id);

        std::string serialized;
        serialize_for_moos(&serialized, *modem_message);        
        base_app_->outbox(MOOS_VAR_OUTGOING_DATA, serialized);
        base_app_->outbox(out_var, serialized);
        
        if(queue(dccl_id))
        {
            goby::acomms::protobuf::QueueKey key;
            key.set_type(goby::acomms::protobuf::QUEUE_DCCL);
            key.set_id(dccl_id);
            signal_pack(key, *modem_message);
        }
        
        handle_tcp_share(dccl_id, *modem_message, std::set<std::string>());
        
        if(loopback(dccl_id)) unpack(dccl_id, *modem_message);
    }
    else
    {
        base_app_->logger() << group("dccl_enc") << "finished buffering part " << repeat_count_[dccl_id] << " of " <<  dccl_.get_repeat(dccl_id) << " for repeated message: " << dccl_.id2name(dccl_id) << ". Nothing has been sent yet." << std::endl;
    }    
}

void MOOSDCCLCodec::unpack(unsigned dccl_id, const goby::acomms::protobuf::ModemDataTransmission& modem_message, std::set<std::string>previous_hops_ips /* = std::set<std::string>() */)
{
    handle_tcp_share(dccl_id, modem_message, previous_hops_ips);

    if(!decode(dccl_id))
        return;

    try
    {
        if(modem_message.base().dest() != modem_id_ && modem_message.base().dest() != goby::acomms::BROADCAST_ID)
        {
            base_app_->logger() << group("dccl_dec") << "ignoring message for modem_id " << modem_message.base().dest() << std::endl;
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
                base_app_->outbox(m);
            }
            else
            {
                std::string sval = p.second;
                CMOOSMsg m(MOOS_NOTIFY, p.first, sval, -1);
                m.m_sOriginatingCommunity = boost::lexical_cast<std::string>(modem_message.base().src());
                base_app_->outbox(m);   
            }
        }
    }    
    catch(std::exception& e)
    {
        base_app_->logger() << group("dccl_dec") << warn << "could not decode message with id: " << dccl_id << ", reason: " << e.what() << std::endl;
    }    
}

void MOOSDCCLCodec::register_variables()
{
    std::set<std::string> enc_vars, dec_vars;

    BOOST_FOREACH(unsigned id, dccl_.all_message_ids())
    {
        if(encode(id))
        {
            std::set<std::string> vars = dccl_.get_pubsub_encode_vars(id);
            enc_vars.insert(vars.begin(), vars.end());
        }
        if(decode(id))
        {
            std::set<std::string> vars = dccl_.get_pubsub_decode_vars(id);
            dec_vars.insert(vars.begin(), vars.end());       
        }
    }

    BOOST_FOREACH(const std::string& s, dec_vars)
    {
        if(!s.empty())
        {
            base_app_->subscribe(s, &MOOSDCCLCodec::inbox, this);
            
            base_app_->logger() << group("dccl_dec")
                  <<  "registering (dynamic) for decoding related hex var: {"
                  << s << "}" << std::endl;
        }
    }
    
    
    BOOST_FOREACH(const std::string& s, enc_vars)
    {
        if(!s.empty())
        {
            base_app_->subscribe(s, &MOOSDCCLCodec::inbox, this);
            
            base_app_->logger() << group("dccl_dec") <<  "registering (dynamic) for encoding related hex var: {" << s << "}" << std::endl;
        }
    }        

    // publish initializes
    BOOST_FOREACH(CMOOSMsg& msg, initializes_)
        base_app_->outbox(msg);
    
    base_app_->logger() << group("dccl_enc") << dccl_.brief_summary() << std::endl;
}


void MOOSDCCLCodec::alg_power_to_dB(DCCLMessageVal& val_to_mod)
{
    val_to_mod = 10*log10(double(val_to_mod));
}

void MOOSDCCLCodec::alg_dB_to_power(DCCLMessageVal& val_to_mod)
{
    val_to_mod = pow(10.0, double(val_to_mod)/10.0);
}

// applied to "T" (temperature), references are "S" (salinity), then "D" (depth)
void MOOSDCCLCodec::alg_TSD_to_soundspeed(DCCLMessageVal& val,
                           const std::vector<DCCLMessageVal>& ref_vals)
{
    val.set(goby::util::mackenzie_soundspeed(val, ref_vals[0], ref_vals[1]), 3);
}

// operator-=
void MOOSDCCLCodec::alg_subtract(DCCLMessageVal& val,
                                 const std::vector<DCCLMessageVal>& ref_vals)
{
    double d = val;
    BOOST_FOREACH(const::DCCLMessageVal& mv, ref_vals)
        d -= double(mv);
    
    val.set(d, val.precision());
}

// operator+=
void MOOSDCCLCodec::alg_add(DCCLMessageVal& val,
                            const std::vector<DCCLMessageVal>& ref_vals)
{
    double d = val;
    BOOST_FOREACH(const::DCCLMessageVal& mv, ref_vals)
        d += double(mv);
    val.set(d, val.precision());
}



void MOOSDCCLCodec::alg_to_upper(DCCLMessageVal& val_to_mod)
{
    val_to_mod = boost::algorithm::to_upper_copy(std::string(val_to_mod));
}

void MOOSDCCLCodec::alg_to_lower(DCCLMessageVal& val_to_mod)
{
    val_to_mod = boost::algorithm::to_lower_copy(std::string(val_to_mod));
}

void MOOSDCCLCodec::alg_angle_0_360(DCCLMessageVal& angle)
{
    double a = angle;
    while (a < 0)
        a += 360;
    while (a >=360)
        a -= 360;
    angle = a;
}

void MOOSDCCLCodec::alg_angle_n180_180(DCCLMessageVal& angle)
{
    double a = angle;
    while (a < -180)
        a += 360;
    while (a >=180)
        a -= 360;
    angle = a;
}

void MOOSDCCLCodec::alg_lat2utm_y(DCCLMessageVal& mv,
                                  const std::vector<DCCLMessageVal>& ref_vals)
{
    double lat = mv;
    double lon = ref_vals[0];
    double x = NaN;
    double y = NaN;
        
    if(!isnan(lat) && !isnan(lon)) geodesy_.LatLong2LocalUTM(lat, lon, y, x);        
    mv = y;
}

void MOOSDCCLCodec::alg_lon2utm_x(DCCLMessageVal& mv,
                                  const std::vector<DCCLMessageVal>& ref_vals)
{
    double lon = mv;
    double lat = ref_vals[0];
    double x = NaN;
    double y = NaN;

    if(!isnan(lat) && !isnan(lon)) geodesy_.LatLong2LocalUTM(lat, lon, y, x);
    mv = x;
}


void MOOSDCCLCodec::alg_utm_x2lon(DCCLMessageVal& mv,
                                  const std::vector<DCCLMessageVal>& ref_vals)
{
    double x = mv;
    double y = ref_vals[0];

    double lat = NaN;
    double lon = NaN;
    if(!isnan(y) && !isnan(x)) geodesy_.UTM2LatLong(x, y, lat, lon);    
    mv = lon;
}

void MOOSDCCLCodec::alg_utm_y2lat(DCCLMessageVal& mv,
                                  const std::vector<DCCLMessageVal>& ref_vals)
{
    double y = mv;
    double x = ref_vals[0];
    
    double lat = NaN;
    double lon = NaN;
    if(!isnan(x) && !isnan(y)) geodesy_.UTM2LatLong(x, y, lat, lon);    
    mv = lat;
}


 void MOOSDCCLCodec::alg_modem_id2name(DCCLMessageVal& in)
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
 
 void MOOSDCCLCodec::alg_modem_id2type(DCCLMessageVal& in)
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

void MOOSDCCLCodec::alg_name2modem_id(DCCLMessageVal& in)
{
    std::stringstream ss;
    ss << modem_lookup_.get_id_from_name(in);
    
    in = ss.str();
}


void MOOSDCCLCodec::handle_tcp_share(unsigned dccl_id, const goby::acomms::protobuf::ModemDataTransmission& modem_message, std::set<std::string> previous_hops)
{
    if(tcp_share_enable_ && tcp_share(dccl_id))
    {
        typedef std::pair<IP, goby::util::TCPClient*> P;
        BOOST_FOREACH(const P&p, tcp_share_map_)
        {
            if(p.second->active())
            {
                std::stringstream ss;
                ss << "dccl_id=" << dccl_id << ",seen={" << p.second->local_ip() << ":" << tcp_share_port_ << ",";
                BOOST_FOREACH(const std::string& s, previous_hops)
                    ss << s << ",";
                ss << "}," << modem_message << "\r\n";

                if(!previous_hops.count(p.first.ip_and_port()))
                {
                    //base_app_->logger() << group("tcp") << "dccl_id: " << dccl_id << ": " <<  modem_message << " to " << p.first.ip << ":" << p.first.port << std::endl;
                    base_app_->logger() << group("tcp") << "dccl_id: " << dccl_id << ": " <<  ss.str() << " to " << p.first.ip << ":" << p.first.port << std::endl;
                    p.second->write(ss.str());
                }
                else
                {
                    base_app_->logger() << group("tcp") << p.first.ip << ":" << p.first.port << " has already seen this message, so not sending." << std::endl;
                }
            }
            else
                base_app_->logger() << group("tcp") << warn << p.first.ip << ":" << p.first.port << " is not connected." << std::endl;
        }
    }    
}
