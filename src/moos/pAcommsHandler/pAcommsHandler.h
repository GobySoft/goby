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
#include "goby/moos/lib_moos_dccl/moos_dccl_codec.h"

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

// command to measure noise
const std::string MOOS_VAR_MEASURE_NOISE_REQUEST = "ACOMMS_MEASURE_NOISE_COMMAND";

// acoustic acknowledgments get written here
const std::string MOOS_VAR_ACK = "ACOMMS_ACK";

// expired messages (ttl ends)
const std::string MOOS_VAR_EXPIRE = "ACOMMS_EXPIRE";

// communications quality statistics
// const std::string MOOS_VAR_STATS = "ACOMMS_STATS";

const std::string MOOS_VAR_CYCLE_UPDATE = "ACOMMS_MAC_CYCLE_UPDATE"; // preferred
const std::string MOOS_VAR_POLLER_UPDATE = "ACOMMS_POLLER_UPDATE"; // legacy



class CpAcommsHandler : public TesMoosApp
{
  public:
    CpAcommsHandler();
    ~CpAcommsHandler();
    
  private:
    // from TesMoosApp
    void loop();    
    void do_subscriptions();
    void read_configuration(CProcessConfigReader& config);


    // read the internal driver part of the .moos file
    void read_driver_parameters(CProcessConfigReader& config);
    // read the internal mac part of the .moos file
    void read_internal_mac_parameters(CProcessConfigReader& config);
    // read the message queueing (XML / send, receive = ) part of the .moos file
    void read_queue_parameters(CProcessConfigReader& config);
    
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
    void handle_nmea_out_request(const CMOOSMsg& msg);
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
    
  private:
    //DCCL parsing
    goby::acomms::DCCLCodec dccl_;

    // ours ($CCCFG,SRC,modem_id_)
    int modem_id_;    
    
    // do the encoding / decoding
    MOOSDCCLCodec moos_dccl_;

    // manages queues and does additional packing
    goby::acomms::QueueManager queue_manager_;
    
    // internal driver class that interfaces to the modem
    goby::acomms::MMDriver driver_;

    // internal MAC
    goby::acomms::MACManager mac_;

    std::map<std::string, goby::acomms::protobuf::QueueKey> out_moos_var2queue_;
    std::map<goby::acomms::protobuf::QueueKey, std::string> in_queue2moos_var_;
};

#endif 
