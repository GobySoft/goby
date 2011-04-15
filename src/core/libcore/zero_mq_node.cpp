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

#include <boost/asio/detail/socket_ops.hpp> // for network_to_host_long

#include "goby/acomms/acomms_helpers.h" // for goby::acomms::hex_encode
#include "goby/util/logger.h" // for manipulators die, warn, group(), etc.
#include "goby/util/string.h" // for goby::util::as

#include "zero_mq_node.h"

using goby::util::as;

goby::core::ZeroMQNode::ZeroMQNode(std::ostream* log /* = 0 */)
    : null_(new std::ofstream("/dev/null", std::ios_base::out)),
      log_(log),
      context_(1),
      publisher_(context_, ZMQ_PUB),
      subscriber_(context_, ZMQ_SUB)
{
    if(!log_) log_ = null_;
}

goby::core::ZeroMQNode::~ZeroMQNode()
{
    delete null_;
}


void goby::core::ZeroMQNode::subscribe(MarshallingScheme marshalling_scheme,
                                                   const std::string& identifier)
{
    std::string zmq_filter = make_header(marshalling_scheme, identifier);
    subscriber_.setsockopt(ZMQ_SUBSCRIBE, zmq_filter.c_str(), zmq_filter.size());
}

void goby::core::ZeroMQNode::publish(MarshallingScheme marshalling_scheme,
                                                 const std::string& identifier,
                                                 const void* body_data,
                                                 int body_size)
{
    std::string header = make_header(marshalling_scheme, identifier);

    // pad with nulls to fixed identifier size
    header += std::string(header_size() - header.size(), '\0');
    
    zmq::message_t msg(header.size() + body_size);
    memcpy(msg.data(), header.c_str(), header.size()); // insert header
    memcpy(static_cast<char*>(msg.data()) + header.size(), body_data, body_size); // insert body
    logger() << debug << "message hex: " << goby::acomms::hex_encode(std::string(static_cast<const char*>(msg.data()),msg.size())) << std::endl;
    publisher_.send(msg);
}

void goby::core::ZeroMQNode::start_sockets(const std::string& multicast_connection
                                                       /*= "epgm://127.0.0.1;239.255.7.15:11142"*/)
{
    try
    {
        publisher_.connect(multicast_connection.c_str());
        subscriber_.connect(multicast_connection.c_str());        
        
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
    register_poll_item(item,
                       &goby::core::ZeroMQNode::handle_subscribed_message,
                       this);

}
void goby::core::ZeroMQNode::handle_subscribed_message(const void* data,
                                                                   int size,
                                                                   int message_part)
{
    std::string bytes(static_cast<const char*>(data),
                      size);
    
    logger() << debug
             << "got a message: " << goby::acomms::hex_encode(bytes) << std::endl;
    
    
    static MarshallingScheme marshalling_scheme = MARSHALLING_UNKNOWN;
    static std::string identifier;

    switch(message_part)
    {
        case 0:
        {
            if(bytes.size() < header_size())
                throw(std::runtime_error("Message is too small"));
        
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
        
            
            identifier = bytes.substr(BITS_IN_UINT32 / BITS_IN_BYTE, FIXED_IDENTIFIER_SIZE);

            // remove trailing nulls
            std::string::size_type null_pos = identifier.find_first_of('\0');
            logger() << "identifier " << identifier << std::endl;
            logger() << "null pos " << null_pos << std::endl;
            
            identifier.erase(null_pos);
            logger() << debug << "Got message of type: [" << identifier << "]" << std::endl;

            std::string body(static_cast<const char*>(data)+header_size(),
                             size-header_size());

            logger() << debug << "Body [" << goby::acomms::hex_encode(body)<< "]" << std::endl;
            
            inbox(marshalling_scheme,
                         identifier,
                         static_cast<const char*>(data)+header_size(),
                         size-header_size());
        }
        break;
            
            
        default:
            throw(std::runtime_error("Got more parts to the message than expecting (expecting only 1)"));    
            break;
    }
}

bool goby::core::ZeroMQNode::poll(long timeout /* = -1 */)
{
    bool had_events = false;
    zmq::poll (&poll_items_[0], poll_items_.size(), timeout);
    for(int i = 0, n = poll_items_.size(); i < n; ++i)
    {
        if (poll_items_[i].revents & ZMQ_POLLIN) 
        {
            int message_part = 0;
            int64_t more;
            size_t more_size = sizeof more;
            do {
                /* Create an empty ØMQ message to hold the message part */
                zmq_msg_t part;
                int rc = zmq_msg_init (&part);
                // assert (rc == 0);
                /* Block until a message is available to be received from socket */
                rc = zmq_recv (poll_items_[i].socket, &part, 0);
                poll_callbacks_[i](zmq_msg_data(&part), zmq_msg_size(&part), message_part);
                // assert (rc == 0);
                /* Determine if more message parts are to follow */
                rc = zmq_getsockopt (poll_items_[i].socket, ZMQ_RCVMORE, &more, &more_size);
                // assert (rc == 0);
                zmq_msg_close (&part);
                ++message_part;
            } while (more);
            had_events = true;
        }
    }
    return had_events;
}


std::string goby::core::ZeroMQNode::make_header(MarshallingScheme marshalling_scheme,
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


