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

#ifndef MINIMALAPPLICATIONBASE20110413H
#define MINIMALAPPLICATIONBASE20110413H

#include <iostream>
#include <string>

#include <zmq.hpp>

#include "goby/core/core_constants.h"

namespace goby
{
    namespace core
    {
        class MinimalApplicationBase
        {
          public:
            MinimalApplicationBase(std::ostream* log = 0);
            virtual ~MinimalApplicationBase();

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

            zmq::context_t& zmq_context() { return context_; }
            
          private:
            std::ostream& logger() { return *log_; }
            std::string make_header(MarshallingScheme marshalling_scheme,
                                    const std::string& protobuf_type_name);
            
          private:
            std::ofstream* null_;
            std::ostream* log_;

            zmq::context_t context_;
            zmq::socket_t publisher_;
            zmq::socket_t subscriber_;
            std::vector<zmq::pollitem_t> poll_items_;
        };
    }
}


#endif
