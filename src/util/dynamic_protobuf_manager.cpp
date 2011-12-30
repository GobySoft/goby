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

#include "goby/util/exception.h"

boost::shared_ptr<goby::util::DynamicProtobufManager> goby::util::DynamicProtobufManager::inst_;

boost::signal<void (const google::protobuf::FileDescriptor*)> goby::util::DynamicProtobufManager::new_descriptor_hooks;

const google::protobuf::FileDescriptor* goby::util::DynamicProtobufManager::add_protobuf_file(const google::protobuf::FileDescriptorProto& proto)
{
    simple_database().Add(proto);
    
    const google::protobuf::FileDescriptor* return_desc = descriptor_pool().FindFileByName(proto.name());
    new_descriptor_hooks(return_desc);
    return return_desc; 
}
