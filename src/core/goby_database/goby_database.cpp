// copyright 2011 t. schneider tes@mit.edu
// 
// the file is the goby database, part of the core goby autonomy system
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

#include <boost/thread.hpp>
#include <boost/format.hpp>

#include "goby/util/logger.h"

#include "goby_database.h"

goby::core::protobuf::DatabaseConfig goby::core::Database::cfg_;
google::protobuf::DynamicMessageFactory goby::core::Database::msg_factory_;
google::protobuf::DescriptorPool goby::core::Database::descriptor_pool_;

using goby::util::as;
using goby::util::glogger;

int main(int argc, char* argv[])
{
    goby::run<Database>(argc, argv);
}


goby::core::ArbitraryTypeSubscription::ArbitraryTypeSubscription()
{ }

void goby::core::ArbitraryTypeSubscription::post(const void* data, int size, boost::shared_ptr<google::protobuf::Message> msg)
{
    msg->ParseFromArray(data, size);
    if(handler_) handler_(msg);
}



goby::core::Database::Database()
    : MinimalApplicationBase(&glogger()),
      database_server_(ApplicationBase::zmq_context(), ZMQ_REP),
      dbo_manager_(DBOManager::get_instance())
{
    if(!cfg_.base().using_database())
        glogger() << die << "AppBaseConfig::using_database == false. Since we aren't wanting, we aren't starting (set to true to enable use of the database)!" << std::endl;
    

    std::string database_binding = "tcp://*:";
    if(cfg_.base().has_database_port())
        database_binding += as<std::string>(cfg_.base().database_port());
    else
        database_binding += as<std::string>(cfg_.base().ethernet_port());

    try
    {
        database_server_.bind(database_binding.c_str());
        glogger() << debug << "bound (requests line) to: "
                  << database_binding << std::endl;
        
    }
    catch(std::exception& e)
    {
        glogger() << die << "cannot bind to: "
                  << database_binding << ": " << e.what()
                  << " check AppBaseConfig::database_port";
    }

    // subscribe for everything
    subscribe(MARSHALLING_PROTOBUF, "");
    set_subscribe_advanced_handler(&goby::core::Database::inbox, this);

    init_sql();
}

void goby::core::Database::init_sql()
{
    dbo_manager_->set_dynamic_message_factory(&ApplicationBase::msg_factory());
    dbo_manager_->set_descriptor_pool(&ApplicationBase::descriptor_pool());    
    
    try
    {
        cfg_.mutable_sqlite()->set_path(format_filename(cfg_.sqlite().path()));        
        dbo_manager_->connect(std::string(cfg_.sqlite().path()));
            
    }
    catch(std::exception& e)
    {
        cfg_.mutable_sqlite()->clear_path();
        cfg_.mutable_sqlite()->set_path(format_filename(cfg_.sqlite().path()));
        
        glogger() << warn << "db connection failed: "
                  << e.what() << std::endl;
        std::string default_file = cfg_.sqlite().path();
            
        glogger() << "trying again with defaults: "
                  << default_file << std::endl;

        dbo_manager_->connect(default_file);
    }
    
    // #include <google/protobuf/descriptor.pb.h>
    const google::protobuf::FileDescriptor* file_desc =
        add_protobuf_file(google::protobuf::FileDescriptorProto::descriptor());

}

std::string goby::core::Database::format_filename(const std::string& in)
{   
    boost::format f(in);
    // don't thrown if user doesn't need some or all of format fields
    f.exceptions(boost::io::all_error_bits^(
                     boost::io::too_many_args_bit | boost::io::too_few_args_bit));
    f % cfg_.base().platform_name() % goby::util::goby_file_timestamp();
    return f.str();
}


void inbox(MarshallingScheme marshalling_scheme,
           const std::string& identifier,
           const void* data,
           int size)
{
    if(static_cast<MarshallingScheme>(marshalling_scheme) != MARSHALLING_PROTOBUF)
    {
        throw(std::runtime_error("Unknown Marshalling Scheme; should be MARSHALLING_PROTOBUF, got: " + as<std::string>(marshalling_scheme)));
    }

    if(arbitrary_type_subscription_.has_valid_handler())
        arbitrary_type_subscription_.post(message.data(),
                                          message.size(),
                                          new_protobuf_message(protobuf_type_name));
}


void goby::core::Database::inbox(boost::shared_ptr<google::protobuf::Message> msg)
{
    glogger() << *msg << std::endl;
    dbo_manager_->add_message(msg);
}

void goby::core::Database::run()
{  
    for(;;)
    {
        
    }
}

void goby::core::Database::loop()
{
    static protobuf::DatabaseRequest proto_request;
    static protobuf::DatabaseResponse proto_response;
    
    for(;;)
    {
        zmq::message_t zmq_request;

        bool rc = database_server_.recv(&zmq_request, ZMQ_NOBLOCK);
        if(!rc) break;
        
        proto_request.ParseFromArray(zmq_request.data(), zmq_request.size());
        glogger() << debug << "Got request: " << proto_request << std::endl;

        switch(proto_request.request_type())
        {
            case protobuf::DatabaseRequest::NEW_PUBLISH:
            {
                for(int i = 0, n = proto_request.file_descriptor_proto_size(); i < n; ++i)
                {
                    const google::protobuf::FileDescriptor* file_desc =
                        add_protobuf_file(proto_request.file_descriptor_proto(i));
                        
                    const google::protobuf::Descriptor* desc =
                        descriptor_pool().FindMessageTypeByName(
                            proto_request.publish_protobuf_full_name());
                    proto_response.Clear();
                    
                    if(!desc)
                    {
                        proto_response.set_response_type(protobuf::DatabaseResponse::NEW_PUBLISH_DENIED);
                    } 
                    else
                    {
                        dbo_manager_->add_type(desc);
                        proto_response.set_response_type(protobuf::DatabaseResponse::NEW_PUBLISH_ACCEPTED);
                    }
                }

                zmq::message_t zmq_response(proto_response.ByteSize());
                proto_response.SerializeToArray(zmq_response.data(), zmq_response.size());    
                database_server_.send(zmq_response);
                glogger() << debug << "Send response: " << proto_response << std::endl;
            }
            break;
            
            case protobuf::DatabaseRequest::SQL_QUERY:
                // unimplemented
                break;
        }            
    }

    dbo_manager_->commit();
}


boost::shared_ptr<google::protobuf::Message> goby::core::Database::new_protobuf_message(const std::string& protobuf_type_name)
{
    const google::protobuf::Descriptor* desc = descriptor_pool_.FindMessageTypeByName(protobuf_type_name);

    if(desc)
        return boost::shared_ptr<google::protobuf::Message>(
            msg_factory_.GetPrototype(desc)->New());
    else
        throw(std::runtime_error("Unknown type " + name + ", be sure it is loaded with call to ApplicationBase::add_protobuf_file()"));
}



const google::protobuf::FileDescriptor* goby::core::Database::add_protobuf_file(const google::protobuf::Descriptor* descriptor)
{
    google::protobuf::FileDescriptorProto proto_file;
    descriptor->file()->CopyTo(&proto_file);
    return add_protobuf_file(proto_file);
}
    

const google::protobuf::FileDescriptor* goby::core::Database::add_protobuf_file(const google::protobuf::FileDescriptorProto& proto)
{
    return descriptor_pool_.BuildFile(proto); 
}

