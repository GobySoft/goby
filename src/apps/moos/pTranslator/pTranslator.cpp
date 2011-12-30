// t. schneider tes@mit.edu 06.05.08
// ocean engineering graudate student - mit / whoi joint program
// massachusetts institute of technology (mit)
// laboratory for autonomous marine sensing systems (lamss)
// 
// this is pTranslator.cpp, part of pTranslator 
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

#include "goby/moos/moos_string.h"

#include "pTranslator.h"


using goby::glog;
using namespace goby::util::logger;
using goby::moos::operator<<;

pTranslatorConfig CpTranslator::cfg_;
CpTranslator* CpTranslator::inst_ = 0;

CpTranslator* CpTranslator::get_instance()
{
    if(!inst_)
        inst_ = new CpTranslator();
    return inst_;
}

void CpTranslator::delete_instance()
{
    delete inst_;
}

CpTranslator::CpTranslator()
    : TesMoosApp(&cfg_),
      source_database_(&disk_source_tree_),
      translator_(cfg_.translator_entry(),
                  cfg_.common().lat_origin(),
                  cfg_.common().lon_origin(),
                  cfg_.modem_id_lookup_path()),
      work_(timer_io_service_)
{ 
    source_database_.RecordErrorsTo(&error_collector_);
    disk_source_tree_.MapPath("/", "/");
    goby::util::DynamicProtobufManager::add_database(&source_database_);


    // load all shared libraries
    for(int i = 0, n = cfg_.load_shared_library_size(); i < n; ++i)
    {
        glog.is(VERBOSE) &&
            glog << "Loading shared library: " << cfg_.load_shared_library(i) << std::endl;
        
        void* handle = dlopen(cfg_.load_shared_library(i).c_str(), RTLD_LAZY);
        if(!handle)
        {
            glog.is(DIE) &&
                glog << "Failed ... check path provided or add to /etc/ld.so.conf "
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
        goby::util::DynamicProtobufManager::new_protobuf_message<GoogleProtobufMessagePointer>(
            cfg_.translator_entry(i).protobuf_name());

        if(cfg_.translator_entry(i).trigger().type() ==
           goby::moos::protobuf::TranslatorEntry::Trigger::TRIGGER_PUBLISH)
        {
            // subscribe for trigger publish variables
            TesMoosApp::subscribe(cfg_.translator_entry(i).trigger().moos_var(),
                                  boost::bind(&CpTranslator::create_on_publish,
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
            new_timer.async_wait(boost::bind(&CpTranslator::create_on_timer, this,
                                             _1, cfg_.translator_entry(i), &new_timer));
        }

        for(int j = 0, m = cfg_.translator_entry(i).create_size(); j < m; ++j)
        {
            // subscribe for all create variables
            subscribe(cfg_.translator_entry(i).create(j).moos_var());
        }
    }
}

CpTranslator::~CpTranslator()
{


}



void CpTranslator::loop()
{
    timer_io_service_.poll();
}


void CpTranslator::create_on_publish(const CMOOSMsg& trigger_msg,
                                     const goby::moos::protobuf::TranslatorEntry& entry)
{
    glog.is(VERBOSE) && glog << "Received trigger: " << trigger_msg << std::endl;

    if(!entry.trigger().has_mandatory_content() ||
       trigger_msg.GetString().find(entry.trigger().mandatory_content()) != std::string::npos)
        do_translation(entry);
}

void CpTranslator::create_on_timer(const boost::system::error_code& error,
                                   const goby::moos::protobuf::TranslatorEntry& entry,
                                   boost::asio::deadline_timer* timer)
{
  if (!error)
  {
      // reset the timer
      timer->expires_at(timer->expires_at() +
                        boost::posix_time::seconds(entry.trigger().period()));
      
      timer->async_wait(boost::bind(&CpTranslator::create_on_timer, this,
                                       _1, entry, timer));
      
      glog.is(VERBOSE) && glog << "Received trigger for: " << entry.protobuf_name() << std::endl;
      glog.is(VERBOSE) && glog << "Next expiry: " << timer->expires_at() << std::endl;
      
      do_translation(entry);
  }
}

void CpTranslator::do_translation(const goby::moos::protobuf::TranslatorEntry& entry)
{
    std::multimap<std::string, CMOOSMsg> out;

    boost::shared_ptr<google::protobuf::Message> created_message =
        translator_.moos_to_protobuf<boost::shared_ptr<google::protobuf::Message> >(
            dynamic_vars().all(), entry.protobuf_name());

    glog.is(DEBUG1) &&
        glog << "Created message: \n" << created_message->DebugString() << std::endl;
    
    
    out = translator_.protobuf_to_moos(*created_message);

    for(std::multimap<std::string, CMOOSMsg>::iterator it = out.begin(), n = out.end();
        it != n; ++it)
    {
        glog.is(VERBOSE) && glog << "Publishing: " << it->second << std::endl;
        publish(it->second);
    }


    throw(std::runtime_error(""));
    
    
}

