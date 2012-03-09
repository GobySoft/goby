
#ifndef MOOSNODE20110419H
#define MOOSNODE20110419H


#include "moos_serializer.h"
#include "moos_string.h"

#include "goby/common/node_interface.h"
#include "goby/common/zeromq_service.h"

namespace goby
{
    namespace moos
    {        
        class MOOSNode : public goby::common::NodeInterface<CMOOSMsg>
        {
          public:
            MOOSNode(goby::common::ZeroMQService* service);
            
            virtual ~MOOSNode()
            { }
            
            void send(const CMOOSMsg& msg, int socket_id);
            void subscribe(const std::string& full_or_partial_moos_name, int socket_id);
            void unsubscribe(const std::string& full_or_partial_moos_name, int socket_id);
            
            
            
            CMOOSMsg& newest(const std::string& key);

            // returns newest for "BOB", "BIG", when substr is "B"
            std::vector<CMOOSMsg> newest_substr(const std::string& substring);
            
            
          protected:
            // not const because CMOOSMsg requires mutable for many const calls...
            virtual void moos_inbox(CMOOSMsg& msg) = 0;
            
            
            
          private:
            void inbox(goby::common::MarshallingScheme marshalling_scheme,
                       const std::string& identifier,
                       const void* data,
                       int size,
                       int socket_id);

          private:
            std::map<std::string, boost::shared_ptr<CMOOSMsg> > newest_vars;
            
        };
    }
}


#endif
