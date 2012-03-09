
#ifndef DYNAMICPROTOBUFMANAGER20110419H
#define DYNAMICPROTOBUFMANAGER20110419H

#include <set>

#include <google/protobuf/descriptor.h>
#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/dynamic_message.h>
#include <google/protobuf/descriptor_database.h>

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
                // try the user pool
                 const google::protobuf::Descriptor* desc = descriptor_pool().FindMessageTypeByName(protobuf_type_name);
                
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
            
            
            static void add_database(google::protobuf::DescriptorDatabase* database)
            {
                get_instance()->databases_.push_back(database);
                get_instance()->update_databases();
            }
            
            static void protobuf_shutdown()
            {
                get_instance()->shutdown();
            }
            
            
            static const google::protobuf::FileDescriptor* add_protobuf_file(
                const google::protobuf::FileDescriptorProto& proto);

            static boost::signal<void (const google::protobuf::FileDescriptor*) > new_descriptor_hooks;

            static google::protobuf::DynamicMessageFactory& msg_factory()
            { return *get_instance()->msg_factory_; }
            static google::protobuf::DescriptorPool& descriptor_pool()
            { return *get_instance()->descriptor_pool_; }
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
              : msg_factory_(new google::protobuf::DynamicMessageFactory),
                generated_database_(new google::protobuf::DescriptorPoolDatabase(*google::protobuf::DescriptorPool::generated_pool())),
                simple_database_(new google::protobuf::SimpleDescriptorDatabase)
            {
                databases_.push_back(simple_database_); 
                databases_.push_back(generated_database_);

                merged_database_ = new google::protobuf::MergedDescriptorDatabase(databases_);
                descriptor_pool_ = new google::protobuf::DescriptorPool(merged_database_);
            }
            
            ~DynamicProtobufManager()
            {
            }

            void shutdown()
            {

                delete msg_factory_;
                delete descriptor_pool_;
                delete merged_database_;
                delete simple_database_;
                delete generated_database_;

                google::protobuf::ShutdownProtobufLibrary();
            }
            
            
            void update_databases()
            {
                delete descriptor_pool_;
                delete merged_database_;
                
                merged_database_ = new google::protobuf::MergedDescriptorDatabase(databases_);
                descriptor_pool_ = new google::protobuf::DescriptorPool(merged_database_);
            }
            
            
            DynamicProtobufManager(const DynamicProtobufManager&);
            DynamicProtobufManager& operator= (const DynamicProtobufManager&);
            
          private:
            std::vector<google::protobuf::DescriptorDatabase *> databases_;
            
            google::protobuf::DynamicMessageFactory* msg_factory_;
            google::protobuf::DescriptorPoolDatabase* generated_database_;
            google::protobuf::SimpleDescriptorDatabase* simple_database_;
            
            google::protobuf::MergedDescriptorDatabase* merged_database_;
            google::protobuf::DescriptorPool* descriptor_pool_;
            
        };
        
    }
}

#endif
