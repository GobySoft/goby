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

#include <boost/preprocessor.hpp>

#include "goby/util/string.h"
#include "goby/util/time.h"

#include "wt_dbo_overloads.h"
#include "dbo_manager.h"


// must be define since we are using the preprocessor
#define GOBY_MAX_PROTOBUF_TYPES 16

google::protobuf::DynamicMessageFactory goby::core::DBOManager::msg_factory;
google::protobuf::DescriptorPool goby::core::DBOManager::descriptor_pool;
boost::bimap<int, std::string> goby::core::DBOManager::dbo_map;
int goby::core::DBOManager::index = 0;

goby::core::DBOManager* goby::core::DBOManager::inst_ = 0;

boost::posix_time::time_duration goby::core::DBOManager::t_between_commits_ = boost::posix_time::seconds(1);

using goby::util::goby_time;


// singleton class, use this to get pointer
goby::core::DBOManager* goby::core::DBOManager::get_instance()
{
    if(!inst_) inst_ = new goby::core::DBOManager();
    return(inst_);
}

goby::core::DBOManager::DBOManager()
    : log_(0),
      connection_(0),
      transaction_(0),
      t_last_commit_(goby_time())
{}

void goby::core::DBOManager::add_flex_groups(util::FlexOstream& tout)
{
    tout.add_group("dbo", "d", "lt_green", "database");
}          

void goby::core::DBOManager::add_file(const google::protobuf::FileDescriptorProto& proto)
{
    const google::protobuf::FileDescriptor* new_file_descriptor = descriptor_pool.BuildFile(proto);    
    for(int i = 0, n = new_file_descriptor->message_type_count(); i < n; ++i)
        add_type(new_file_descriptor->message_type(i));
}

// add check for type if name already exists in the database!
void goby::core::DBOManager::add_type(const google::protobuf::Descriptor* descriptor)
{
    using goby::util::as;

    if(dbo_map.right.count(descriptor->full_name()))
    {
        if(log_) *log_ << group("dbo") << "type with name " << descriptor->full_name() << " already exists" << std::endl;
        return;
    }

    if(log_) *log_ << group("dbo") << "adding type: \n"
                 << descriptor->DebugString() << std::endl;
    
    dbo_map.insert(boost::bimap<int, std::string>::value_type(index, descriptor->full_name()));

    // allows us to select compile time type to use at runtime
    switch(index)
    {
        // preprocessor `for` loop from 0 to GOBY_MAX_PROTOBUF_TYPES
#define BOOST_PP_LOCAL_MACRO(n)                                         \
        case n: session_.mapClass< ProtoBufWrapper<n> >(dbo_map.left.at(n).c_str()); break;
#define BOOST_PP_LOCAL_LIMITS (0, GOBY_MAX_PROTOBUF_TYPES)
#include BOOST_PP_LOCAL_ITERATE()
        // end preprocessor `for` loop

        default: throw(std::runtime_error(std::string("exceeded maximum number of types allowed: " + as<std::string>(GOBY_MAX_PROTOBUF_TYPES)))); break;
    }

    // if the tables are not created, make them now
    // maybe pull this out to a separate function call in the future
    try
    { session_.createTables(); }
    catch(...)
    { }

    ++index;
}

void goby::core::DBOManager::add_message(const std::string& name, const std::string& serialized_message)
{
    google::protobuf::Message* msg = new_msg_from_name(name);
    msg->ParseFromString(serialized_message);
    add_message(msg);
}

void goby::core::DBOManager::add_message(google::protobuf::Message* msg)
{
    using goby::util::as;
    
    
    if(log_) *log_ << group("dbo") << "adding message: \n"
                 << msg->ShortDebugString() << std::endl;

    
    switch(dbo_map.right.at(msg->GetTypeName()))
    {

        // preprocessor `for` loop from 0 to GOBY_MAX_PROTOBUF_TYPES
#define BOOST_PP_LOCAL_MACRO(n)                                         \
        case n: session_.add(new ProtoBufWrapper<n>(msg)); break;
#define BOOST_PP_LOCAL_LIMITS (0, GOBY_MAX_PROTOBUF_TYPES)
#include BOOST_PP_LOCAL_ITERATE()
        // end preprocessor `for` loop


        default: throw(std::runtime_error(std::string("exceeded maximum number of types allowed: " + as<std::string>(GOBY_MAX_PROTOBUF_TYPES)))); break;
    }
}

void goby::core::DBOManager::commit()
{
    if(log_) *log_ << group("dbo") << "starting commit"
                   << std::endl;
        
    transaction_->commit();
        
    if(log_) *log_ << group("dbo") << "finished commit"
                   << std::endl;

    t_last_commit_ = goby_time();
    delete transaction_;
    transaction_ = new Wt::Dbo::Transaction(session_);
}

