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

#include <Wt/Dbo/Dbo>
#include <Wt/Dbo/backend/Sqlite3>

#include <google/protobuf/message.h>

namespace goby 
{
    namespace core
    {    

        // provides a way for Wt::Dbo to work with arbitrary
        // (run-time provided) Google Protocol Buffers types
        class DBOManager
        {
          public:
            static DBOManager* get_instance();

            void add_file(const google::protobuf::FileDescriptorProto& proto);
            void add_type(const google::protobuf::Descriptor* descriptor);
            void add_message(google::protobuf::Message* msg);

            google::protobuf::Message* new_msg_from_name(const std::string& name)
            {
                return msg_factory.GetPrototype(descriptor_pool.FindMessageTypeByName(name))->New();
            }
            
            void connect(const std::string& db_name)
            {
                connection_ = new Wt::Dbo::backend::Sqlite3(db_name);
                session_.setConnection(*connection_);
            }

            // wraps a particular type of Protobuf message (designated by id number i)
            // so that we can use it with Wt::Dbo
            template <int i>
                class ProtoBufWrapper
            {
              public:
              ProtoBufWrapper(google::protobuf::Message* p = 0)
                  : p_(p)
                {
                    // create new blank message if none given
                    // either way, we own the message
                    if(!p) p_ = msg_factory.GetPrototype(descriptor_pool.FindMessageTypeByName(dbo_map.left.at(i)))->New();
                }

                ~ProtoBufWrapper()
                {
                    // we own the message, delete it
                    if(p_) delete p_;
                }
    
                google::protobuf::Message& msg() const { return *p_; }
    
              private:
                google::protobuf::Message* p_;
            };
    

            static google::protobuf::DynamicMessageFactory msg_factory;
            static google::protobuf::DescriptorPool descriptor_pool;

          private:    
            static DBOManager* inst_;
          DBOManager()
              : connection_(0)
            { }
            ~DBOManager()
            {
                if(connection_) delete connection_;
            }
            DBOManager(const DBOManager&);
            DBOManager& operator = (const DBOManager&);

            // used to map runtime provided type (name, std::string) onto the compile time
            // templated integers (incrementing int) for Wt::Dbo    
            static boost::bimap<int, std::string> dbo_map;
            // next integer to use for new incoming type
            static int index;

            Wt::Dbo::Session session_;
            Wt::Dbo::backend::Sqlite3* connection_;    
    
        };
    }
}

// provide a special overload of `persist` for the ProtoBufWrapper class
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
