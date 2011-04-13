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

#include "minimal_application_base.h"

#include "goby/acomms/acomms_helpers.h" // for goby::acomms::hex_encode
#include "goby/util/logger.h" // for manipulators die, warn, group(), etc.
#include "goby/util/string.h" // for goby::util::as

#include <boost/asio/detail/socket_ops.hpp> // for network_to_host_long

using goby::util::as;

goby::core::MinimalApplicationBase::MinimalApplicationBase(std::ostream* log /* = 0 */)
    : null_(new std::ofstream("/dev/null", std::ios_base::out)),
      log_(log),
      context_(1),
      publisher_(context_, ZMQ_PUB),
      subscriber_(context_, ZMQ_SUB)
{
    if(!log_) log_ = null_;
    
}

goby::core::MinimalApplicationBase::~MinimalApplicationBase()
{
    delete null_;
}


void goby::core::MinimalApplicationBase::subscribe(MarshallingScheme marshalling_scheme,
                                                   const std::string& identifier)
{
    std::string zmq_filter = make_header(marshalling_scheme, identifier);
    subscriber_.setsockopt(ZMQ_SUBSCRIBE, zmq_filter.c_str(), zmq_filter.size());
}

void goby::core::MinimalApplicationBase::publish(MarshallingScheme marshalling_scheme,
                                                 const std::string& identifier,
                                                 const void* data,
                                                 int size)
{
    std::string header = make_header(marshalling_scheme, identifier); 
    zmq::message_t msg_header(header.size());
    memcpy(msg_header.data(), header.c_str(), header.size());
    logger() << debug << "header hex: " << goby::acomms::hex_encode(std::string(static_cast<const char*>(header.data()),header.size())) << std::endl;    
    publisher_.send(msg_header, ZMQ_SNDMORE);
    
    zmq::message_t buffer(size);
    memcpy(buffer.data(), data, size);
    logger() << debug << "body hex: " << goby::acomms::hex_encode(std::string(static_cast<const char*>(buffer.data()),buffer.size())) << std::endl;
    publisher_.send(buffer);
}

void goby::core::MinimalApplicationBase::start_sockets(const std::string& multicast_connection
                                                       /*= "epgm://127.0.0.1;239.255.7.15:11142"*/)
{
    try
    {
        publisher_.connect(multicast_connection.c_str());
        subscriber_.connect(multicast_connection.c_str());

        // epgm seems to swallow first message...
        zmq::message_t blank(0); 
        publisher_.send(blank);

        logger() << debug << "connected to: "
                 << multicast_connection << std::endl;
    }
    catch(std::exception& e)
    {
        logger() << die << "cannot connect to: "
                 << multicast_connection << ": " << e.what() << std::endl;
    }

    //  Initialize poll set
    zmq::pollitem_t item = { subscriber_, 0, ZMQ_POLLIN, 0 };
    poll_items_.push_back(item);

}


bool goby::core::MinimalApplicationBase::poll(long timeout /* = -1 */)
{
    bool had_events = false;
    zmq::poll (&poll_items_[0], poll_items_.size(), timeout);
    if (poll_items_[0].revents & ZMQ_POLLIN) 
    {
        int message_part = 0;
        MarshallingScheme marshalling_scheme = MARSHALLING_UNKNOWN;
        std::string identifier;
        int64_t more = 0;
        do
        {
            zmq::message_t message;
            subscriber_.recv(&message);
            std::string bytes(static_cast<const char*>(message.data()),
                              message.size());
            
            logger() << debug
                     << "got a message: " << goby::acomms::hex_encode(bytes) << std::endl;
            
            switch(message_part)
            {
                case 0:
                {
                    if(bytes.size() <  BITS_IN_UINT32 / BITS_IN_BYTE)
                        throw(std::runtime_error("Message header is too small"));
                    
                    google::protobuf::uint32 marshalling_int = 0;
                    for(int i = BITS_IN_UINT32/ BITS_IN_BYTE-1, n = 0; i >= n; --i)
                    {
                        marshalling_int <<= BITS_IN_BYTE;
                        marshalling_int ^= bytes[i];
                    }
                    
                    marshalling_int = boost::asio::detail::socket_ops::network_to_host_long(
                        marshalling_int);                        
                    
                    if(marshalling_int >= MARSHALLING_UNKNOWN &&
                       marshalling_int <= MARSHALLING_MAX)
                        marshalling_scheme = static_cast<MarshallingScheme>(marshalling_int);
                    else
                        throw(std::runtime_error("Invalid marshalling value = "
                                                 + as<std::string>(marshalling_int)));
                    
                    
                    identifier = bytes.substr(BITS_IN_UINT32 / BITS_IN_BYTE);
                    logger() << debug << "Got message of type: " << identifier << std::endl;
                }
                break;
                
                case 1:
                {
                    inbox(marshalling_scheme, identifier, message.data(), message.size());
                    had_events = true;
                }
                break;

                default:
                    throw(std::runtime_error("Got more parts to the message than expecting (expecting only 2"));    
                    break;
            }
            
            size_t more_size = sizeof(more);
            zmq_getsockopt(subscriber_, ZMQ_RCVMORE, &more, &more_size);
            ++message_part;
        } while(more);
    }

    return had_events;
}




std::string goby::core::MinimalApplicationBase::make_header(MarshallingScheme marshalling_scheme,
                                                                  const std::string& identifier)
{
    std::string zmq_filter;
    
    google::protobuf::uint32 marshalling_int = boost::asio::detail::socket_ops::host_to_network_long(static_cast<google::protobuf::uint32>(marshalling_scheme));
    
    for(int i = 0, n = BITS_IN_UINT32 / BITS_IN_BYTE; i < n; ++i)
    {
        zmq_filter.push_back(marshalling_int & 0xFF);
        marshalling_int >>= BITS_IN_BYTE;
    }
    zmq_filter += identifier;

    logger() << debug << "zmq header: " << goby::acomms::hex_encode(zmq_filter) << std::endl;

    return zmq_filter;
}


