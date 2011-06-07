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

#ifndef ZEROMQNODE20110413H
#define ZEROMQNODE20110413H

#include <iostream>
#include <string>
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/signals.hpp>

#include "goby/protobuf/zero_mq_node_config.pb.h"

#include <zmq.hpp>

#include "goby/core/core_constants.h"

namespace goby
{
    namespace core
    {
        class ZeroMQNode
        {
          public:
           static ZeroMQNode* get()
            {
                if(!inst_)
                    inst_.reset(new ZeroMQNode);
                return inst_.get();
            }
            
            
            template<class C>
                void connect_inbox_slot(
                    void(C::*mem_func)(MarshallingScheme,
                                       const std::string&,
                                       const void*,
                                       int,
                                       int),
                    C* obj)
            { connect_inbox_slot(boost::bind(mem_func, obj, _1, _2, _3, _4, _5)); }

            void set_cfg(const protobuf::ZeroMQNodeConfig& cfg)
            {
                process_cfg(cfg);
                cfg_.CopyFrom(cfg);
            }
            
            void merge_cfg(const protobuf::ZeroMQNodeConfig& cfg)
            {
                process_cfg(cfg);
                cfg_.MergeFrom(cfg);
            }
            
            void connect_inbox_slot(
                boost::function<void (MarshallingScheme marshalling_scheme,
                                      const std::string& identifier,
                                      const void* data,
                                      int size,
                                      int socket_id)> slot)
            { inbox_signal_.connect(slot); }

            void subscribe_all(int socket_id);            
            
            void send(MarshallingScheme marshalling_scheme,
                      const std::string& identifier,
                      const void* data,
                      int size,
                      int socket_id);
            
            void subscribe(MarshallingScheme marshalling_scheme,
                           const std::string& identifier,
                           int socket_id);

            
            bool poll(long timeout = -1);
            
            template<class C>
                void register_poll_item(
                    const zmq::pollitem_t& item,                    
                    void(C::*mem_func)(const void*, int, int),
                    C* obj)
            { register_poll_item(item, boost::bind(mem_func, obj, _1, _2, _3)); }
            

            void register_poll_item(
                const zmq::pollitem_t& item,
                boost::function<void (const void* data, int size, int message_part)> callback)
            {
                poll_items_.push_back(item);
                poll_callbacks_.insert(std::make_pair(poll_items_.size()-1, callback));
                //glog.is(debug2) && glog << "Registering poll item " << poll_items_.size()-1 << std::endl;
            }
            
            zmq::context_t& zmq_context() { return context_; }


            boost::signal<void (MarshallingScheme marshalling_scheme,
                                const std::string& identifier,
                                int socket_id)> pre_send_hooks;

            boost::signal<void (MarshallingScheme marshalling_scheme,
                                const std::string& identifier,
                                int socket_id)> pre_subscribe_hooks;

            boost::signal<void (MarshallingScheme marshalling_scheme,
                                const std::string& identifier,
                                int socket_id)> post_send_hooks;

            boost::signal<void (MarshallingScheme marshalling_scheme,
                                const std::string& identifier,
                                int socket_id)> post_subscribe_hooks;
            
            
            
          private:
            ZeroMQNode();
            virtual ~ZeroMQNode();

            ZeroMQNode(const ZeroMQNode&);
            ZeroMQNode& operator= (const ZeroMQNode&);
            
            void process_cfg(const protobuf::ZeroMQNodeConfig& cfg);

            std::string make_header(MarshallingScheme marshalling_scheme,
                                    const std::string& protobuf_type_name);

            void handle_receive(const void* data, int size, int message_part, int socket_id);

            int socket_type(protobuf::ZeroMQNodeConfig::Socket::SocketType type);
            
            boost::shared_ptr<zmq::socket_t> socket_from_id(int socket_id);

            // so we can use shared_ptr to hold the singleton
            template<typename T>
                friend void boost::checked_delete(T*);
          private:
            static boost::shared_ptr<ZeroMQNode> inst_;
                        
            zmq::context_t context_;
            std::map<int, boost::shared_ptr<zmq::socket_t> > sockets_;
            std::vector<zmq::pollitem_t> poll_items_;

            protobuf::ZeroMQNodeConfig cfg_;
            
            // maps poll_items_ index to a callback function
            std::map<size_t, boost::function<void (const void* data, int size, int message_part)> > poll_callbacks_;
            
            boost::signal<void (MarshallingScheme marshalling_scheme,
                                const std::string& identifier,
                                const void* data,
                                int size,
                                int socket_id)> inbox_signal_;

        };
    }
}


#endif
