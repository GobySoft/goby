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

// terminal IO
extern goby::util::FlexOstream logger_;

class GobyDaemon
{
    
  public:
    GobyDaemon();
    ~GobyDaemon();
    
    void run();

    class ConnectedClient
    {
      public:
        ConnectedClient(const std::string& name);
        ~ConnectedClient();

        void run();
        void stop() { active_ = false; }
        
        
      private:
                
      private:
        bool active_;
        std::string name_;
        boost::posix_time::ptime t_connected_;
    };

    
  private:
    void do_connect(const std::string& name);
    void do_disconnect(const std::string& name);
    
  private:
    bool active_;

    // ServerRequest objects passed through here
    boost::interprocess::message_queue listen_queue_;

    boost::mutex mutex_;

    std::map<std::string, boost::shared_ptr<ConnectedClient> > clients_;
    std::map<std::string, boost::shared_ptr<boost::thread> > client_threads_;
    

};





#endif
