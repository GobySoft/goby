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

#include <zmq.hpp>

#include "goby/core/core_constants.h"

namespace goby
{
    namespace core
    {
        class ZeroMQNode
        {
          public:
            ZeroMQNode(std::ostream* log = 0);
            virtual ~ZeroMQNode();

            virtual void inbox(MarshallingScheme marshalling_scheme,
                                      const std::string& identifier,
                                      const void* data,
                                      int size) = 0;
            
            void subscribe(MarshallingScheme marshalling_scheme,
                           const std::string& identifier);
            
            void publish(MarshallingScheme marshalling_scheme,
                         const std::string& identifier,
                         const void* data,
                         int size);
            
            void start_sockets(const std::string& multicast_connection =
                               "epgm://127.0.0.1;239.255.7.15:11142");
            
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
            }
            
            zmq::context_t& zmq_context() { return context_; }

            enum { FIXED_IDENTIFIER_SIZE = 255 };

            unsigned header_size() 
            { return BITS_IN_UINT32 / BITS_IN_BYTE + FIXED_IDENTIFIER_SIZE; }
            
            
            
          private:
            std::ostream& logger() { return *log_; }
            std::string make_header(MarshallingScheme marshalling_scheme,
                                    const std::string& protobuf_type_name);

            void handle_subscribed_message(const void* data, int size, int message_part);
            
          private:
            std::ofstream* null_;
            std::ostream* log_;

            zmq::context_t context_;
            zmq::socket_t publisher_;
            zmq::socket_t subscriber_;
            std::vector<zmq::pollitem_t> poll_items_;

            // maps poll_items_ index to a callback function
            std::map<size_t, boost::function<void (const void* data, int size, int message_part)> > poll_callbacks_;
        };
    }
}


#endif
