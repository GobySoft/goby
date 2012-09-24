// Copyright 2009-2012 Toby Schneider (https://launchpad.net/~tes)
//                     Massachusetts Institute of Technology (2007-)
//                     Woods Hole Oceanographic Institution (2007-)
//                     Goby Developers Team (https://launchpad.net/~goby-dev)
// 
//
// This file is part of the Goby Underwater Autonomy Project Libraries
// ("The Goby Libraries").
//
// The Goby Libraries are free software: you can redistribute them and/or modify
// them under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// The Goby Libraries are distributed in the hope that they will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with Goby.  If not, see <http://www.gnu.org/licenses/>.


#ifndef DYNAMICPROTOBUFMANAGER20110419H
#define DYNAMICPROTOBUFMANAGER20110419H

#define PROTO_RUNTIME_COMPILE @IS_PROTO_RUNTIME_COMPILE@

#include <dlfcn.h>

#include <set>

#include <google/protobuf/descriptor.h>
#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/dynamic_message.h>
#include <google/protobuf/descriptor_database.h>
#include <google/protobuf/compiler/importer.h>


#include <boost/signals2.hpp>
#include <boost/shared_ptr.hpp>

namespace goby
{
    namespace util
    {
        class DynamicProtobufManager
        {
          public:
            static const google::protobuf::Descriptor* find_descriptor(const std::string& protobuf_type_name)
            {
                // try the generated pool
                const google::protobuf::Descriptor* desc = google::protobuf::DescriptorPool::generated_pool()->FindMessageTypeByName(protobuf_type_name);
                if(desc) return desc;
                
                // try the user pool
                desc = user_descriptor_pool().FindMessageTypeByName(protobuf_type_name);
                return desc;
            }
            
            template<typename GoogleProtobufMessagePointer>
                static GoogleProtobufMessagePointer new_protobuf_message(
                    const std::string& protobuf_type_name)
            {
                const google::protobuf::Descriptor* desc = find_descriptor(protobuf_type_name);
                if(desc)
                    return new_protobuf_message<GoogleProtobufMessagePointer>(desc);
                else
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
            
            
            static void add_database(google::protobuf::DescriptorDatabase* database)
            {
                get_instance()->databases_.push_back(database);
                get_instance()->update_databases();
            }

            static void enable_compilation()
            {
                get_instance()->enable_disk_source_database();
            }

#if PROTO_RUNTIME_COMPILE
            static const google::protobuf::FileDescriptor*
                load_from_proto_file(const std::string& proto_file);
#endif
            
            static void* load_from_shared_lib(const std::string& shared_lib_path)
            {
                void* handle = dlopen(shared_lib_path.c_str(), RTLD_LAZY);
                if(handle)
                    get_instance()->dl_handles_.push_back(handle);
                return handle;
            }
            
            static void protobuf_shutdown()
            {
                get_instance()->shutdown();
            }
            
            
            static const google::protobuf::FileDescriptor* add_protobuf_file(
                const google::protobuf::FileDescriptorProto& proto);
            
            static boost::signals2::signal<void (const google::protobuf::FileDescriptor*) > new_descriptor_hooks;

            static google::protobuf::DynamicMessageFactory& msg_factory()
            { return *get_instance()->msg_factory_; }
            static google::protobuf::DescriptorPool& user_descriptor_pool()
            { return *get_instance()->user_descriptor_pool_; }
            static google::protobuf::SimpleDescriptorDatabase& simple_database()
            { return *get_instance()->simple_database_; }
            
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
              : generated_database_(new google::protobuf::DescriptorPoolDatabase(*google::protobuf::DescriptorPool::generated_pool())),
                simple_database_(new google::protobuf::SimpleDescriptorDatabase),
                msg_factory_(new google::protobuf::DynamicMessageFactory),
                disk_source_tree_(0),
                source_database_(0),
                error_collector_(0)                
            {
                databases_.push_back(simple_database_); 
                databases_.push_back(generated_database_);

                merged_database_ = new google::protobuf::MergedDescriptorDatabase(databases_);
                user_descriptor_pool_ = new google::protobuf::DescriptorPool(merged_database_);
            }
            
            ~DynamicProtobufManager()
            {
            }

            void shutdown()
            {

                delete msg_factory_;
                delete user_descriptor_pool_;
                delete merged_database_;
                delete simple_database_;
                delete generated_database_;

                if(disk_source_tree_)
                    delete disk_source_tree_;
                if(source_database_)
                    delete source_database_;
                if(error_collector_)
                    delete error_collector_;
                
                google::protobuf::ShutdownProtobufLibrary();

                for(std::vector<void *>::iterator it = dl_handles_.begin(),
                        n = dl_handles_.end(); it != n; ++it)
                    dlclose(*it);
            }
            
            
            void update_databases()
            {
                delete user_descriptor_pool_;
                delete merged_database_;
                
                merged_database_ = new google::protobuf::MergedDescriptorDatabase(databases_);
                user_descriptor_pool_ = new google::protobuf::DescriptorPool(merged_database_);
            }

            void enable_disk_source_database();
            
            DynamicProtobufManager(const DynamicProtobufManager&);
            DynamicProtobufManager& operator= (const DynamicProtobufManager&);
            
          private:
            std::vector<google::protobuf::DescriptorDatabase *> databases_;

            // always used
            google::protobuf::DescriptorPoolDatabase* generated_database_;
            google::protobuf::SimpleDescriptorDatabase* simple_database_;
            google::protobuf::MergedDescriptorDatabase* merged_database_;
            google::protobuf::DescriptorPool* user_descriptor_pool_;
            google::protobuf::DynamicMessageFactory* msg_factory_;

            // sometimes used
            google::protobuf::compiler::DiskSourceTree* disk_source_tree_;
            google::protobuf::compiler::SourceTreeDescriptorDatabase* source_database_;

            class GLogMultiFileErrorCollector
                : public google::protobuf::compiler::MultiFileErrorCollector
            {
                void AddError(const std::string & filename, int line, int column,
                              const std::string & message);
            };
            
            GLogMultiFileErrorCollector* error_collector_;

            std::vector<void *> dl_handles_;
        };
        
    }
}

#endif
