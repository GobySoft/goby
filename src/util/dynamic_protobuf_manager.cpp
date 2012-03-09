
#include "dynamic_protobuf_manager.h"

#include "goby/common/exception.h"

boost::shared_ptr<goby::util::DynamicProtobufManager> goby::util::DynamicProtobufManager::inst_;

boost::signal<void (const google::protobuf::FileDescriptor*)> goby::util::DynamicProtobufManager::new_descriptor_hooks;

const google::protobuf::FileDescriptor* goby::util::DynamicProtobufManager::add_protobuf_file(const google::protobuf::FileDescriptorProto& proto)
{
    simple_database().Add(proto);
    
    const google::protobuf::FileDescriptor* return_desc = descriptor_pool().FindFileByName(proto.name());
    new_descriptor_hooks(return_desc);
    return return_desc; 
}
