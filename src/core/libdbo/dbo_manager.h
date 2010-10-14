// copyright 2010 t. schneider tes@mit.edu
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

#ifndef DBOMANAGER20100901H
#define DBOMANAGER20100901H

#include <stdexcept>

#include <boost/bimap.hpp>
#include <boost/shared_ptr.hpp>

#include <google/protobuf/message.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/dynamic_message.h>

#include <Wt/Dbo/Dbo>

#include "goby/util/time.h"

namespace Wt
{
    namespace Dbo
    {
        namespace backend
        {
            class Sqlite3;
        }
        
        class Session;
        class Transaction;
    }
}

namespace goby
{
    namespace core
    {        
        /// \brief provides a way for Wt::Dbo to work with arbitrary
        /// (run-time provided) Google Protocol Buffers types
        class DBOManager
        {
          public:
            /// \brief singleton class: use this to get a pointer
            static DBOManager* get_instance();
            /// \brief if desired, this will release all resources (use right before exiting)
            static void shutdown();

            /// \brief add the entire .proto file in which descriptor is defined
            ///
            /// this does not add all the types contained within this file. You must add each type you want persisted by calling add_type() for each
            /// \param descriptor Descriptor (meta-data) of the message whose containing file you wish to add
            void add_file(const google::protobuf::Descriptor* descriptor);
            /// \brief add the entire .proto file given by this FileDescriptorProto
            ///
            /// this does not add all the types contained within this file. You must add each type you want persisted by calling add_type() for each
            /// \param proto FileDescriptorProto representation of a .proto file. This object can be transmitted on the wire like any other google::protobuf::Message
            void add_file(const google::protobuf::FileDescriptorProto& proto);

            /// \brief add a type (given by its descriptor) to the Wt::Dbo SQL database
            ///
            /// you must have already added the .proto file in which this type resides using add_file()
            void add_type(const google::protobuf::Descriptor* descriptor);
            /// \brief add a type (given by its full name as defined by Descriptor::full_name()) to the Wt::Dbo SQL database 
            ///
            /// you must have already added the .proto file in which this type resides using add_file()
            void add_type(const std::string& name);
            
//            void add_message(const std::string& name, const std::string& serialized_message);
            /// \brief add a message to the Wt::Dbo SQL database
            ///
            /// This is not written to the database until commit() is called
            void add_message(boost::shared_ptr<google::protobuf::Message> msg);

            /// \brief commit all changes to the Wt::Dbo SQL database
            void commit();
            
            /// \brief create a blank message of the type given by its name (as defined by Descriptor::full_name())
            static boost::shared_ptr<google::protobuf::Message>
                new_msg_from_name(const std::string& name);            

            /// \brief connect to the Wt::Dbo SQL database
            void connect(const std::string& db_name = "");            
            
            /// \brief wraps a particular type of Google Protocol Buffers message
            /// (designated by id number i) so that we can use it with Wt::Dbo
            ///
            /// Wt:Dbo requires each type to be "persisted" or stored in the database
            /// to have a compile-time type. Since gobyd does not know about the types
            /// we want to use at compile-time, we have use this placeholder (ProtoBufWrapper)
            /// which creates a new type for each value of i. At runtime we map
            /// the Protocol Buffers types onto a given value of i and use Reflection
            /// to properly store the fields
            template <int i>
                class ProtoBufWrapper
            {
              public:
              ProtoBufWrapper(boost::shared_ptr<google::protobuf::Message> p =
                              boost::shared_ptr<google::protobuf::Message>())
                  : p_(p)
                {
                    // create new blank message if none given
                    if(!p)
                    {
                        p_.reset(msg_factory.GetPrototype
                                 (descriptor_pool.FindMessageTypeByName
                                  (dbo_map.left.at(i)))->New());
                    }
                }
                
                google::protobuf::Message& msg(){ return *p_; }                
                
              private:
                boost::shared_ptr<google::protobuf::Message> p_;
            };

            // see google::protobuf documentation: this assists in
            // creating messages at runtime
            static google::protobuf::DynamicMessageFactory msg_factory;
            // see google::protobuf documentation: this assists in
            // creating messages at runtime
            static google::protobuf::DescriptorPool descriptor_pool;

          private:
            void map_type(const google::protobuf::Descriptor* descriptor);
            void reset_session();

          private:    
            static DBOManager* inst_;
            DBOManager();
            
            ~DBOManager();
            
            DBOManager(const DBOManager&);
            DBOManager& operator = (const DBOManager&);

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

            std::string db_name_;

            // Wt Dbo has its own brand of memory management
            Wt::Dbo::backend::Sqlite3* connection_;
            Wt::Dbo::Session* session_;
            Wt::Dbo::Transaction* transaction_;

            boost::posix_time::ptime t_last_commit_;
            
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
            struct persist<goby::core::DBOManager::ProtoBufWrapper<i> >
        {
            template<typename A>
                static void apply(goby::core::DBOManager::ProtoBufWrapper<i>& wrapper, A& action)
            { protobuf_message_persist(wrapper.msg(), action); }
        };
    }
}


#endif
