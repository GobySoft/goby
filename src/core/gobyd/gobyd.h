// copyright 2010 t. schneider tes@mit.edu
// 
// the file is the goby daemon, part of the core goby autonomy system
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

#ifndef GOBYD20100914H
#define GOBYD20100914H

#include <map>
#include <boost/unordered_map.hpp>

#include "goby/util/logger.h"
#include "goby/util/time.h"

#include "goby/core/dbo.h"

namespace goby
{
    namespace core
    {
        class Daemon
        {
          public:
            Daemon();
            ~Daemon();
    
            void run();

            class ConnectedClient;
            
            //                    |
            //                  / | \
            //                  Types
            //                 /  |  \
            //                /|  |\  \
            //                  Names
            //               / |  | \  \
            //              /| |\ |\ |\ \
            //               Subscribers
            // set of subscribers
            typedef std::set<boost::shared_ptr<ConnectedClient> >
                Subscribers;
            // key = goby variable name
            typedef boost::unordered_map<std::string, Subscribers>
                NameSubscribersMap;
            // key = protobuf message type name
            typedef boost::unordered_map<std::string, NameSubscribersMap>
                TypeNameSubscribersMap;
            static TypeNameSubscribersMap subscriptions;
            
            class ConnectedClient : public boost::enable_shared_from_this<ConnectedClient>
            {
              public:
                ConnectedClient(const std::string& name);
                ~ConnectedClient();

                void run();
                void stop() { active_ = false; }

                std::string name() const { return name_; }
        
              private:
                void process_notification(const goby::core::proto::NotificationToServer& notification);
                void process_publish(const goby::core::proto::NotificationToServer& incoming_message);
                void process_publish_name(const goby::core::proto::NotificationToServer& incoming_message, const NameSubscribersMap& name_map);
                void process_publish_subscribers(const goby::core::proto::NotificationToServer& incoming_message, const Subscribers& subscribers);

                void process_subscribe(const goby::core::proto::NotificationToServer& incoming_message);
        
              private:
                bool active_;
                std::string name_;
                boost::posix_time::ptime t_connected_;
                boost::posix_time::ptime t_last_active_;
                boost::posix_time::ptime t_next_heartbeat_;
                
                boost::unordered_map<boost::shared_ptr<ConnectedClient>, boost::shared_ptr<boost::interprocess::message_queue> > open_queues_;
        
            };


            
            static boost::mutex dbo_mutex;
            static boost::mutex subscription_mutex;
            static boost::mutex logger_mutex;
            static goby::util::FlexOstream glogger;
            
          private:
            void connect(const std::string& name);
            void disconnect(const std::string& name);
            
            
          private:
            
            bool active_;

            // ServerRequest objects passed through here
            boost::shared_ptr<boost::interprocess::message_queue> listen_queue_;

            goby::core::DBOManager* dbo_manager_;
    
            // key = client name, value = ConnectedClient deals with this client
            boost::unordered_map<std::string, boost::shared_ptr<ConnectedClient> > clients_;
            // key = client name, value = thread to run ConnectedClient in 
            boost::unordered_map<std::string, boost::shared_ptr<boost::thread> > client_threads_;

            boost::posix_time::ptime t_start_;
            boost::posix_time::ptime t_next_db_commit_;

            static boost::posix_time::time_duration HEARTBEAT_INTERVAL;
            static boost::posix_time::time_duration DEAD_INTERVAL;

        };
    }
}


#endif
