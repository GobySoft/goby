// copyright 2010-2011 t. schneider tes@mit.edu
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

#ifndef PROTOBUFNODE20110607H
#define PROTOBUFNODE20110607H

#include "zeromq_service.h"

namespace goby
{
    namespace core
    {
        template<typename NodeTypeBase>
            class NodeInterface
        {
          public:
            ZeroMQService* zeromq_service() { return zeromq_service_; }
            virtual void send(const NodeTypeBase& msg, int socket_id) = 0;
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
