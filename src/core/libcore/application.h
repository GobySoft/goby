// copyright 2010-2011 t. schneider tes@mit.edu
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

#ifndef APPLICATION20100908H
#define APPLICATION20100908H

#include <boost/shared_ptr.hpp>
#include <boost/date_time.hpp>

#include <google/protobuf/io/zero_copy_stream_impl.h>

#include "goby/util/logger.h"
#include "goby/core/core_helpers.h"

#include "goby/protobuf/config.pb.h"
#include "goby/protobuf/core_database_request.pb.h"
#include "goby/protobuf/header.pb.h"

#include "zeromq_application_base.h"
#include "protobuf_node.h"

namespace google { namespace protobuf { class Message; } }
namespace goby
{
        
    /// Contains objects relating to the core publish / subscribe architecture provided by Goby.
    namespace core
    {
        /// Base class provided for users to generate applications that participate in the Goby publish/subscribe architecture.
        class Application : public StaticProtobufNode, public ZeroMQApplicationBase
        {
          protected:
            // make these more accessible
            typedef goby::util::Colors Colors;
            
            /// \name Constructors / Destructor
            //@{
            /// \param cfg pointer to object derived from google::protobuf::Message that defines the configuration for this Application. This constructor will use the Description of `cfg` to read the command line parameters and configuration file (if given) and use these values to populate `cfg`. `cfg` must be a static member of the subclass or global object since member objects will be constructed *after* the Application constructor is called.
            Application(google::protobuf::Message* cfg = 0);
            virtual ~Application();
            //@}            
            
            /// \name Publish / Subscribe
            //@{
            
            /// \brief Interplatform publishing options. `self` publishes only to the local multicast group, `other` also attempts to transmit to the named other platform, and `all` attempts to transmit to all known platforms
            enum PublishDestination { self = ::Header::PUBLISH_SELF,
                                      other = ::Header::PUBLISH_OTHER,
                                      all = ::Header::PUBLISH_ALL };
            

            /// \brief Publish a message (of any type derived from google::protobuf::Message)
            ///
            /// \param msg Message to publish
            /// \param platform_name Platform to send to as well as `self` if PublishDestination == other
            template<PublishDestination dest>
                void publish(google::protobuf::Message& msg, const std::string& platform_name = "")
            { __publish(msg, platform_name, dest); }            

            /// \brief Publish a message (of any type derived from google::protobuf::Message) to all local subscribers (self)
            ///
            /// \param msg Message to publish
            void publish(google::protobuf::Message& msg)
            { __publish(msg, "", self); }

            //@}


            /// \brief Subscribe to a message (of any type derived from google::protobuf::Message)            
            ///
            /// \param handler Function object to be called as soon as possible upon receipt of a message of this type. The signature of `handler` must match: void handler(const ProtoBufMessage& msg). if `handler` is omitted, no handler is called and only the newest message buffer is updated upon message receipt (for calls to newest<ProtoBufMessage>())
             template<typename ProtoBufMessage>
                void subscribe(
                    boost::function<void (const ProtoBufMessage&)> handler =
                    boost::function<void (const ProtoBufMessage&)>()
                    )
             {
                 if(base_cfg().pubsub_config().using_pubsub())
                 {
                     glog.is(warn) && glog << "Ignoring subscribe since we have `base.pubsub_config.using_pubsub`=false" << std::endl;
                     return;
                 }
    
                 StaticProtobufNode::subscribe<ProtoBufMessage>(
                     SOCKET_SUBSCRIBE, handler);
             }
             
            
            /// \brief Subscribe for a type using a class member function as the handler
            /// 
            /// \param mem_func Member function (method) of class C with a signature of void C::mem_func(const ProtoBufMessage& msg)
            /// \param obj pointer to the object whose member function (mem_func) to call
            template<typename ProtoBufMessage, class C>
                void subscribe(void(C::*mem_func)(const ProtoBufMessage&),
                               C* obj)
            { subscribe<ProtoBufMessage>(boost::bind(mem_func, obj, _1)); }
            
            
          private:
            Application(const Application&);
            Application& operator= (const Application&);

            enum {
                SOCKET_SUBSCRIBE = 103998, SOCKET_PUBLISH = 103999
            };
            void __set_up_sockets();


            // add the protobuf description of the given descriptor (essentially the
            // instructions on how to make the descriptor or message meta-data)
            // to the notification_ message. 
            void __insert_file_descriptor_proto(
                const google::protobuf::FileDescriptor* file_descriptor,
                protobuf::DatabaseRequest* request);
            

            // adds required fields to the Header if not given by the derived application
            void __finalize_header(
                google::protobuf::Message* msg,
                const goby::core::Application::PublishDestination dest_type,
                const std::string& dest_platform);
            
            void __publish(google::protobuf::Message& msg, const std::string& platform_name,  PublishDestination dest);
            
          private:
            // database related things
            zmq::socket_t database_client_;
            std::set<const google::protobuf::FileDescriptor*> registered_file_descriptors_;
            

        };
    }
}

#endif
