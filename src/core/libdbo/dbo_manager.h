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

#include <Wt/Dbo/Dbo>
#include <Wt/Dbo/backend/Sqlite3>

#include <google/protobuf/message.h>

#include "goby/util/logger.h"


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
            static void shutdown();

            void add_file(const google::protobuf::Descriptor* descriptor);
            void add_file(const google::protobuf::FileDescriptorProto& proto);

            void add_type(const google::protobuf::Descriptor* descriptor);
            void add_type(const std::string& name);
//            void add_message(const std::string& name, const std::string& serialized_message);
            void add_message(boost::shared_ptr<google::protobuf::Message> msg);

            void commit();
            
            
            /// Registers the group names used for the FlexOstream logger
            void add_flex_groups(util::FlexOstream& tout);
            
            void set_logger(std::ostream* log) { log_ = log; }            
            
            static boost::shared_ptr<google::protobuf::Message> new_msg_from_name(const std::string& name)
            {
                return boost::shared_ptr<google::protobuf::Message>(
                    msg_factory.GetPrototype(
                        descriptor_pool.FindMessageTypeByName(name))->New());
            }
            
            void connect(const std::string& db_name = "")
            {
                if(!db_name.empty())
                    db_name_ = db_name;

                if(transaction_) delete transaction_;
                if(connection_) delete connection_;
                if(session_) delete session_;
                connection_ = new Wt::Dbo::backend::Sqlite3(db_name_);
                session_ = new Wt::Dbo::Session;
                session_->setConnection(*connection_);
                // transaction deleted by session
                transaction_ = new Wt::Dbo::Transaction(*session_);
            }

            void reset_session()
            {
                
                commit();
                if(log_) *log_ << "resetting session" << std::endl;
                connect();
            }
            
            
            // wraps a particular type of Protobuf message (designated by id number i)
            // so that we can use it with Wt::Dbo
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

            static google::protobuf::DynamicMessageFactory msg_factory;
            static google::protobuf::DescriptorPool descriptor_pool;

          private:
            void map_type(const google::protobuf::Descriptor* descriptor);

          private:    
            static DBOManager* inst_;
            DBOManager();
            
            ~DBOManager()
            {
                if(transaction_) delete transaction_;
                if(connection_) delete connection_;
                if(session_) delete session_;
            }
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

            std::ostream* log_;

            std::string db_name_;

            // Wt Dbo has its own brand of memory management
            Wt::Dbo::backend::Sqlite3* connection_;
            Wt::Dbo::Session* session_;
            Wt::Dbo::Transaction* transaction_;

            boost::posix_time::ptime t_last_commit_;
            
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
