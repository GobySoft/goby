// copyright 2011 t. schneider tes@mit.edu
// 
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

#ifndef PROTOBUFPUBSUBNODE20110506H
#define PROTOBUFPUBSUBNODE20110506H

#include "goby/common/pubsub_node_wrapper.h"
#include "protobuf_node.h"

namespace goby
{
    namespace core
    {
        class StaticProtobufPubSubNodeWrapper : public PubSubNodeWrapper<google::protobuf::Message>
        {
          public:
          StaticProtobufPubSubNodeWrapper(StaticProtobufNode* node, const protobuf::PubSubSocketConfig& cfg)
              : PubSubNodeWrapper<google::protobuf::Message>(node, cfg),
                node_(*node)
                { }
            
            ~StaticProtobufPubSubNodeWrapper()
            { }

            template<typename ProtoBufMessage>
                void subscribe(boost::function<void (const ProtoBufMessage&)> handler)
            {
                if(!cfg().using_pubsub())
                {
                    glog.is(goby::util::logger::WARN) && glog << "Ignoring subscribe since we have `using_pubsub`=false" << std::endl;
                    return;
                }    
                node_.subscribe<ProtoBufMessage>(SOCKET_SUBSCRIBE, handler);
            }

            template<typename ProtoBufMessage>
                const ProtoBufMessage& newest() const
            {
                return node_.newest<ProtoBufMessage>();
            }
            

          private:
            StaticProtobufNode& node_;
        };
        
        

    }
}

#endif
