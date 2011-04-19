// copyright 2010-2011 t. schneider tes@mit.edu
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

#ifndef GOBYD20100914H
#define GOBYD20100914H

#include <map>
#include <boost/unordered_map.hpp>

#include "goby/core/libcore/zeromq_application_base.h"
#include "goby/core/libcore/protobuf_node.h"
#include "goby/core/libdbo/dbo_manager.h"


#include "goby/protobuf/core_database_request.pb.h"
#include "database_config.pb.h"

namespace goby
{
    namespace core
    {
        
        /* class ArbitraryProtobufApplicationBase : public ProtobufApplicationBase */
        /* { */
            

        /* }; */

        class Database : public ZeroMQApplicationBase, public ProtobufNode
        {
          public:
            Database();
          private:
            void loop();
            void protobuf_inbox(const std::string& protobuf_type_name,
                                const void* data,
                                int size);
            
            void init_sql();
            std::string format_filename(const std::string& in);

            void handle_database_request(const void* data, int size, int message_part);
            
            boost::shared_ptr<google::protobuf::Message> new_protobuf_message(
                const std::string& protobuf_type_name);
            const google::protobuf::FileDescriptor* add_protobuf_file(
                const google::protobuf::Descriptor* descriptor);
            const google::protobuf::FileDescriptor* add_protobuf_file(
                const google::protobuf::FileDescriptorProto& proto);
            
            static google::protobuf::DynamicMessageFactory& msg_factory()
            { return msg_factory_; }
            static google::protobuf::DescriptorPool& descriptor_pool()
            { return descriptor_pool_; }

          private:
            static protobuf::DatabaseConfig cfg_;
            zmq::socket_t database_server_;
            DBOManager* dbo_manager_;
            
            // see google::protobuf documentation: this assists in
            // creating messages at runtime
            static google::protobuf::DynamicMessageFactory msg_factory_;
            // see google::protobuf documentation: this assists in
            // creating messages at runtime
            static google::protobuf::DescriptorPool descriptor_pool_;

            enum { MAX_LOOP_FREQ = 1 };
            
        };
        
    }
}



#endif
