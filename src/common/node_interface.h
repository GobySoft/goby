
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
