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
#include "goby/core/libcore/dynamic_protobuf_manager.h"

#include "goby/core/core_constants.h"

#include <Wt/Dbo/Dbo>

#include "goby/util/time.h"

#ifdef HAS_MOOS
#include "goby/moos/libmoos_util/moos_serializer.h"
#endif

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
        struct RawEntry
        {
            MarshallingScheme marshalling_scheme;
            std::string identifier;
            std::vector<unsigned char> bytes;
            std::string time;
            int unique_id;
            
            template<class Action>
            void persist(Action& a)
                {
                    Wt::Dbo::field(a, unique_id, "raw_id");
                    Wt::Dbo::field(a, time, "time_written");
                    Wt::Dbo::field(a, marshalling_scheme, "marshalling_scheme");
                    Wt::Dbo::field(a, identifier, "identifier");
                    Wt::Dbo::field(a, bytes, "bytes");
                }
        };
        

        /// \brief provides a way for Wt::Dbo to work with arbitrary
        /// (run-time provided) Google Protocol Buffers types
        class DBOManager
        {
          public:
            /// \brief singleton class: use this to get a pointer
            static DBOManager* get_instance();
            /// \brief if desired, this will release all resources (use right before exiting)
            static void shutdown();

            
            /// \brief add a type (given by its descriptor) to the Wt::Dbo SQL database
            ///
            /// you must have already added the .proto file in which this type resides using add_file()
            void add_type(const google::protobuf::Descriptor* descriptor);
 
            /// \brief add a message to the Wt::Dbo SQL database
            ///
            /// This is not written to the database until commit() is called
            void add_message(int unique_id, boost::shared_ptr<google::protobuf::Message> msg);
            void add_message(int unique_id, const google::protobuf::Message& msg);

#ifdef HAS_MOOS
            void add_message(int unique_id, CMOOSMsg& msg);
#endif
            
            void add_raw(MarshallingScheme marshalling_scheme,
                         const std::string& identifier,
                         const void* data,
                         int size,
                         int socket_id);
            
            
            /// \brief commit all changes to the Wt::Dbo SQL database
            void commit();

            /// \brief connect to the Wt::Dbo SQL database
            void connect(const std::string& db_name = "");            
            
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
                        p_ = DynamicProtobufManager::new_protobuf_message(dbo_map.left.at(i));
                    }
                }
                
                google::protobuf::Message& msg(){ return *p_; }                
                int& unique_id() { return unique_id_; }
                
                
              private:
                int unique_id_;
                boost::shared_ptr<google::protobuf::Message> p_;
            };
            
          private:
            void map_type(const google::protobuf::Descriptor* descriptor);
            void reset_session();
            void map_static_types();
            
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


            bool static_tables_created_;
            
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
            {
                Wt::Dbo::field(action, wrapper.unique_id(), "raw_id");
                protobuf_message_persist(wrapper.msg(), action);
            }
        };


    }
}



#endif // DBOMANAGER20100901H
