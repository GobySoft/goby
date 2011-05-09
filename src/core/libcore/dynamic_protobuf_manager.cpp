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

#include "dynamic_protobuf_manager.h"

#include "exception.h"

google::protobuf::DynamicMessageFactory goby::core::DynamicProtobufManager::msg_factory_;
google::protobuf::DescriptorPool goby::core::DynamicProtobufManager::descriptor_pool_;


boost::shared_ptr<google::protobuf::Message> goby::core::DynamicProtobufManager::new_protobuf_message(const std::string& protobuf_type_name)
{
    const google::protobuf::Descriptor* desc = descriptor_pool_.FindMessageTypeByName(protobuf_type_name);

    if(desc)
        return boost::shared_ptr<google::protobuf::Message>(
            msg_factory_.GetPrototype(desc)->New());
    else
        throw(std::runtime_error("Unknown type " + protobuf_type_name + ", be sure it is loaded with call to add_protobuf_file()"));
}

const google::protobuf::FileDescriptor* goby::core::DynamicProtobufManager::add_protobuf_file_with_dependencies(const google::protobuf::FileDescriptor* file_descriptor)
{
    for(int i = 0, n = file_descriptor->dependency_count(); i < n; ++i)
    {
        google::protobuf::FileDescriptorProto proto_file;
        file_descriptor->dependency(i)->CopyTo(&proto_file);
        add_protobuf_file(proto_file);
    }
    add_protobuf_file(file_descriptor);
}


const google::protobuf::FileDescriptor* goby::core::DynamicProtobufManager::add_protobuf_file(const google::protobuf::FileDescriptor* file_descriptor)
{
    google::protobuf::FileDescriptorProto proto_file;
    file_descriptor->CopyTo(&proto_file);
    return add_protobuf_file(proto_file);
}
    

const google::protobuf::FileDescriptor* goby::core::DynamicProtobufManager::add_protobuf_file(const google::protobuf::FileDescriptorProto& proto)
{
    return descriptor_pool_.BuildFile(proto); 
}
