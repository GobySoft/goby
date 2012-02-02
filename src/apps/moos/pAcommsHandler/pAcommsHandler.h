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

#include <google/protobuf/compiler/importer.h>
#include <google/protobuf/descriptor.h>

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/asio/deadline_timer.hpp>

#include "MOOSLIB/MOOSApp.h"

#include "goby/acomms.h"
#include "goby/util.h"

#ifdef ENABLE_GOBY_V1_TRANSITIONAL_SUPPORT
#include "goby/moos/transitional/dccl_transitional.h"
#endif

#include "MOOSLIB/MOOSLib.h"
#include "MOOSUtilityLib/MOOSGeodesy.h"

#include "goby/util/dynamic_protobuf_manager.h"
#include "goby/moos/goby_moos_app.h"
#include "goby/moos/moos_translator.h"

#include "goby/moos/protobuf/pAcommsHandler_config.pb.h"

extern std::vector<void *> dl_handles;

namespace goby {
    namespace acomms {
        namespace protobuf {
            class ModemDataTransmission;
        }
    } 
}

class CpAcommsHandler : public GobyMOOSApp
{
  public:
    static CpAcommsHandler* get_instance();
    static void delete_instance();
    
  private:
    CpAcommsHandler();
    ~CpAcommsHandler();
    void loop();     // from GobyMOOSApp

    goby::uint64 microsec_moos_time()
    { return static_cast<goby::uint64>(MOOSTime() * 1.0e6); }
    
        
    
    void process_configuration();

    void create_on_publish(const CMOOSMsg& trigger_msg, const goby::moos::protobuf::TranslatorEntry& entry);
    void create_on_multiplex_publish(const CMOOSMsg& moos_msg);
    void create_on_timer(const boost::system::error_code& error,
                         const goby::moos::protobuf::TranslatorEntry& entry,
                         boost::asio::deadline_timer* timer);
    
    void translate_and_push(const goby::moos::protobuf::TranslatorEntry& entry);    
    
    // from QueueManager
    void handle_queue_receive(const google::protobuf::Message& msg);

    void handle_goby_signal(const google::protobuf::Message& msg1,
                            const std::string& moos_var1,
                            const google::protobuf::Message& msg2,
                            const std::string& moos_var2);

    void handle_raw(const goby::acomms::protobuf::ModemRaw& msg, const std::string& moos_var);

    void handle_mac_cycle_update(const CMOOSMsg& msg);
    void handle_message_push(const CMOOSMsg& msg);
    
  private:
    google::protobuf::compiler::DiskSourceTree disk_source_tree_;
    google::protobuf::compiler::SourceTreeDescriptorDatabase source_database_;

    class TranslatorErrorCollector: public google::protobuf::compiler::MultiFileErrorCollector
    {
        void AddError(const std::string & filename, int line, int column, const std::string & message)
        {
            goby::glog.is(goby::common::logger::DIE) &&
                goby::glog << "File: " << filename
                           << " has error (line: " << line << ", column: " << column << "): "
                           << message << std::endl;
        }       
    };
                
    TranslatorErrorCollector error_collector_;

    goby::moos::MOOSTranslator translator_;
    
#ifdef ENABLE_GOBY_V1_TRANSITIONAL_SUPPORT
    //Old style XML DCCL1 parsing
    goby::transitional::DCCLTransitionalCodec transitional_dccl_;
#endif
    
    // new DCCL2 codec
    goby::acomms::DCCLCodec* dccl_;
    
    // manages queues and does additional packing
    goby::acomms::QueueManager queue_manager_;
    
    // driver class that interfaces to the modem
    goby::acomms::ModemDriverBase* driver_;

    // MAC
    goby::acomms::MACManager mac_;    
    
    boost::asio::io_service timer_io_service_;
    boost::asio::io_service::work work_;

    
    std::vector<boost::shared_ptr<boost::asio::deadline_timer> > timers_;
    
    static pAcommsHandlerConfig cfg_;
    static CpAcommsHandler* inst_;    
};

#endif 
