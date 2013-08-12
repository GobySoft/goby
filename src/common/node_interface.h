// Copyright 2009-2013 Toby Schneider (https://launchpad.net/~tes)
//                     Massachusetts Institute of Technology (2007-)
//                     Woods Hole Oceanographic Institution (2007-)
//                     Goby Developers Team (https://launchpad.net/~goby-dev)
// 
//
// This file is part of the Goby Underwater Autonomy Project Libraries
// ("The Goby Libraries").
//
// The Goby Libraries are free software: you can redistribute them and/or modify
// them under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// The Goby Libraries are distributed in the hope that they will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with Goby.  If not, see <http://www.gnu.org/licenses/>.


#ifndef PROTOBUFNODE20110607H
#define PROTOBUFNODE20110607H

#include "zeromq_service.h"

namespace goby
{
    namespace common
    {
        template<typename NodeTypeBase>
            class NodeInterface
        {
          public:
            ZeroMQService* zeromq_service() { return zeromq_service_; }
            virtual void send(const NodeTypeBase& msg, int socket_id, const std::string& group = "") = 0;
            virtual void subscribe(const std::string& identifier, int socket_id) = 0;

          protected:
          NodeInterface(ZeroMQService* service)
              : zeromq_service_(service)
            {
                zeromq_service_->connect_inbox_slot(&NodeInterface<NodeTypeBase>::inbox, this);
            }            
            
            virtual void inbox(MarshallingScheme marshalling_scheme,
                               const std::string& identifier,
                               const void* data,
                               int size,
                               int socket_id) = 0;
          private:
            ZeroMQService* zeromq_service_;
            
        };
    }
}


#endif
