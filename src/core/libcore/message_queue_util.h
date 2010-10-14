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
        /// \name Message Queue Utilities
        //@{

        /// \brief create the name of the queue for connecting to gobyd
        ///
        /// \param self_name name of the current platform (e.g. "unicorn")
        /// \return string name of the boost::interprocess::message_queue for connecting to gobyd
        inline std::string name_connect_listen(const std::string& self_name)
        { return CONNECT_LISTEN_QUEUE_PREFIX + self_name; }

        /// \brief create the name of the queue for the gobyd connection response
        ///
        /// \param self_name name of the current platform (e.g. "unicorn")
        /// \param app_name name of this current application (e.g. "garmin_gps_g")
        /// \return string name of the boost::interprocess::message_queue for listening to connection responses
        inline std::string name_connect_response(const std::string& self_name,
                                                  const std::string& app_name)
        { return CONNECT_LISTEN_QUEUE_PREFIX + app_name + "_" + self_name; }
        
        /// \brief create the name of the queue for sending messages to gobyd from the Application
        ///
        /// \param self_name name of the current platform (e.g. "unicorn")
        /// \param app_name name of this current application (e.g. "garmin_gps_g")
        /// \return string name of the boost::interprocess::message_queue for sending message to gobyd
        inline std::string name_to_server(const std::string& self_name,
                                           const std::string& app_name)
        { return TO_SERVER_QUEUE_PREFIX + app_name + "_" + self_name; }

        
        /// \brief create the name of the queue for sending messages from gobyd to the Application
        ///
        /// \param self_name name of the current platform (e.g. "unicorn")
        /// \param app_name name of this current application (e.g. "garmin_gps_g")
        /// \return string name of the boost::interprocess::message_queue for messages sent from gobyd
        inline std::string name_from_server(const std::string& self_name,
                                             const std::string& app_name)
        { return FROM_SERVER_QUEUE_PREFIX + app_name + "_" + self_name; }
        
        /// \brief wrapper around boost::interprocess::message_queue::send for an already serialized message
        ///
        /// \param queue reference to message_queue to send to
        /// \param buffer pointer to char (byte) buffer that contains the message to send
        /// \param message_size size (in bytes) of the message in `buffer`
        inline void send(boost::interprocess::message_queue& queue,
                         char* buffer, std::size_t message_size)
        { queue.send(buffer, message_size, 0); }

        
        /// \brief wrapper around boost::interprocess::message_queue::send that performs serialization of the Google Protocol Buffers message before sending
        ///
        /// \param queue reference to message_queue to send to
        /// \param out message to serialize and send
        /// \param buffer pointer to char (byte) buffer to use when serializing `out`
        /// \param buffer_size size (in bytes) of `buffer`
        template<typename SerializeFromType>
            void send(boost::interprocess::message_queue& queue,
                      SerializeFromType& out, char* buffer, int buffer_size)
        {            
            out.SerializeToArray(buffer, buffer_size);
            send(queue, buffer, out.ByteSize());
        }
        
        /// \brief wrapper around boost::interprocess::message_queue::try_send for an already serialized message
        ///
        /// \param queue reference to message_queue to send to
        /// \param buffer pointer to char (byte) buffer that contains the message to send
        /// \param message_size size (in bytes) of the message in `buffer`
        /// \return true if successful sending, false otherwise
        inline bool try_send(boost::interprocess::message_queue& queue,
                             char* buffer, std::size_t message_size)
        { return queue.try_send(buffer, message_size, 0); }

        /// \brief wrapper around boost::interprocess::message_queue::try_send that performs serialization of the Google Protocol Buffers message before sending
        ///
        /// \param queue reference to message_queue to send to
        /// \param out message to serialize and send
        /// \param buffer pointer to char (byte) buffer to use when serializing `out`
        /// \param buffer_size size (in bytes) of `buffer`
        /// \return true if successful sending, false otherwise
        template<typename SerializeFromType>
            bool try_send(boost::interprocess::message_queue& queue,
                          SerializeFromType& out, char* buffer, int buffer_size)
        {
            out.SerializeToArray(buffer, buffer_size);
            return try_send(queue, buffer, out.ByteSize());
        }
        
        /// \brief wrapper around boost::interprocess::message_queue::timed_send for an already serialized message
        ///
        /// \param queue reference to message_queue to send to
        /// \param buffer pointer to char (byte) buffer that contains the message to send
        /// \param message_size size (in bytes) of the message in `buffer`
        /// \param abs_time time to try to send the message until (after which sending fails and this function returns false)
        /// \return true if successful sending, false otherwise
        inline bool timed_send(boost::interprocess::message_queue& queue,
                               char* buffer, std::size_t message_size,
                               boost::posix_time::ptime abs_time)
        { return queue.timed_send(buffer, message_size, 0, abs_time); }

        /// \brief wrapper around boost::interprocess::message_queue::timed_send that performs serialization of the Google Protocol Buffers message before sending
        ///
        /// \param queue reference to message_queue to send to
        /// \param out message to serialize and send
        /// \param buffer pointer to char (byte) buffer to use when serializing `out`
        /// \param buffer_size size (in bytes) of `buffer`
        /// \param abs_time time to try to send the message until (after which sending fails and this function returns false)
        /// \return true if successful sending, false otherwise
        template<typename SerializeFromType>
            bool timed_send(boost::interprocess::message_queue& queue,
                          SerializeFromType& out, char* buffer,
                          int buffer_size,
                          boost::posix_time::ptime abs_time)
        {
            out.SerializeToArray(buffer, buffer_size);
            return timed_send(queue, buffer, out.ByteSize(), abs_time);
        }
        
        //
        // receive
        //

        /// \brief wrapper around boost::interprocess::message_queue::receive that performs parsing on the received message
        ///
        /// \param queue reference to message_queue to receive from
        /// \param in message to store parsed received bytes
        /// \param buffer pointer to char (byte) buffer to use when receiving
        /// \param recvd_size pointer to store received size of message
        // TODO(tes): need to throw exception on parsing errors (ParseFromArray is false) 
        template<typename ParseToType>
            void receive(boost::interprocess::message_queue& queue,
                         ParseToType& in,
                         char* buffer,
                         std::size_t* recvd_size)
        {
            unsigned int priority;

            queue.receive(buffer, MAX_MSG_BUFFER_SIZE, *recvd_size, priority);
            in.Clear();
            in.ParseFromArray(buffer, recvd_size);
        }
        
        /// \brief wrapper around boost::interprocess::message_queue::receive that performs parsing on the received message
        ///
        /// \param queue reference to message_queue to receive from
        /// \param in message to store parsed received bytes
        /// \param buffer pointer to char (byte) buffer to use when receiving
        /// \param recvd_size pointer to store received size of message
        /// \param abs_time time to try to receive the message until (after which receiving fails and this function returns false)
        /// \return true if successful receiving, false otherwise
        template<typename ParseToType>
            bool timed_receive(boost::interprocess::message_queue& queue,
                               ParseToType& in,
                               boost::posix_time::ptime abs_time,
                               char* buffer,
                               std::size_t* recvd_size)
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
        //@}

        
    }
}



#endif
