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

#include "goby/util/logger.h"
#include "goby/util/time.h"

#include "goby/core/dbo.h"

class GobyDaemon
{
    
  public:
    GobyDaemon();
    ~GobyDaemon();
    
    void run();

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
        void process_subscribe(const goby::core::proto::NotificationToServer& incoming_message);
        
      private:
        bool active_;
        std::string name_;
        boost::posix_time::ptime t_connected_;
        boost::posix_time::ptime t_last_active_;
    };

    // key = protobuf message type name, value = set of subscribed clients
    static std::map<std::string, std::set<boost::shared_ptr<ConnectedClient> > > subscriptions;
    static goby::util::FlexOstream glogger;
    static boost::mutex dbo_mutex;
    static boost::mutex subscription_mutex;
    
  private:
    void connect(const std::string& name);
    void disconnect(const std::string& name);
    
  private:
    bool active_;

    // ServerRequest objects passed through here
    boost::interprocess::message_queue listen_queue_;

    goby::core::DBOManager* dbo_manager_;
    
    // key = client name, value = ConnectedClient deals with this client
    std::map<std::string, boost::shared_ptr<ConnectedClient> > clients_;
    // key = client name, value = thread to run ConnectedClient in 
    std::map<std::string, boost::shared_ptr<boost::thread> > client_threads_;

};


#endif
