// copyright 2010 t. schneider tes@mit.edu
// 
// this file is part of goby_dbo
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

#include <stdexcept>
#include <algorithm>

#include <boost/preprocessor.hpp>

#include <google/protobuf/descriptor.pb.h>

#include <Wt/Dbo/backend/Sqlite3>

#include "goby/util/string.h"
#include "goby/util/time.h"
#include "goby/util/logger.h"
#include "wt_dbo_overloads.h"
#include "goby/core/libdbo/dbo_manager.h"

#include "dbo_plugin.h"


boost::bimap<int, std::string> goby::core::DBOManager::dbo_map;

goby::core::DBOManager* goby::core::DBOManager::inst_ = 0;

using goby::util::goby_time;
using goby::glog;


// singleton class, use this to get pointer
goby::core::DBOManager* goby::core::DBOManager::get_instance()
{
    if(!inst_) inst_ = new goby::core::DBOManager();
    return(inst_);
}

void goby::core::DBOManager::shutdown()
{
    if(inst_) delete inst_;
}


goby::core::DBOManager::DBOManager()
    : index_(0),
      connection_(0),
      session_(0),
      transaction_(0),
      t_last_commit_(goby_time()),
      static_tables_created_(false)
{
    glog.add_group("dbo", goby::util::Colors::lt_green, "database");
    plugin_factory_.add_library("/home/toby/goby/2.0/lib/libgoby_moos_util.so.2");
    plugin_factory_.add_plugin(&protobuf_plugin_);
    DynamicProtobufManager::new_descriptor_hooks.connect(boost::bind(&DBOManager::add_file_descriptor, this, _1));
}

goby::core::DBOManager::~DBOManager()
{
    if(transaction_) delete transaction_;
    if(connection_) delete connection_;
    if(session_) delete session_;
}


void goby::core::DBOManager::add_raw(MarshallingScheme marshalling_scheme,
                                     const std::string& identifier,
                                     const void* data,
                                     int size,
                                     int socket_id)
{
    static int i = -1;
    ++i;
    
    RawEntry *new_entry = new RawEntry();
    new_entry->marshalling_scheme = marshalling_scheme;
    new_entry->identifier = identifier;
    new_entry->unique_id = i;
    new_entry->socket_id = socket_id;

    std::string bytes = std::string(static_cast<const char*>(data), size);
    new_entry->bytes = std::vector<unsigned char>(bytes.begin(), bytes.end());
    new_entry->time = goby::util::as<std::string>(goby::util::goby_time());
                
    session_->add(new_entry);

    DBOPlugin* plugin = plugin_factory_.plugin(marshalling_scheme);
    if(plugin)
        plugin->add_message(session_, i, identifier, data, size);
}

void goby::core::DBOManager::add_file_descriptor(const google::protobuf::FileDescriptor* file_descriptor)
{
    for(int i = 0, n = file_descriptor->message_type_count(); i < n; ++i)
    {
        const google::protobuf::Descriptor* desc = file_descriptor->message_type(i);
        for(int j = 0, m = desc->nested_type_count(); j < m; ++j)
            add_type(desc->nested_type(j));
        add_type(desc);
    }
    
    
}


void goby::core::DBOManager::add_type(const google::protobuf::Descriptor* descriptor)
{
    using goby::util::as;
    
    if(dbo_map.right.count(descriptor->full_name()))
    {
        glog.is(verbose) &&
            glog << group("dbo") << "type with name " << descriptor->full_name()
                 << " already exists" << std::endl;
        return;
    }

    glog.is(verbose) &&
        glog << group("dbo") << "adding type: "
             << descriptor->DebugString() << "\n"
             << "with index: " << index_ << std::endl;
    
    reset_session();
    dbo_map.insert(boost::bimap<int, std::string>::value_type(index_, descriptor->full_name()));

    std::string mangled_name = descriptor->full_name();
    std::replace(mangled_name.begin(), mangled_name.end(), '.', '_');
    mangle_map_.insert(std::make_pair(index_, mangled_name));
    ++index_;

    map_type(descriptor);

    try{ session_->createTables(); }
    catch(Wt::Dbo::Exception& e)
    {
        glog.is(warn) &&
            glog << e.what() << std::endl;
    }
    
    glog.is(verbose) &&
        glog << group("dbo") << "created table for " << descriptor->full_name() << std::endl;
    reset_session();

    // remap all the tables
    map_static_types();
    for(boost::bimap<int, std::string>::left_iterator it = dbo_map.left.begin(),
            n = dbo_map.left.end();
        it != n;
        ++it)
    {
        map_type(DynamicProtobufManager::descriptor_pool().FindMessageTypeByName(it->second));
    }
}

void goby::core::DBOManager::map_type(const google::protobuf::Descriptor* descriptor)
{
    using goby::util::as;
    
    glog.is(verbose) &&
        glog <<group("dbo") << "mapping type: " << descriptor->full_name() << std::endl;
    
    // allows us to select compile time type to use at runtime
    switch(dbo_map.right.at(descriptor->full_name()))
    {
        // preprocessor `for` loop from 0 to GOBY_MAX_PROTOBUF_TYPES
#define BOOST_PP_LOCAL_MACRO(n)                                         \
        case n: session_->mapClass< ProtoBufWrapper<n> >(mangle_map_.find(n)->second.c_str()); break;
#define BOOST_PP_LOCAL_LIMITS (0, GOBY_MAX_PROTOBUF_TYPES)
#include BOOST_PP_LOCAL_ITERATE()
        // end preprocessor `for` loop
        
        default: throw(std::runtime_error(std::string("exceeded maximum number of types allowed: " + as<std::string>(GOBY_MAX_PROTOBUF_TYPES)))); break;
    }
}



void goby::core::DBOManager::add_message(int unique_id, const google::protobuf::Message& msg)
{
    boost::shared_ptr<google::protobuf::Message> new_msg(msg.New());
    new_msg->CopyFrom(msg);
    add_message(unique_id, new_msg);
}

void goby::core::DBOManager::add_message(int unique_id, boost::shared_ptr<google::protobuf::Message> msg)
{
    using goby::util::as;

    glog.is(verbose) &&
        glog << group("dbo") << "adding message of type: " << msg->GetTypeName() << std::endl;
    
    switch(dbo_map.right.at(msg->GetTypeName()))
    {

        // preprocessor `for` loop from 0 to GOBY_MAX_PROTOBUF_TYPES
#define BOOST_PP_LOCAL_MACRO(n)                                         \
        case n: session_->add(new ProtoBufWrapper<n>(unique_id, msg)); break;
#define BOOST_PP_LOCAL_LIMITS (0, GOBY_MAX_PROTOBUF_TYPES)
#include BOOST_PP_LOCAL_ITERATE()
        // end preprocessor `for` loop

        default: throw(std::runtime_error(std::string("exceeded maximum number of types allowed: " + as<std::string>(GOBY_MAX_PROTOBUF_TYPES)))); break;
    }
}

void goby::core::DBOManager::commit()
{
    glog.is(verbose) &&
        glog << group("dbo") << "starting commit" << std::endl;
        
    transaction_->commit();
        
    glog.is(verbose) &&
        glog << group("dbo") << "finished commit" << std::endl;

    t_last_commit_ = goby_time();

    delete transaction_;
    transaction_ = new Wt::Dbo::Transaction(*session_);
}



void goby::core::DBOManager::connect(const std::string& db_name /* = "" */)
{
    if(!db_name.empty())
        db_name_ = db_name;

    if(transaction_) delete transaction_;
    if(connection_) delete connection_;
    if(session_) delete session_;
    connection_ = new Wt::Dbo::backend::Sqlite3(db_name_);
    session_ = new Wt::Dbo::Session;
    session_->setConnection(*connection_);
    // transaction deleted by session
    transaction_ = new Wt::Dbo::Transaction(*session_);
    
    if(!static_tables_created_)
    {
        map_static_types();
        try{ session_->createTables(); }
        catch(Wt::Dbo::Exception& e)
        {
            glog.is(warn) &&
                glog << e.what() << std::endl;
        }

        glog.is(verbose) &&
            glog << group("dbo") << "created table for static types" << std::endl;
        static_tables_created_ = true;
        reset_session();
    }
    
    
}

void goby::core::DBOManager::reset_session()
{

    commit();
    glog.is(verbose) &&
        glog << "resetting session" << std::endl;
    connect();
}
            

void goby::core::DBOManager::map_static_types()
{
    session_->mapClass<RawEntry>("_raw");

    DBOPlugin* plugin = plugin_factory_.plugin(MARSHALLING_MOOS);
    if(plugin)
        plugin->map_type(session_);
}
