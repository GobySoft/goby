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

#include <dlfcn.h>
#include <cctype>

#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/numeric/conversion/cast.hpp>
#include <boost/foreach.hpp>
#include <boost/math/special_functions/fpclassify.hpp>

#include "pAcommsHandler.h"
#include "goby/util/sci.h"

using namespace goby::util::tcolor;
using namespace goby::util::logger;
using goby::acomms::operator<<;
using goby::moos::operator<<;
using goby::util::as;
using google::protobuf::uint32;
using goby::transitional::DCCLMessageVal;
using goby::glog;

const double NaN = std::numeric_limits<double>::quiet_NaN();

pAcommsHandlerConfig CpAcommsHandler::cfg_;
CpAcommsHandler* CpAcommsHandler::inst_ = 0;

CpAcommsHandler* CpAcommsHandler::get_instance()
{
    if(!inst_)
        inst_ = new CpAcommsHandler();
    return inst_;
}

void CpAcommsHandler::delete_instance()
{
    delete inst_;
}

CpAcommsHandler::CpAcommsHandler()
    : TesMoosApp(&cfg_),
      source_database_(&disk_source_tree_),
      translator_(goby::moos::protobuf::TranslatorEntry(),
                  cfg_.common().lat_origin(),
                  cfg_.common().lon_origin(),
                  cfg_.modem_id_lookup_path()),
      dccl_(goby::acomms::DCCLCodec::get()),
      work_(timer_io_service_)
{
    source_database_.RecordErrorsTo(&error_collector_);
    disk_source_tree_.MapPath("/", "/");
    goby::util::DynamicProtobufManager::add_database(&source_database_);

    transitional_dccl_.convert_to_v2_representation(&cfg_);
    translator_.add_entry(cfg_.translator_entry());
    
    
    goby::acomms::connect(&queue_manager_.signal_receive,
                          this, &CpAcommsHandler::queue_receive);
    // goby::acomms::connect(&queue_manager_.signal_ack,
    //                       this, &CpAcommsHandler::queue_ack);
    // goby::acomms::connect(&queue_manager_.signal_data_on_demand,
    //                       this, &CpAcommsHandler::queue_on_demand);
    // goby::acomms::connect(&queue_manager_.signal_queue_size_change,
    //                       this, &CpAcommsHandler::queue_qsize);
    // goby::acomms::connect(&queue_manager_.signal_expire,
    //                       this, &CpAcommsHandler::queue_expire);

    process_configuration();

    // bind the lower level pieces of goby-acomms together
    if(driver_)
    {
        goby::acomms::bind(*driver_, queue_manager_);
        goby::acomms::bind(mac_, *driver_);

// bind our methods to the rest of the goby-acomms signals
        // goby::acomms::connect(&driver_->signal_raw_outgoing,
        //                       this, &CpAcommsHandler::modem_raw_out);
        // goby::acomms::connect(&driver_->signal_raw_incoming,
        //                       this, &CpAcommsHandler::modem_raw_in);    
        // goby::acomms::connect(&driver_->signal_receive,
        //                       this, &CpAcommsHandler::modem_receive);
        // goby::acomms::connect(&driver_->signal_transmit_result,
        //                       this, &CpAcommsHandler::modem_transmit_result);
    }
    
    
    // update comms cycle
    subscribe(MOOS_VAR_CYCLE_UPDATE, &CpAcommsHandler::handle_mac_cycle_update, this);    
}

CpAcommsHandler::~CpAcommsHandler()
{}

void CpAcommsHandler::loop()
{
    timer_io_service_.poll();

    if(driver_) driver_->do_work();
    mac_.do_work();
    queue_manager_.do_work();    
}



//
// Mail Handlers
//

void CpAcommsHandler::handle_mac_cycle_update(const CMOOSMsg& msg)
{
    goby::acomms::protobuf::MACUpdate update_msg;
    parse_for_moos(msg.GetString(), &update_msg);
    
    glog << "got update for MAC: " << update_msg << std::endl;

    if(!update_msg.dest() == cfg_.modem_id())
    {
        glog << "update not for us" << std::endl;
        return;
    }

    goby::acomms::MACManager::iterator it1 = mac_.begin(), it2 = mac_.begin();
    
    for(int i = 0, n = update_msg.first_iterator(); i < n; ++i)
        ++it1;
    
    for(int i = 0, n = update_msg.second_iterator(); i < n; ++i)
        ++it2;
    
    
    switch(update_msg.update_type())
    {
        case goby::acomms::protobuf::MACUpdate::ASSIGN:
            mac_.assign(update_msg.slot().begin(), update_msg.slot().end());
            break;
            
        case goby::acomms::protobuf::MACUpdate::PUSH_BACK:
            for(int i = 0, n = update_msg.slot_size(); i < n; ++i)
                mac_.push_back(update_msg.slot(i));
            break;
            
        case goby::acomms::protobuf::MACUpdate::PUSH_FRONT:
            for(int i = 0, n = update_msg.slot_size(); i < n; ++i)
                mac_.push_front(update_msg.slot(i));
            break;

        case goby::acomms::protobuf::MACUpdate::POP_BACK:
            mac_.pop_back();
            break;
            
        case goby::acomms::protobuf::MACUpdate::POP_FRONT:
            mac_.pop_front();
            break;

        case goby::acomms::protobuf::MACUpdate::INSERT:
            mac_.insert(it1, update_msg.slot().begin(), update_msg.slot().end());            
            break;
            
        case goby::acomms::protobuf::MACUpdate::ERASE:
            if(update_msg.second_iterator() != -1)
                mac_.erase(it1, it2);
            else
                mac_.erase(it1);
            break;
            
        case goby::acomms::protobuf::MACUpdate::CLEAR:
            mac_.clear();
            break;            
    }
    mac_.update();
    
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
            driver_ = new goby::acomms::MMDriver;
            break;

        case pAcommsHandlerConfig::DRIVER_ABC_EXAMPLE_MODEM:
            driver_ = new goby::acomms::ABCDriver;
            break;
            
        case pAcommsHandlerConfig::DRIVER_NONE: break;
    }    

    // check and propagate modem id
    if(cfg_.modem_id() == goby::acomms::BROADCAST_ID)
        glog.is(DIE) && glog << "modem_id = " << goby::acomms::BROADCAST_ID << " is reserved for broadcast messages. You must specify a modem_id != " << goby::acomms::BROADCAST_ID << " for this vehicle." << std::endl;
    
    publish("MODEM_ID", cfg_.modem_id());    
    publish("VEHICLE_ID", cfg_.modem_id());    
    
    cfg_.mutable_queue_cfg()->set_modem_id(cfg_.modem_id());
    cfg_.mutable_mac_cfg()->set_modem_id(cfg_.modem_id());
    cfg_.mutable_transitional_cfg()->set_modem_id(cfg_.modem_id());
    cfg_.mutable_driver_cfg()->set_modem_id(cfg_.modem_id());
    

    // start goby-acomms classes
    if(driver_) driver_->startup(cfg_.driver_cfg());
    mac_.startup(cfg_.mac_cfg());
    queue_manager_.set_cfg(cfg_.queue_cfg());
    dccl_->set_cfg(cfg_.dccl_cfg());

    // load all shared libraries
    for(int i = 0, n = cfg_.load_shared_library_size(); i < n; ++i)
    {
        glog.is(VERBOSE) &&
            glog << "Loading shared library: " << cfg_.load_shared_library(i) << std::endl;
        
        void* handle = dlopen(cfg_.load_shared_library(i).c_str(), RTLD_LAZY);
        if(!handle)
        {
            glog << die << "Failed ... check path provided or add to /etc/ld.so.conf "
                 << "or LD_LIBRARY_PATH" << std::endl;
        }
        dl_handles.push_back(handle);
    }
    
    
    // load all .proto files
    for(int i = 0, n = cfg_.load_proto_file_size(); i < n; ++i)
    {
        glog.is(VERBOSE) &&
            glog << "Loading protobuf file: " << cfg_.load_proto_file(i) << std::endl;

        
        if(!goby::util::DynamicProtobufManager::descriptor_pool().FindFileByName(
               cfg_.load_proto_file(i)))
            glog.is(DIE) && glog << "Failed to load file." << std::endl;
    }

    // process translator entries
    for(int i = 0, n = cfg_.translator_entry_size(); i < n; ++i)
    {
        typedef boost::shared_ptr<google::protobuf::Message> GoogleProtobufMessagePointer;
        glog.is(VERBOSE) &&
            glog << "Checking translator entry: " << cfg_.translator_entry(i).DebugString()
                 << std::flush;

        // check that the protobuf file is loaded somehow
        GoogleProtobufMessagePointer msg = goby::util::DynamicProtobufManager::new_protobuf_message<GoogleProtobufMessagePointer>(cfg_.translator_entry(i).protobuf_name());

        // validate with DCCL
        dccl_->validate(msg->GetDescriptor());
        
        if(cfg_.translator_entry(i).trigger().type() ==
           goby::moos::protobuf::TranslatorEntry::Trigger::TRIGGER_PUBLISH)
        {
            // subscribe for trigger publish variables
            TesMoosApp::subscribe(cfg_.translator_entry(i).trigger().moos_var(),
                                  boost::bind(&CpAcommsHandler::create_on_publish,
                                              this, _1, cfg_.translator_entry(i)));
        }
        else if(cfg_.translator_entry(i).trigger().type() ==
                goby::moos::protobuf::TranslatorEntry::Trigger::TRIGGER_TIME)
        {
            timers_.push_back(boost::shared_ptr<boost::asio::deadline_timer>(
                                  new boost::asio::deadline_timer(timer_io_service_)));

            boost::asio::deadline_timer& new_timer = *timers_.back();
            
            new_timer.expires_from_now(boost::posix_time::seconds(
                                           cfg_.translator_entry(i).trigger().period()));
            // Start an asynchronous wait.
            new_timer.async_wait(boost::bind(&CpAcommsHandler::create_on_timer, this,
                                             _1, cfg_.translator_entry(i), &new_timer));
        }

        for(int j = 0, m = cfg_.translator_entry(i).create_size(); j < m; ++j)
        {
            // subscribe for all create variables
            subscribe(cfg_.translator_entry(i).create(j).moos_var());
        }
    }


    for(int i = 0, m = cfg_.multiplex_create_moos_var_size(); i < m; ++i)
    {
        TesMoosApp::subscribe(cfg_.multiplex_create_moos_var(i),
                              &CpAcommsHandler::create_on_multiplex_publish, this);
    }    
}

void CpAcommsHandler::queue_receive(const google::protobuf::Message& msg)
{
    std::multimap<std::string, CMOOSMsg> out;    

    out = translator_.protobuf_to_moos(msg);
    
    for(std::multimap<std::string, CMOOSMsg>::iterator it = out.begin(), n = out.end();
        it != n; ++it)
    {
        glog.is(VERBOSE) && glog << "Publishing: " << it->second << std::endl;
        publish(it->second);
    }    
}


void CpAcommsHandler::create_on_publish(const CMOOSMsg& trigger_msg,
                                     const goby::moos::protobuf::TranslatorEntry& entry)
{
    glog.is(VERBOSE) && glog << "Received trigger: " << trigger_msg << std::endl;

    if(!entry.trigger().has_mandatory_content() ||
       trigger_msg.GetString().find(entry.trigger().mandatory_content()) != std::string::npos)
        translate_and_push(entry);
    else
        glog.is(VERBOSE) && glog << "Message missing mandatory content for: " << entry.protobuf_name() << std::endl;
        
}

void CpAcommsHandler::create_on_multiplex_publish(const CMOOSMsg& moos_msg)
{
    std::string moos_msg_str = moos_msg.GetString(); 
    size_t first_space_pos = moos_msg_str.find(' ');

    std::string protobuf_name = moos_msg_str.substr(0, first_space_pos - 0);
    std::string message_contents = moos_msg_str.substr(first_space_pos);

    boost::trim(protobuf_name);
    boost::trim(message_contents);
    
    boost::shared_ptr<google::protobuf::Message> msg =
        goby::util::DynamicProtobufManager::new_protobuf_message(protobuf_name);

    if(&*msg == 0)
    {
        glog.is(WARN) &&
            glog << "Multiplex receive failed: Unknown Protobuf type: " << protobuf_name << "; be sure it is compiled in or directly loaded into the goby::util::DynamicProtobufManager." << std::endl;
        return;
    }

    goby::moos::MOOSTranslation<goby::moos::protobuf::TranslatorEntry::TECHNIQUE_PROTOBUF_TEXT_FORMAT>::parse(message_contents, &*msg);

    std::multimap<std::string, CMOOSMsg> out;    

    out = translator_.protobuf_to_inverse_moos(*msg);
    
    for(std::multimap<std::string, CMOOSMsg>::iterator it = out.begin(), n = out.end();
        it != n; ++it)
    {
        glog.is(VERBOSE) && glog << "Inverse Publishing: " << it->second << std::endl;
        publish(it->second);
    }
}



void CpAcommsHandler::create_on_timer(const boost::system::error_code& error,
                                   const goby::moos::protobuf::TranslatorEntry& entry,
                                   boost::asio::deadline_timer* timer)
{
  if (!error)
  {
      // reset the timer
      timer->expires_at(timer->expires_at() +
                        boost::posix_time::seconds(entry.trigger().period()));
      
      timer->async_wait(boost::bind(&CpAcommsHandler::create_on_timer, this,
                                       _1, entry, timer));
      
      glog.is(VERBOSE) && glog << "Received trigger for: " << entry.protobuf_name() << std::endl;
      glog.is(VERBOSE) && glog << "Next expiry: " << timer->expires_at() << std::endl;
      
      translate_and_push(entry);
  }
}

void CpAcommsHandler::translate_and_push(const goby::moos::protobuf::TranslatorEntry& entry)
{
    boost::shared_ptr<google::protobuf::Message> created_message =
        translator_.moos_to_protobuf<boost::shared_ptr<google::protobuf::Message> >(
            dynamic_vars().all(), entry.protobuf_name());

    glog.is(DEBUG1) &&
        glog << "Created message: \n" << created_message->DebugString() << std::endl;

    queue_manager_.push_message(*created_message);
}


