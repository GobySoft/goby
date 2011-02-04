// t. schneider tes@mit.edu 9.16.09
// ocean engineering graduate student - mit / whoi joint program
// massachusetts institute of technology (mit)
// laboratory for autonomous marine sensing systems (lamss)
// 
// this is moos_dccl_codec.h
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

#ifndef MOOSDCCLCODECH
#define MOOSDCCLCODECH

#include "MOOSLIB/MOOSLib.h"
#include "MOOSUtilityLib/MOOSGeodesy.h"

#include "goby/util/logger.h"
#include "goby/acomms/dccl.h"
#include "goby/acomms/queue.h"
#include "goby/util/linebasedcomms.h"

#include "goby/moos/lib_tes_util/modem_id_convert.h"
#include "goby/moos/lib_tes_util/dynamic_moos_vars.h"
#include "goby/moos/lib_tes_util/tes_moos_app.h"

#include <google/protobuf/io/tokenizer.h>



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

class FlexOStreamErrorCollector : public google::protobuf::io::ErrorCollector
{
  public:
    void AddError(int line, int column, const std::string& message)
    {
        goby::util::glogger() << warn << message << std::endl;
    }
    void AddWarning(int line, int column, const std::string& message)
    {
        goby::util::glogger() << warn << message << std::endl;        
    }
};

inline void parse_for_moos(const std::string& in, google::protobuf::Message* msg)
{
    google::protobuf::TextFormat::Parser parser;
    FlexOStreamErrorCollector error_collector;
    parser.RecordErrorsTo(&error_collector);
    parser.ParseFromString(in, msg);
}


class MOOSDCCLCodec
{
  public:    
  MOOSDCCLCodec(TesMoosApp* base_app,
                goby::acomms::QueueManager* queue_manager = 0,
                bool all_no_encode = false,
                bool all_no_decode = false)
      : all_no_encode_(all_no_encode),
        all_no_decode_(all_no_decode),
        base_app_(base_app),
        dccl_(&base_app->logger()),
        queue_manager_(queue_manager),
        modem_id_(1),
        tcp_share_enable_(false),
        tcp_share_port_(DEFAULT_TCP_SHARE_PORT),
        tcp_share_server_(0)
        { }

    void inbox(const CMOOSMsg& msg);
    void iterate();
    void read_parameters(CProcessConfigReader& processConfigReader);
    void register_variables();

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
        
    void add_flex_groups()
    { dccl_.add_flex_groups(&base_app_->logger()); }
    
    goby::acomms::DCCLCodec& dccl_codec() { return dccl_; }
    
    
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

    boost::signal<void (goby::acomms::protobuf::QueueKey key,
                        const goby::acomms::protobuf::ModemDataTransmission& message)> signal_pack;
    
  
  private:
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

    TesMoosApp* base_app_;
    
    goby::acomms::DCCLCodec dccl_;
    goby::acomms::QueueManager* queue_manager_;

    CMOOSGeodesy geodesy_;
    
    tes::ModemIdConvert modem_lookup_;

    // buffer for <repeat> messages
    // maps message <id> onto pubsub encoding map
    std::map<unsigned, std::map<std::string, std::vector<goby::acomms::DCCLMessageVal> > > repeat_buffer_;
    std::map<unsigned, unsigned> repeat_count_;

    int modem_id_;

    bool tcp_share_enable_;
    unsigned tcp_share_port_;
    // map ip -> port
    
    std::map<IP, goby::util::TCPClient*> tcp_share_map_;
    goby::util::TCPServer* tcp_share_server_;
};

inline bool operator<(const IP& a, const IP& b)
{ return a.ip < b.ip; }

#endif
