// copyright 2010 t. schneider tes@mit.edu
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


#ifndef MESSAGEQUEUEUTIL20100915H
#define MESSAGEQUEUEUTIL20100915H

#include <boost/interprocess/ipc/message_queue.hpp>

#include <google/protobuf/message.h>

#include "goby/core/core_constants.h"
#include "goby/util/time.h"

namespace goby 
{
    namespace core
    {
        //
        // send
        //
        inline void send(boost::interprocess::message_queue& queue, char* buffer, std::size_t message_size)
        { queue.send(buffer, message_size, 0); }

        template<typename SerializeFromType>
            void send(boost::interprocess::message_queue& queue, SerializeFromType& out, char* buffer, int buffer_size)
        {            
            out.SerializeToArray(buffer, buffer_size);
            send(queue, buffer, out.ByteSize());
        }
        

        inline bool try_send(boost::interprocess::message_queue& queue, char* buffer, std::size_t message_size)
        { return queue.try_send(buffer, message_size, 0); }
        template<typename SerializeFromType>
            bool try_send(boost::interprocess::message_queue& queue, SerializeFromType& out, char* buffer, int buffer_size)
        {
            out.SerializeToArray(buffer, buffer_size);
            return try_send(queue, buffer, out.ByteSize());
        }

        
        //
        // receive
        //

        // TODO(tes): need to throw exception on parsing errors (ParseFromArray is false) 
        template<typename ParseToType>
            void receive(boost::interprocess::message_queue& queue, ParseToType& in, char* buffer, std::size_t* recvd_size)
        {
            unsigned int priority;

            queue.receive(buffer, MAX_MSG_BUFFER_SIZE, *recvd_size, priority);
            in.Clear();
            in.ParseFromArray(buffer, recvd_size);
        }
        
        template<typename ParseToType>
            bool timed_receive(boost::interprocess::message_queue& queue, ParseToType& in, boost::posix_time::ptime abs_time, char* buffer, std::size_t* recvd_size)
        {
            unsigned int priority;

            bool receive_good =
                queue.timed_receive(buffer, MAX_MSG_BUFFER_SIZE, *recvd_size, priority, abs_time);
            if(receive_good)
            {
                in.Clear();
                in.ParseFromArray(buffer, *recvd_size);
            }
            
            return receive_good;
        }
    }
}



#endif
