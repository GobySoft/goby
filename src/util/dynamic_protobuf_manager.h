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

#include <boost/signals.hpp>
#include <boost/shared_ptr.hpp>

namespace goby
{
    namespace util
    {
        class DynamicProtobufManager
        {
          public:
            template<typename GoogleProtobufMessagePointer>
                static GoogleProtobufMessagePointer new_protobuf_message(
                    const std::string& protobuf_type_name)
            {
                // try the compiled pool
                const google::protobuf::Descriptor* desc = google::protobuf::DescriptorPool::generated_pool()->FindMessageTypeByName(protobuf_type_name);
                
                if(desc) return new_protobuf_message<GoogleProtobufMessagePointer>(desc);
                
                // try the user pool
                desc = descriptor_pool().FindMessageTypeByName(protobuf_type_name);
                
                if(desc) return new_protobuf_message<GoogleProtobufMessagePointer>(desc);
                
                throw(std::runtime_error("Unknown type " + protobuf_type_name + ", be sure it is loaded at compile-time, via dlopen, or with a call to add_protobuf_file()"));
            }
            
            template<typename GoogleProtobufMessagePointer>
                static GoogleProtobufMessagePointer new_protobuf_message( 
                    const google::protobuf::Descriptor* desc)
            { return GoogleProtobufMessagePointer(msg_factory().GetPrototype(desc)->New()); }
            
            static boost::shared_ptr<google::protobuf::Message> new_protobuf_message(
                const google::protobuf::Descriptor* desc)
            { return new_protobuf_message<boost::shared_ptr<google::protobuf::Message> >(desc); }
            
            static boost::shared_ptr<google::protobuf::Message> new_protobuf_message(
                const std::string& protobuf_type_name)
            { return new_protobuf_message<boost::shared_ptr<google::protobuf::Message> >(protobuf_type_name); }

            static std::set<const google::protobuf::FileDescriptor*>
                add_protobuf_file_with_dependencies(
                const google::protobuf::FileDescriptor* file_descriptor);
            static const google::protobuf::FileDescriptor* add_protobuf_file(
                const google::protobuf::FileDescriptor* file_descriptor);
            static const google::protobuf::FileDescriptor* add_protobuf_file(
                const google::protobuf::FileDescriptorProto& proto);
            
            static google::protobuf::DynamicMessageFactory& msg_factory()
            {
                return *get_instance()->msg_factory_;
            }
            static google::protobuf::DescriptorPool& descriptor_pool()
            {
                return *get_instance()->descriptor_pool_;
            }

            static boost::signal<void (const google::protobuf::FileDescriptor*) > new_descriptor_hooks;
            
          private:
            // so we can use shared_ptr to hold the singleton
            template<typename T>
                friend void boost::checked_delete(T*);
            static boost::shared_ptr<DynamicProtobufManager> inst_;

            static DynamicProtobufManager* get_instance()
            {
                if(!inst_)
                    inst_.reset(new DynamicProtobufManager);
                return inst_.get();
            }
            
            DynamicProtobufManager()
            {
                msg_factory_ = new google::protobuf::DynamicMessageFactory();
                descriptor_pool_ = new google::protobuf::DescriptorPool();
            }
            ~DynamicProtobufManager()
            {
                delete msg_factory_;
                delete descriptor_pool_;
                google::protobuf::ShutdownProtobufLibrary();
            }

            DynamicProtobufManager(const DynamicProtobufManager&);
            DynamicProtobufManager& operator= (const DynamicProtobufManager&);
            
          private:
            
            // see google::protobuf documentation: this assists in
            // creating messages at runtime
            google::protobuf::DynamicMessageFactory* msg_factory_;
            // see google::protobuf documentation: this assists in
            // creating messages at runtime
            google::protobuf::DescriptorPool* descriptor_pool_;
            
        };
        
    }
}

#endif
