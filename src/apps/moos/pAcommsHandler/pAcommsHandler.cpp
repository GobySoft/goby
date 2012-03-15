// Copyright 2009-2012 Toby Schneider (https://launchpad.net/~tes)
//                     Massachusetts Institute of Technology (2007-)
//                     Woods Hole Oceanographic Institution (2007-)
//                     Goby Developers Team (https://launchpad.net/~goby-dev)
// 
//
// This file is part of the Goby Underwater Autonomy Project Binaries
// ("The Goby Binaries").
//
// The Goby Binaries are free software: you can redistribute them and/or modify
// them under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// The Goby Binaries are distributed in the hope that they will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Goby.  If not, see <http://www.gnu.org/licenses/>.

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
#include "goby/moos/moos_protobuf_helpers.h"
#include "goby/moos/moos_ufield_sim_driver.h"
#include "goby/moos/protobuf/ufield_sim_driver.pb.h"

using namespace goby::common::tcolor;
using namespace goby::common::logger;
using goby::acomms::operator<<;
using goby::moos::operator<<;
using goby::util::as;
using google::protobuf::uint32;


using goby::glog;


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
    : GobyMOOSApp(&cfg_),
      source_database_(&disk_source_tree_),
      translator_(goby::moos::protobuf::TranslatorEntry(),
                  cfg_.common().lat_origin(),
                  cfg_.common().lon_origin(),
                  cfg_.modem_id_lookup_path()),
      dccl_(goby::acomms::DCCLCodec::get()),
      work_(timer_io_service_)
{
    goby::common::goby_time_function = boost::bind(&CpAcommsHandler::microsec_moos_time, this);

    
    source_database_.RecordErrorsTo(&error_collector_);
    disk_source_tree_.MapPath("/", "/");
    goby::util::DynamicProtobufManager::add_database(&source_database_);

#ifdef ENABLE_GOBY_V1_TRANSITIONAL_SUPPORT
    transitional_dccl_.convert_to_v2_representation(&cfg_);
    glog.is(VERBOSE) && glog << group("pAcommsHandler") << "Configuration after transitional configuration modifications: \n" << cfg_ << std::flush;
#else
    if(cfg_.has_transitional_cfg())
        glog.is(DIE) && glog << "transitional_cfg is set but pAcommsHandler was not compiled with the CMake flag 'enable_goby_v1_transitional_support' set to ON" << std::endl;
#endif
    
    
    translator_.add_entry(cfg_.translator_entry());
    
    
    goby::acomms::connect(&queue_manager_.signal_receive,
                          this, &CpAcommsHandler::handle_queue_receive);


    // informational 'queue' signals
    goby::acomms::connect(&queue_manager_.signal_ack,
                          boost::bind(&CpAcommsHandler::handle_goby_signal, this,
                                      _1, cfg_.moos_var().queue_ack_transmission(),
                                      _2, cfg_.moos_var().queue_ack_original_msg()));
    goby::acomms::connect(&queue_manager_.signal_receive,
                          boost::bind(&CpAcommsHandler::handle_goby_signal, this,
                                      _1, cfg_.moos_var().queue_receive(), _1, ""));
    goby::acomms::connect(&queue_manager_.signal_expire,
                          boost::bind(&CpAcommsHandler::handle_goby_signal, this,
                                      _1, cfg_.moos_var().queue_expire(), _1, ""));
    goby::acomms::connect(&queue_manager_.signal_queue_size_change,
                          boost::bind(&CpAcommsHandler::handle_goby_signal, this,
                                      _1, cfg_.moos_var().queue_size(), _1, ""));

    // informational 'mac' signals
    goby::acomms::connect(&mac_.signal_initiate_transmission,
                          boost::bind(&CpAcommsHandler::handle_goby_signal, this,
                                      _1, cfg_.moos_var().mac_initiate_transmission(), _1, ""));


    goby::acomms::connect(&queue_manager_.signal_data_on_demand,
                          this, &CpAcommsHandler::handle_encode_on_demand);

    process_configuration();

    // bind the lower level pieces of goby-acomms together
    if(driver_)
    {
        goby::acomms::bind(*driver_, queue_manager_);
        goby::acomms::bind(mac_, *driver_);

        // informational 'driver' signals
        goby::acomms::connect(&driver_->signal_receive,
                              boost::bind(&CpAcommsHandler::handle_goby_signal, this,
                                          _1, cfg_.moos_var().driver_receive(), _1, ""));

        goby::acomms::connect(&driver_->signal_transmit_result,
                              boost::bind(&CpAcommsHandler::handle_goby_signal, this,
                                          _1, cfg_.moos_var().driver_transmit(), _1, ""));

        goby::acomms::connect(&driver_->signal_raw_incoming,
                              boost::bind(&CpAcommsHandler::handle_goby_signal, this,
                                          _1, cfg_.moos_var().driver_raw_msg_in(), _1, ""));
        goby::acomms::connect(&driver_->signal_raw_outgoing,
                              boost::bind(&CpAcommsHandler::handle_goby_signal, this,
                                          _1, cfg_.moos_var().driver_raw_msg_out(), _1, ""));    

        goby::acomms::connect(&driver_->signal_raw_incoming,
                              boost::bind(&CpAcommsHandler::handle_raw, this,
                                          _1, cfg_.moos_var().driver_raw_in()));
        
        goby::acomms::connect(&driver_->signal_raw_outgoing,
                              boost::bind(&CpAcommsHandler::handle_raw, this,
                                          _1, cfg_.moos_var().driver_raw_out()));
        
        
    }    
    
    // update comms cycle
    subscribe(cfg_.moos_var().mac_cycle_update(), &CpAcommsHandler::handle_mac_cycle_update, this);    
    
    subscribe(cfg_.moos_var().queue_flush(), &CpAcommsHandler::handle_flush_queue, this);    
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
    
    glog << group("pAcommsHandler") << "got update for MAC: " << update_msg << std::endl;

    if(!update_msg.dest() == cfg_.modem_id())
    {
        glog << group("pAcommsHandler") << "update not for us" << std::endl;
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
            if(mac_.size())
                mac_.pop_back();
            else
                glog.is(WARN) && glog << group("pAcommsHandler") << "Cannot POP_BACK of empty MAC cycle" << std::endl;
            break;
            
        case goby::acomms::protobuf::MACUpdate::POP_FRONT:
            if(mac_.size())
                mac_.pop_front();
            else
                glog.is(WARN) && glog << group("pAcommsHandler") << "Cannot POP_FRONT of empty MAC cycle" << std::endl;
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

        case goby::acomms::protobuf::MACUpdate::NO_CHANGE:
            break;
    }

    mac_.update();

    if(update_msg.has_cycle_state())
    {
        switch(update_msg.cycle_state())
        {
            case goby::acomms::protobuf::MACUpdate::STARTED:
                mac_.restart();
                break;
            
            case goby::acomms::protobuf::MACUpdate::STOPPED:
                mac_.shutdown();
                break;
        }
    }

    
    
}


void CpAcommsHandler::handle_flush_queue(const CMOOSMsg& msg)
{
    goby::acomms::protobuf::QueueFlush flush;
    parse_for_moos(msg.GetString(), &flush);
    
    glog.is(VERBOSE) && glog << group("pAcommsHandler") <<  "Queue flush request: " << flush << std::endl;
    queue_manager_.flush_queue(flush);
}



void CpAcommsHandler::handle_goby_signal(const google::protobuf::Message& msg1,
                                         const std::string& moos_var1,
                                         const google::protobuf::Message& msg2,
                                         const std::string& moos_var2)

{
    if(!moos_var1.empty())
    {
        std::string serialized1;
        serialize_for_moos(&serialized1, msg1);
        publish(moos_var1, serialized1);
    }
    
    if(!moos_var2.empty())
    {
        std::string serialized2;
        serialize_for_moos(&serialized2, msg2);
        publish(moos_var2, serialized2);
    }
}

void CpAcommsHandler::handle_raw(const goby::acomms::protobuf::ModemRaw& msg, const std::string& moos_var)
{
    publish(moos_var, msg.raw());
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

        case pAcommsHandlerConfig::DRIVER_UFIELD_SIM_DRIVER:
            driver_ = new goby::moos::UFldDriver;
            cfg_.mutable_driver_cfg()->SetExtension(
                goby::moos::protobuf::Config::modem_id_lookup_path,
                cfg_.modem_id_lookup_path());
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
            glog << group("pAcommsHandler") << "Loading shared library: " << cfg_.load_shared_library(i) << std::endl;
        
        void* handle = dlopen(cfg_.load_shared_library(i).c_str(), RTLD_LAZY);
        if(!handle)
        {
            glog.is(DIE) && glog << "Failed ... check path provided or add to /etc/ld.so.conf "
                         << "or LD_LIBRARY_PATH" << std::endl;
        }
        dl_handles.push_back(handle);

        // load any shared library codecs
        void (*dccl_load_ptr)(goby::acomms::DCCLCodec*);
        dccl_load_ptr = (void (*)(goby::acomms::DCCLCodec*)) dlsym(handle, "goby_dccl_load");
        if(dccl_load_ptr)
        {
            glog << group("pAcommsHandler") << "Loading shared library dccl codecs." << std::endl;
            (*dccl_load_ptr)(dccl_);
        }
        
    }
    
    
    // load all .proto files
    for(int i = 0, n = cfg_.load_proto_file_size(); i < n; ++i)
    {
        glog.is(VERBOSE) &&
            glog << group("pAcommsHandler") << "Loading protobuf file: " << cfg_.load_proto_file(i) << std::endl;

        
        if(!goby::util::DynamicProtobufManager::descriptor_pool().FindFileByName(
               cfg_.load_proto_file(i)))
            glog.is(DIE) && glog << "Failed to load file." << std::endl;
    }

    // process translator entries
    for(int i = 0, n = cfg_.translator_entry_size(); i < n; ++i)
    {
        typedef boost::shared_ptr<google::protobuf::Message> GoogleProtobufMessagePointer;
        glog.is(VERBOSE) &&
            glog << group("pAcommsHandler") << "Checking translator entry: " << cfg_.translator_entry(i).protobuf_name() << std::endl;

        // check that the protobuf file is loaded somehow
        GoogleProtobufMessagePointer msg = goby::util::DynamicProtobufManager::new_protobuf_message<GoogleProtobufMessagePointer>(cfg_.translator_entry(i).protobuf_name());

        // validate with DCCL
        dccl_->validate(msg->GetDescriptor());
        queue_manager_.add_queue(msg->GetDescriptor());
        
        if(cfg_.translator_entry(i).trigger().type() ==
           goby::moos::protobuf::TranslatorEntry::Trigger::TRIGGER_PUBLISH)
        {
            // subscribe for trigger publish variables
            GobyMOOSApp::subscribe(cfg_.translator_entry(i).trigger().moos_var(),
                                  boost::bind(&CpAcommsHandler::create_on_publish,
                                              this, _1, cfg_.translator_entry(i)));
        }
        else if(cfg_.translator_entry(i).trigger().type() ==
                goby::moos::protobuf::TranslatorEntry::Trigger::TRIGGER_TIME)
        {
            timers_.push_back(boost::shared_ptr<Timer>(
                                  new Timer(timer_io_service_)));

            Timer& new_timer = *timers_.back();
            
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
        GobyMOOSApp::subscribe(cfg_.multiplex_create_moos_var(i),
                              &CpAcommsHandler::create_on_multiplex_publish, this);
    }    
}

void CpAcommsHandler::handle_queue_receive(const google::protobuf::Message& msg)
{
    std::multimap<std::string, CMOOSMsg> out;    

    out = translator_.protobuf_to_moos(msg);
    
    for(std::multimap<std::string, CMOOSMsg>::iterator it = out.begin(), n = out.end();
        it != n; ++it)
    {
        glog.is(VERBOSE) && glog << group("pAcommsHandler") << "Publishing: " << it->second << std::endl;
        publish(it->second);
    }    
}


void CpAcommsHandler::handle_encode_on_demand(const goby::acomms::protobuf::ModemTransmission& request_msg, google::protobuf::Message* data_msg)
{
    glog.is(VERBOSE) && glog << group("pAcommsHandler") << "Received encode on demand request: " << request_msg << std::endl;

    
    boost::shared_ptr<google::protobuf::Message> created_message =
        translator_.moos_to_protobuf<boost::shared_ptr<google::protobuf::Message> >(dynamic_vars().all(), data_msg->GetDescriptor()->full_name());

    data_msg->CopyFrom(*created_message);
}



void CpAcommsHandler::create_on_publish(const CMOOSMsg& trigger_msg,
                                     const goby::moos::protobuf::TranslatorEntry& entry)
{
    glog.is(VERBOSE) && glog << group("pAcommsHandler") << "Received trigger: " << trigger_msg.GetKey() << std::endl;

    if(!entry.trigger().has_mandatory_content() ||
       trigger_msg.GetString().find(entry.trigger().mandatory_content()) != std::string::npos)
        translate_and_push(entry);
    else
        glog.is(VERBOSE) && glog << group("pAcommsHandler") << "Message missing mandatory content for: " << entry.protobuf_name() << std::endl;
        
}

void CpAcommsHandler::create_on_multiplex_publish(const CMOOSMsg& moos_msg)
{
    boost::shared_ptr<google::protobuf::Message> msg =
        dynamic_parse_for_moos(moos_msg.GetString());

    if(&*msg == 0)
    {
        glog.is(WARN) &&
             glog << group("pAcommsHandler") << "Multiplex receive failed: Unknown Protobuf type for " << moos_msg.GetString() << "; be sure it is compiled in or directly loaded into the goby::util::DynamicProtobufManager." << std::endl;
        return;
    }

    std::multimap<std::string, CMOOSMsg> out;    

    out = translator_.protobuf_to_inverse_moos(*msg);
    
    for(std::multimap<std::string, CMOOSMsg>::iterator it = out.begin(), n = out.end();
        it != n; ++it)
    {
        glog.is(VERBOSE) && glog << group("pAcommsHandler") << "Inverse Publishing: " << it->second.GetKey() << std::endl;
        publish(it->second);
    }
}



void CpAcommsHandler::create_on_timer(const boost::system::error_code& error,
                                   const goby::moos::protobuf::TranslatorEntry& entry,
                                   Timer* timer)
{
  if (!error)
  {
      // reset the timer
      timer->expires_at(timer->expires_at() +
                        boost::posix_time::seconds(entry.trigger().period()));
      
      timer->async_wait(boost::bind(&CpAcommsHandler::create_on_timer, this,
                                       _1, entry, timer));
      
      glog.is(VERBOSE) && glog << group("pAcommsHandler") << "Received trigger for: " << entry.protobuf_name() << std::endl;
      glog.is(VERBOSE) && glog << group("pAcommsHandler") << "Next expiry: " << timer->expires_at() << std::endl;
      
      translate_and_push(entry);
  }
}

void CpAcommsHandler::translate_and_push(const goby::moos::protobuf::TranslatorEntry& entry)
{
    boost::shared_ptr<google::protobuf::Message> created_message =
        translator_.moos_to_protobuf<boost::shared_ptr<google::protobuf::Message> >(
            dynamic_vars().all(), entry.protobuf_name());

    glog.is(DEBUG1) &&
        glog << group("pAcommsHandler") << "Created message: \n" << created_message->DebugString() << std::endl;

    queue_manager_.push_message(*created_message);
}


