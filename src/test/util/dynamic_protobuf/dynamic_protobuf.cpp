#include "goby/util/dynamic_protobuf_manager.h"
#include <cassert>
#include <iostream>
#include <dlfcn.h>
#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/text_format.h>

#include "test_a.pb.h"
#include "test_b.pb.h"

using namespace goby::util;


int main()
{
    void* dl_handle = dlopen("libtest_dyn_protobuf" SHARED_LIBRARY_SUFFIX, RTLD_LAZY);

    if(!dl_handle)
    {
        std::cerr << "Failed to open libtest_dyn_protobuf" << std::endl;
        exit(1);
    }
    
    google::protobuf::SimpleDescriptorDatabase* simple_database = new google::protobuf::SimpleDescriptorDatabase;
    goby::util::DynamicProtobufManager::add_database(simple_database);
    
    {
        // testing compiled in
        boost::shared_ptr<google::protobuf::Message> adyn_msg = goby::util::DynamicProtobufManager::new_protobuf_message("A");
    
        std::cout << adyn_msg->GetDescriptor()->DebugString() << std::endl;
    
        // testing dlopen'd
        boost::shared_ptr<google::protobuf::Message> bdyn_msg = goby::util::DynamicProtobufManager::new_protobuf_message("B");
    
        std::cout << bdyn_msg->GetDescriptor()->DebugString() << std::endl;

        // test non-existent
        try
        {
            boost::shared_ptr<google::protobuf::Message> cdyn_msg = goby::util::DynamicProtobufManager::new_protobuf_message("C");
            // should throw
            assert(false);
        }
        catch(std::exception& e)
        {
            // expected
        }

        // test dynamically loaded
        google::protobuf::FileDescriptorProto d_proto;
        std::string d_proto_str = "name: \"goby/test/util/dynamic_protobuf/test_d.proto\" message_type {   name: \"D\"   field {     name: \"d1\"     number: 1     label: LABEL_REQUIRED     type: TYPE_DOUBLE  } } ";
    
        google::protobuf::TextFormat::ParseFromString(d_proto_str, &d_proto);
        goby::util::DynamicProtobufManager::add_protobuf_file(d_proto);
    
        boost::shared_ptr<google::protobuf::Message> ddyn_msg = goby::util::DynamicProtobufManager::new_protobuf_message("D");
    
        std::cout << ddyn_msg->GetDescriptor()->DebugString() << std::endl;


        // test dynamically via separate database
        google::protobuf::FileDescriptorProto e_proto;
        std::string e_proto_str = "name: \"goby/test/util/dynamic_protobuf/test_e.proto\" message_type {   name: \"E\"   field {     name: \"e1\"     number: 1     label: LABEL_REQUIRED     type: TYPE_DOUBLE  } } ";
    
        google::protobuf::TextFormat::ParseFromString(e_proto_str, &e_proto);

        simple_database->Add(e_proto);
        
        boost::shared_ptr<google::protobuf::Message> edyn_msg = goby::util::DynamicProtobufManager::new_protobuf_message("E");
        std::cout << edyn_msg->GetDescriptor()->DebugString() << std::endl;
        
    
        std::cout << "all tests passed" << std::endl;
    }
    delete simple_database;

    goby::util::DynamicProtobufManager::protobuf_shutdown();

    dlclose(dl_handle);
    return 0;
}
