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

#include "goby/acomms.h"
#include "goby/util.h"
#include "goby/moos/transitional/dccl_transitional.h"

#include "MOOSLIB/MOOSLib.h"
#include "MOOSUtilityLib/MOOSGeodesy.h"

#include "goby/moos/dynamic_moos_vars.h"
#include "goby/moos/modem_id_convert.h"
#include "goby/moos/tes_moos_app.h"

#include "pAcommsHandler_config.pb.h"

namespace goby {
    namespace acomms {
        namespace protobuf {
            class ModemDataTransmission;
        }
    } 
}

// data
const std::string MOOS_VAR_RECEIVE = "ACOMMS_MODEM_RECEIVE";
const std::string MOOS_VAR_TRANSMIT = "ACOMMS_MODEM_TRANSMIT";

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

// acoustic acknowledgments get written here
const std::string MOOS_VAR_ACK = "ACOMMS_ACK";

// expired messages (ttl ends)
const std::string MOOS_VAR_EXPIRE = "ACOMMS_EXPIRE";

const std::string MOOS_VAR_QSIZE = "ACOMMS_QSIZE";

const std::string MOOS_VAR_CYCLE_UPDATE = "ACOMMS_MAC_CYCLE_UPDATE";

const std::string MOOS_VAR_FLUSH_QUEUE = "ACOMMS_FLUSH_QUEUE"; 



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
    


class CpAcommsHandler : public TesMoosApp
{
  public:
    static CpAcommsHandler* get_instance();
    
  private:
    CpAcommsHandler();
    ~CpAcommsHandler();
    void loop();     // from TesMoosApp
    void do_subscriptions();
    void process_configuration();

    // read the internal driver part of the .moos file
    // void read_driver_parameters(CProcessConfigReader& config);
    // read the internal mac part of the .moos file
    // void read_internal_mac_parameters(CProcessConfigReader& config);
    // read the message queueing (XML / send, receive = ) part of the .moos file
    // void read_queue_parameters(CProcessConfigReader& config);
    
    // from QueueManager
    void queue_receive_ccl(const goby::acomms::protobuf::ModemDataTransmission& message);
    void queue_receive(const google::protobuf::Message& msg);

    void queue_ack(const goby::acomms::protobuf::ModemTransmission& ack_msg,
                   const google::protobuf::Message& orig_msg);
    
//    void queue_on_demand(const goby::acomms::protobuf::ModemTransmission& request_msg, google::protobuf::Message* data_msg);
    
    void queue_expire(const google::protobuf::Message& orig_msg);
    void queue_qsize(const goby::acomms::protobuf::QueueSize& size);
    
    void handle_mac_cycle_update(const CMOOSMsg& msg);
    void handle_message_push(const CMOOSMsg& msg);
    void handle_flush_queue(const CMOOSMsg& msg);
    
    // from MMDriver
    // publish raw NMEA stream from the modem ($CA)
    void modem_raw_in(const goby::acomms::protobuf::ModemRaw& raw_msg);
    // publish raw NMEA stream to the modem ($CC)
    void modem_raw_out(const goby::acomms::protobuf::ModemRaw& raw_msg);

    // handle parsed receive from the modem 
    void modem_receive(const goby::acomms::protobuf::ModemTransmission& message);

    // handle parsed transmit from the modem 
    void modem_transmit_result(const goby::acomms::protobuf::ModemTransmission& message);
 
    void dccl_inbox(const CMOOSMsg& msg);
    void dccl_loop();

    void pack(unsigned dccl_id, boost::shared_ptr<google::protobuf::Message>& msg);
    void unpack(const google::protobuf::Message& msg);

//    void handle_tcp_share(goby::acomms::protobuf::ModemDataTransmission* modem_message);
    
    void alg_power_to_dB(goby::transitional::DCCLMessageVal& val_to_mod);
    void alg_dB_to_power(goby::transitional::DCCLMessageVal& val_to_mod);

    void alg_to_upper(goby::transitional::DCCLMessageVal& val_to_mod);
    void alg_to_lower(goby::transitional::DCCLMessageVal& val_to_mod);
    void alg_angle_0_360(goby::transitional::DCCLMessageVal& angle);
    void alg_angle_n180_180(goby::transitional::DCCLMessageVal& angle);

    void alg_TSD_to_soundspeed(goby::transitional::DCCLMessageVal& val,
                               const std::vector<goby::transitional::DCCLMessageVal>& ref_vals);
    

    void alg_add(goby::transitional::DCCLMessageVal& val,
                 const std::vector<goby::transitional::DCCLMessageVal>& ref_vals);
    
    void alg_subtract(goby::transitional::DCCLMessageVal& val,
                      const std::vector<goby::transitional::DCCLMessageVal>& ref_vals);
    
    void alg_lat2utm_y(goby::transitional::DCCLMessageVal& val,
                       const std::vector<goby::transitional::DCCLMessageVal>& ref_vals);

    void alg_lon2utm_x(goby::transitional::DCCLMessageVal& val,
                       const std::vector<goby::transitional::DCCLMessageVal>& ref_vals);

    void alg_utm_x2lon(goby::transitional::DCCLMessageVal& val,
                       const std::vector<goby::transitional::DCCLMessageVal>& ref_vals);
    
    void alg_utm_y2lat(goby::transitional::DCCLMessageVal& val,
                       const std::vector<goby::transitional::DCCLMessageVal>& ref_vals);

    void alg_modem_id2name(goby::transitional::DCCLMessageVal& in);
    void alg_modem_id2type(goby::transitional::DCCLMessageVal& in);
    void alg_name2modem_id(goby::transitional::DCCLMessageVal& in);

    
  private:

    // ours ($CCCFG,SRC,modem_id_)
    int modem_id_;    

    static pAcommsHandlerConfig cfg_;
    
    //Old style XML DCCL1 parsing
    goby::transitional::DCCLTransitionalCodec transitional_dccl_;

    // new DCCL2 codec
    goby::acomms::DCCLCodec* dccl_;
    
    // manages queues and does additional packing
    goby::acomms::QueueManager queue_manager_;
    
    // driver class that interfaces to the modem
    goby::acomms::ModemDriverBase* driver_;

    // MAC
    goby::acomms::MACManager mac_;

    std::map<std::string, unsigned> out_moos_var2queue_;
    std::map<unsigned, std::string> in_queue2moos_var_;
    
    CMOOSGeodesy geodesy_;
    
    tes::ModemIdConvert modem_lookup_;

    // buffer for <repeat> messages
    // maps message <id> onto pubsub encoding map
    std::map<unsigned, std::map<std::string, std::vector<goby::transitional::DCCLMessageVal> > > repeat_buffer_;
    std::map<unsigned, unsigned> repeat_count_;

    std::map<IP, goby::util::TCPClient*> tcp_share_map_;
    goby::util::TCPServer* tcp_share_server_;

    static CpAcommsHandler* inst_;    
};

inline bool operator<(const IP& a, const IP& b)
{ return a.ip < b.ip; }


#endif 
