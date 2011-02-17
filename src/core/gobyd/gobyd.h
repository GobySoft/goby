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

#include "goby/util/time.h"

#include "goby/core/dbo.h"
#include "goby/core/core.h"
#include "goby/protobuf/config.pb.h"

namespace goby
{
    namespace core
    {
        /// \brief The Goby Publish/Subscribe server or daemon that handles all
        /// application communications on a given platform
        class Daemon
        {
          private:
            // run this the same was as an application derived from goby::core::ApplicationBase
            template<typename App>
                friend int ::goby::run(int argc, char* argv[]);
            
            Daemon();
            ~Daemon();
            Daemon(const Daemon&);
            Daemon& operator= (const Daemon&);
            
            void run();

            void init_cfg();
            void init_logger();
            void init_sql();
            void init_listener();
            
            
            class ConnectedClient;
            class Subscriber;
            // key = protobuf message type name
            // value = set of subscribers to this protobuf type
            static boost::unordered_multimap<std::string, Subscriber> subscriptions;
            static boost::mutex dbo_mutex;
            static boost::shared_mutex subscription_mutex;

            // handles a single subscription from a single connected client (one client may
            // be many subscribers)
            class Subscriber
            {
              public:
              Subscriber(
                  boost::shared_ptr<ConnectedClient> client = boost::shared_ptr<ConnectedClient>(),
                  const proto::Filter& filter = proto::Filter())
                  : client_(client),
                    filter_(filter)
                    { }

                // which client is this subscriber
                const boost::shared_ptr<ConnectedClient> client() const
                { return client_; }
                
                // true if filter permits message
                bool check_filter(const google::protobuf::Message& msg) const
                { return clears_filter(msg, filter_); }

                // allows us to map subscribers
                bool operator<(const Subscriber& rhs) const { return client_ < rhs.client_; }
              
              private:
                boost::shared_ptr<ConnectedClient> client_;
                proto::Filter filter_;
            };


            // Represents a connection from a single application (derived from goby::core::AppliccationBase)
            class ConnectedClient : public boost::enable_shared_from_this<ConnectedClient>
            {
              public:
                ConnectedClient(const std::string& name);
                ~ConnectedClient();

                void run();
                void stop()
                { active_ = false; }
                
                std::string name() const { return name_; }
        
              private:
                void process_notification();
                void process_publish();
                void process_publish_subscribers(
                    boost::unordered_multimap<std::string, Subscriber>::const_iterator begin_it,
                    boost::unordered_multimap<std::string, Subscriber>::const_iterator end_it,
                    const google::protobuf::Message& parsed_embedded_msg);

                void process_subscribe();
                void disconnect_self();
                
                
                
              private:
                enum { MESSAGE_SEND_TRIES = 5 };
                
                
                
                std::string name_;
                bool active_;
                boost::posix_time::ptime t_connected_;
                boost::posix_time::ptime t_last_active_;
                boost::posix_time::ptime t_next_heartbeat_;
                
                boost::unordered_map<boost::shared_ptr<ConnectedClient>,
                    boost::shared_ptr<boost::interprocess::message_queue> > open_queues_;

                // used for serialize / deserialize on the line
                char buffer_ [MAX_MSG_BUFFER_SIZE];
                std::size_t buffer_msg_size_;
                proto::Notification notification_;
            };
      
            void connect();
            void disconnect();

            std::string format_filename(const std::string& in);
            
            
          private:
            
            bool active_;
            
            // ServerRequest objects passed through here
            boost::shared_ptr<boost::interprocess::message_queue> listen_queue_;
            
            DBOManager* dbo_manager_;
            
            // key = client name, value = ConnectedClient deals with this client
            boost::unordered_map<std::string, boost::shared_ptr<ConnectedClient> > clients_;
            // key = client name, value = thread to run ConnectedClient in 
            boost::unordered_map<std::string, boost::shared_ptr<boost::thread> > client_threads_;

            boost::posix_time::ptime t_start_;
            boost::posix_time::ptime t_next_db_commit_;

            const static boost::posix_time::time_duration HEARTBEAT_INTERVAL;
            const static boost::posix_time::time_duration DEAD_INTERVAL;
            const static boost::posix_time::time_duration PUBLISH_WAIT;

            proto::Notification notification_;
            char buffer_ [MAX_MSG_BUFFER_SIZE];
            std::size_t buffer_msg_size_;

            std::ofstream fout_;
            static proto::Config cfg_;

            static int argc_;
            static char** argv_;
        };
    }
}



#endif
