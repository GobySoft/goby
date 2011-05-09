// copyright 2011 t. schneider tes@mit.edu
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

#ifndef DYNAMICPROTOBUFMANAGER20110419H
#define DYNAMICPROTOBUFMANAGER20110419H

#include <set>

#include <google/protobuf/descriptor.h>
#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/dynamic_message.h>

#include <boost/shared_ptr.hpp>

namespace goby
{
    namespace core
    {
        class DynamicProtobufManager
        {
          public:
            static boost::shared_ptr<google::protobuf::Message> new_protobuf_message(
                const std::string& protobuf_type_name);
            static std::set<const google::protobuf::FileDescriptor*>
                add_protobuf_file_with_dependencies(
                const google::protobuf::FileDescriptor* file_descriptor);
            static const google::protobuf::FileDescriptor* add_protobuf_file(
                const google::protobuf::FileDescriptor* file_descriptor);
            static const google::protobuf::FileDescriptor* add_protobuf_file(
                const google::protobuf::FileDescriptorProto& proto);
            
            static google::protobuf::DynamicMessageFactory& msg_factory()
            { return msg_factory_; }
            static google::protobuf::DescriptorPool& descriptor_pool()
            { return descriptor_pool_; }

            
          private:
                      
            // see google::protobuf documentation: this assists in
            // creating messages at runtime
            static google::protobuf::DynamicMessageFactory msg_factory_;
            // see google::protobuf documentation: this assists in
            // creating messages at runtime
            static google::protobuf::DescriptorPool descriptor_pool_;
            
        };
        
    }
}

#endif
