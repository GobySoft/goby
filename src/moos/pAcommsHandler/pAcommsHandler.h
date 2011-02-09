// t. schneider tes@mit.edu 06.05.08
// ocean engineering graudate student - mit / whoi joint program
// massachusetts institute of technology (mit)
// laboratory for autonomous marine sensing systems (lamss)
// 
// this is pAcommsHandler.h, part of pAcommsHandler
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

#ifndef pAcommsHandlerH
#define pAcommsHandlerH

#include <iostream>
#include <fstream>

#include <boost/date_time/posix_time/posix_time.hpp>

#include "MOOSLIB/MOOSApp.h"
#include "goby/moos/lib_tes_util/tes_moos_app.h"

#include "goby/acomms/dccl.h"
#include "goby/acomms/queue.h"
#include "goby/acomms/modem_driver.h"
#include "goby/acomms/amac.h"

#include "goby/moos/lib_tes_util/dynamic_moos_vars.h"

#include "MOOSLIB/MOOSLib.h"
#include "MOOSUtilityLib/MOOSGeodesy.h"

#include "goby/util/logger.h"
#include "goby/util/linebasedcomms.h"

#include "goby/moos/lib_tes_util/modem_id_convert.h"
#include "goby/moos/lib_tes_util/tes_moos_app.h"

#include <google/protobuf/io/tokenizer.h>

#include "pAcommsHandler_config.pb.h"


namespace goby {
    namespace acomms {
        namespace protobuf {
            class ModemDataTransmission;
        }
    } 
}

// data
const std::string MOOS_VAR_INCOMING_DATA = "ACOMMS_INCOMING_DATA";
const std::string MOOS_VAR_OUTGOING_DATA = "ACOMMS_OUTGOING_DATA";

const bool DEFAULT_NO_ENCODE = true;
const bool DEFAULT_ENCODE = false;

const bool DEFAULT_NO_DECODE = true;
const bool DEFAULT_DECODE = false;

const unsigned DEFAULT_TCP_SHARE_PORT = 11000;

// largest allowed moos packet
// something like 40000 (see PKT_TMP_BUFFER_SIZE in MOOSCommPkt.cpp)
// but that uses some space for serialization characters
const unsigned MAX_MOOS_PACKET = 35000; 

// serial feed
const std::string MOOS_VAR_NMEA_OUT = "ACOMMS_NMEA_OUT";
const std::string MOOS_VAR_NMEA_IN = "ACOMMS_NMEA_IN";

// ranging responses
const std::string MOOS_VAR_RANGING = "ACOMMS_RANGE_RESPONSE";
const std::string MOOS_VAR_COMMAND_RANGING = "ACOMMS_RANGE_COMMAND";

// acoustic acknowledgments get written here
const std::string MOOS_VAR_ACK = "ACOMMS_ACK";

// expired messages (ttl ends)
const std::string MOOS_VAR_EXPIRE = "ACOMMS_EXPIRE";

// communications quality statistics
// const std::string MOOS_VAR_STATS = "ACOMMS_STATS";

const std::string MOOS_VAR_CYCLE_UPDATE = "ACOMMS_MAC_CYCLE_UPDATE"; // preferred
const std::string MOOS_VAR_POLLER_UPDATE = "ACOMMS_POLLER_UPDATE"; // legacy


struct IP
{
IP(const std::string& ip = "", unsigned port = DEFAULT_TCP_SHARE_PORT)
: ip(ip),
        port(port)
        { }

    std::string ip_and_port() const
        {
            std::stringstream ss;
            ss << ip << ":" << port;
            return ss.str();
        }
    
    std::string ip;
    unsigned port;
};
    

inline void serialize_for_moos(std::string* out, const google::protobuf::Message& msg)
{
    google::protobuf::TextFormat::Printer printer;
    printer.SetSingleLineMode(true);
    printer.PrintToString(msg, out);
}


inline void parse_for_moos(const std::string& in, google::protobuf::Message* msg)
{
    google::protobuf::TextFormat::Parser parser;
    FlexOStreamErrorCollector error_collector(in);
    parser.RecordErrorsTo(&error_collector);
    parser.ParseFromString(in, msg);
}


class CpAcommsHandler : public TesMoosApp
{
  public:
    CpAcommsHandler();
    ~CpAcommsHandler();
    
  private:
    // from TesMoosApp
    void loop();    
    void do_subscriptions();
    void process_configuration();

    // read the internal driver part of the .moos file
    // void read_driver_parameters(CProcessConfigReader& config);
    // read the internal mac part of the .moos file
    // void read_internal_mac_parameters(CProcessConfigReader& config);
    // read the message queueing (XML / send, receive = ) part of the .moos file
    // void read_queue_parameters(CProcessConfigReader& config);
    
    // from QueueManager
    void queue_incoming_data(goby::acomms::protobuf::QueueKey key,
                             const goby::acomms::protobuf::ModemDataTransmission & message);
    void queue_ack(goby::acomms::protobuf::QueueKey key,
                   const goby::acomms::protobuf::ModemDataAck & message);
    void queue_on_demand(goby::acomms::protobuf::QueueKey key,
                         const goby::acomms::protobuf::ModemDataRequest& request_msg,
                         goby::acomms::protobuf::ModemDataTransmission* data_msg);
    
    void queue_qsize(goby::acomms::protobuf::QueueKey qk, unsigned size);
    void queue_expire(goby::acomms::protobuf::QueueKey key,
                      const goby::acomms::protobuf::ModemDataExpire & message);
    
    // from pAcommsPoller
    // provides the next highest priority destination for the poller ($CCCYC)
    void handle_poller_dest_request(const CMOOSMsg& msg);

    void handle_mac_cycle_update(const CMOOSMsg& msg);
    void handle_ranging_request(const CMOOSMsg& msg);
    void handle_message_push(const CMOOSMsg& msg);
    void handle_measure_noise_request(const CMOOSMsg& msg);
    
    // from MMDriver
    // publish raw NMEA stream from the modem ($CA)
    void modem_raw_in(const goby::acomms::protobuf::ModemMsgBase& base_msg);
    // publish raw NMEA stream to the modem ($CC)
    void modem_raw_out(const goby::acomms::protobuf::ModemMsgBase& base_msg);
    // write ping (ranging) responses
    void modem_range_reply(const goby::acomms::protobuf::ModemRangingReply& message);

    ///////
    /////// FROM MOOSDCCL
    ///////

    void dccl_inbox(const CMOOSMsg& msg);
    void dccl_iterate();
    // void read_dccl_parameters(CProcessConfigReader& processConfigReader);

    void pack(unsigned dccl_id, goby::acomms::protobuf::ModemDataTransmission* modem_message);
    void unpack(unsigned dccl_id, const goby::acomms::protobuf::ModemDataTransmission& modem_message, std::set<std::string>previous_hops = std::set<std::string>());

    void handle_tcp_share(unsigned dccl_id, const goby::acomms::protobuf::ModemDataTransmission& modem_message, std::set<std::string>previous_hops);

    
    bool encode(unsigned id)
    { return !(no_encode_.count(id) || all_no_encode_) || on_demand_.count(id); }
    
    bool decode(unsigned id)
    { return !(no_decode_.count(id) || all_no_decode_); }
    
    bool on_demand(unsigned id)
    { return on_demand_.count(id); }

    bool queue(unsigned id)
    { return !(no_queue_.count(id)); }

    bool loopback(unsigned id)
    { return loopback_.count(id); }
        
    bool tcp_share(unsigned id)
    { return tcp_share_.count(id); }
        
    void alg_power_to_dB(goby::acomms::DCCLMessageVal& val_to_mod);
    void alg_dB_to_power(goby::acomms::DCCLMessageVal& val_to_mod);

    void alg_to_upper(goby::acomms::DCCLMessageVal& val_to_mod);
    void alg_to_lower(goby::acomms::DCCLMessageVal& val_to_mod);
    void alg_angle_0_360(goby::acomms::DCCLMessageVal& angle);
    void alg_angle_n180_180(goby::acomms::DCCLMessageVal& angle);

    void alg_TSD_to_soundspeed(goby::acomms::DCCLMessageVal& val,
                               const std::vector<goby::acomms::DCCLMessageVal>& ref_vals);
    

    void alg_add(goby::acomms::DCCLMessageVal& val,
                 const std::vector<goby::acomms::DCCLMessageVal>& ref_vals);
    
    void alg_subtract(goby::acomms::DCCLMessageVal& val,
                      const std::vector<goby::acomms::DCCLMessageVal>& ref_vals);
    
    void alg_lat2utm_y(goby::acomms::DCCLMessageVal& val,
                       const std::vector<goby::acomms::DCCLMessageVal>& ref_vals);

    void alg_lon2utm_x(goby::acomms::DCCLMessageVal& val,
                       const std::vector<goby::acomms::DCCLMessageVal>& ref_vals);

    void alg_utm_x2lon(goby::acomms::DCCLMessageVal& val,
                       const std::vector<goby::acomms::DCCLMessageVal>& ref_vals);
    
    void alg_utm_y2lat(goby::acomms::DCCLMessageVal& val,
                       const std::vector<goby::acomms::DCCLMessageVal>& ref_vals);

    void alg_modem_id2name(goby::acomms::DCCLMessageVal& in);
    void alg_modem_id2type(goby::acomms::DCCLMessageVal& in);
    void alg_name2modem_id(goby::acomms::DCCLMessageVal& in);

    
  private:

    // ours ($CCCFG,SRC,modem_id_)
    int modem_id_;    

    static pAcommsHandlerConfig cfg_;
    
    //DCCL parsing
    goby::acomms::DCCLCodec dccl_;

    // manages queues and does additional packing
    goby::acomms::QueueManager queue_manager_;
    
    // internal driver class that interfaces to the modem
    goby::acomms::ModemDriverBase* driver_;

    // internal MAC
    goby::acomms::MACManager mac_;

    std::map<std::string, goby::acomms::protobuf::QueueKey> out_moos_var2queue_;
    std::map<goby::acomms::protobuf::QueueKey, std::string> in_queue2moos_var_;

    ///////
    /////// FROM MOOSDCCL
    ///////
        // do not encode for these ids
    std::set<unsigned> no_encode_;
    // do not decode for these ids
    std::set<unsigned> no_decode_;
    // encode these on demand 
    std::set<unsigned> on_demand_;
    // do not queue these ids
    std::set<unsigned> no_queue_;
    // loopback these ids
    std::set<unsigned> loopback_;
    // tcp share these ids
    std::set<unsigned> tcp_share_;
    
    bool all_no_encode_;
    bool all_no_decode_;    
    
    // initial values specified by user to be posted to the MOOSDB
    std::vector<CMOOSMsg> initializes_;

    CMOOSGeodesy geodesy_;
    
    tes::ModemIdConvert modem_lookup_;

    // buffer for <repeat> messages
    // maps message <id> onto pubsub encoding map
    std::map<unsigned, std::map<std::string, std::vector<goby::acomms::DCCLMessageVal> > > repeat_buffer_;
    std::map<unsigned, unsigned> repeat_count_;

//    bool tcp_share_enable_;
//    unsigned tcp_share_port_;
    // map ip -> port
    
    std::map<IP, goby::util::TCPClient*> tcp_share_map_;
    goby::util::TCPServer* tcp_share_server_;
};

inline bool operator<(const IP& a, const IP& b)
{ return a.ip < b.ip; }

#endif 
