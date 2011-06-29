// copyright 2011 t. schneider tes@mit.edu
// 
// this file is part of goby_dbo
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

#ifndef DBOPLUGIN20110608H
#define DBOPLUGIN20110608H

#include <Wt/Dbo/Dbo>
#include <Wt/Dbo/backend/Sqlite3>

#include "goby/core/core_constants.h"
#include <boost/bimap.hpp>
#include <google/protobuf/message.h>
#include <google/protobuf/descriptor.h>
#include "goby/protobuf/dynamic_protobuf_manager.h"

// must be define since we are using the preprocessor
#define GOBY_MAX_PROTOBUF_TYPES @GOBY_DBO_MAX_PROTOBUF_TYPES@

namespace goby
{
    namespace core
    {
        class DBOPlugin
        {
          public:
            typedef goby::core::DBOPlugin* create_t();
            typedef void destroy_t(goby::core::DBOPlugin*);
            
            DBOPlugin() { }
            virtual ~DBOPlugin() { }

            virtual MarshallingScheme provides() = 0;
            virtual void add_message(int unique_id, const std::string& identifier, const void* data, int size) = 0;
            virtual void map_types() = 0;
        };

        class ProtobufDBOPlugin : public DBOPlugin
        {
          public:
            
            ProtobufDBOPlugin();
            
            ~ProtobufDBOPlugin() { }

            // implements DBOPlugin
            MarshallingScheme provides();
            void add_message(int unique_id, const std::string& identifier, const void* data, int size);
            void map_types();

            // extra for direct users
            void add_message(int unique_id, boost::shared_ptr<google::protobuf::Message> msg);

            /// \brief wraps a particular type of Google Protocol Buffers message
            /// (designated by id number i) so that we can use it with Wt::Dbo
            ///
            /// Wt:Dbo requires each type to be "persisted" or stored in the database
            /// to have a compile-time type. Since goby_database does not know about the types
            /// we want to use at compile-time, we have use this placeholder (ProtoBufWrapper)
            /// which creates a new type for each value of i. At runtime we map
            /// the Protocol Buffers types onto a given value of i and use Reflection
            /// to properly store the fields
            template <int i>
                class ProtoBufWrapper
            {
              public:
              ProtoBufWrapper(int unique_id = -1,
                              boost::shared_ptr<google::protobuf::Message> p =
                              boost::shared_ptr<google::protobuf::Message>())
                  : unique_id_(unique_id),
                    p_(p)
                {
                    // create new blank message if none given
                    if(!p)
                    {
                        p_ = goby::protobuf::DynamicProtobufManager::new_protobuf_message(dbo_map.left.at(i));
                    }
                }
                
                google::protobuf::Message& msg() { return *p_; }         
                int& unique_id() { return unique_id_; }                
                
              private:
                int unique_id_;
                boost::shared_ptr<google::protobuf::Message> p_;
            };
            
          private:
            //void add_file_descriptor(const google::protobuf::FileDescriptor* file_descriptor);
            void add_type(const google::protobuf::Descriptor* descriptor);
 
            /// \brief add a message to the Wt::Dbo SQL database
            ///
            /// This is not written to the database until commit() is called
            //void add_message(int unique_id, const google::protobuf::Message& msg);


            void map_type(const google::protobuf::Descriptor* descriptor);

          private:
            
            // used to map runtime provided type (name, std::string) onto the compile time
            // templated integers (incrementing int) for Wt::Dbo    
            // key = integer (order type was declared)
            // value = string full name of protobuf type
            static boost::bimap<int, std::string> dbo_map;
            // key = integer (order type was declared)
            // value = string name for database table
            std::map<int, std::string> mangle_map_;
            // next integer to use for new incoming type
            int index_;
        };
        


        
        class DBOPluginFactory
        {
          public:

            DBOPluginFactory()
            { }
            ~ DBOPluginFactory()
            {
                
            }
            
            void add_library(const std::string& lib_name);
            void add_plugin(DBOPlugin* plugin);
            
            std::set<DBOPlugin*> plugins();
            
            DBOPlugin* plugin(goby::core::MarshallingScheme scheme);
          private:
            class PluginLibrary
            {
              public:
                PluginLibrary(const std::string& lib_name);
              PluginLibrary(DBOPlugin* plugin)
                  : handle_(0), plugin_(plugin), destroy_(0), create_(0)
                { }
                ~PluginLibrary();
                
                DBOPlugin* plugin() 
                { return plugin_; }
                
              private:
                
                PluginLibrary(const PluginLibrary&);
                PluginLibrary& operator=(const PluginLibrary&);
                
                void* handle_;
                DBOPlugin* plugin_;
                goby::core::DBOPlugin::destroy_t* destroy_;
                goby::core::DBOPlugin::create_t* create_;
            };
            
            std::map<goby::core::MarshallingScheme, boost::shared_ptr<PluginLibrary> > plugins_;
        };
        
        

    }
}

// provide a special overload of `persist` for the ProtoBufWrapper class so that
// Wt::Dbo knows how to handle ProtoBufWrapper types (and then through that class, google::protobuf::Message types)
namespace Wt
{
    namespace Dbo
    {
        template <>
            template <int i>
            struct persist<goby::core::ProtobufDBOPlugin::ProtoBufWrapper<i> >
        {
            template<typename A>
                static void apply(goby::core::ProtobufDBOPlugin::ProtoBufWrapper<i>& wrapper, A& action)
            {
                Wt::Dbo::field(action, wrapper.unique_id(), "raw_id");
                protobuf_message_persist(wrapper.msg(), action);
            }
        };


    }
}


#endif
