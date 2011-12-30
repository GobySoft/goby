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

#include "goby/util/binary.h" // for hex_encode
#include "goby/common/logger.h" // for glog & manipulators die, warn, group(), etc.
#include "goby/util/as.h" // for goby::util::as

#include "zeromq_service.h"
#include "goby/common/exception.h"

using goby::util::as;
using goby::glog;
using goby::util::hex_encode;
using namespace goby::util::logger_lock;
using namespace goby::util::logger;


goby::core::ZeroMQService::ZeroMQService(boost::shared_ptr<zmq::context_t> context)
    : context_(context)
{
}

goby::core::ZeroMQService::ZeroMQService()
    : context_(new zmq::context_t(2))
{    
}

void goby::core::ZeroMQService::process_cfg(const protobuf::ZeroMQServiceConfig& cfg)
{
    for(int i = 0, n = cfg.socket_size(); i < n; ++i)
    {
        if(!sockets_.count(cfg.socket(i).socket_id()))
        {
            // TODO (tes) - check for compatible socket type
            boost::shared_ptr<zmq::socket_t> new_socket(
                new zmq::socket_t(*context_, socket_type(cfg.socket(i).socket_type())));
            
            sockets_.insert(std::make_pair(cfg.socket(i).socket_id(), ZeroMQSocket(new_socket)));
            
            //  Initialize poll set
            zmq::pollitem_t item = { *new_socket, 0, ZMQ_POLLIN, 0 };

            // publish sockets can't receive
            if(cfg.socket(i).socket_type() != protobuf::ZeroMQServiceConfig::Socket::PUBLISH)
            {
                register_poll_item(item,
                                   boost::bind(&goby::core::ZeroMQService::handle_receive,
                                               this, _1, _2, _3, cfg.socket(i).socket_id()));
            }
        }

        boost::shared_ptr<zmq::socket_t> this_socket = socket_from_id(cfg.socket(i).socket_id()).socket();
        
        if(cfg.socket(i).connect_or_bind() == protobuf::ZeroMQServiceConfig::Socket::CONNECT)
        {
            std::string endpoint;
            switch(cfg.socket(i).transport())
            {
                case protobuf::ZeroMQServiceConfig::Socket::INPROC:
                    endpoint = "inproc://" + cfg.socket(i).socket_name();
                    break;
                    
                case protobuf::ZeroMQServiceConfig::Socket::IPC:
                    endpoint = "ipc://" + cfg.socket(i).socket_name();
                    break;
                    
                case protobuf::ZeroMQServiceConfig::Socket::TCP:
                    endpoint = "tcp://" + cfg.socket(i).ethernet_address() + ":"
                        + as<std::string>(cfg.socket(i).ethernet_port());
                    break;
                    
                case protobuf::ZeroMQServiceConfig::Socket::PGM:
                    endpoint = "pgm://" + cfg.socket(i).ethernet_address() + ";"
                        + cfg.socket(i).multicast_address() + ":" + as<std::string>(cfg.socket(i).ethernet_port());
                break;
                    
                case protobuf::ZeroMQServiceConfig::Socket::EPGM:
                    endpoint = "epgm://" + cfg.socket(i).ethernet_address() + ";"
                        + cfg.socket(i).multicast_address() + ":" + as<std::string>(cfg.socket(i).ethernet_port());
                    break;
            }

            try
            {
                this_socket->connect(endpoint.c_str());
                glog.is(DEBUG1, lock) && glog << cfg.socket(i).ShortDebugString() << " connected to endpoint - " << endpoint << std::endl << unlock;
            }    
            catch(std::exception& e)
            {
                glog.is(DIE, lock) &&
                    glog << "cannot connect to: " << endpoint << ": " << e.what() << std::endl << unlock;
            }
        }
        else if(cfg.socket(i).connect_or_bind() == protobuf::ZeroMQServiceConfig::Socket::BIND)
        {
            std::string endpoint;
            switch(cfg.socket(i).transport())
            {
                case protobuf::ZeroMQServiceConfig::Socket::INPROC:
                    endpoint = "inproc://" + cfg.socket(i).socket_name();
                    break;
                    
                case protobuf::ZeroMQServiceConfig::Socket::IPC:
                    endpoint = "ipc://" + cfg.socket(i).socket_name();
                    break;
                    
                case protobuf::ZeroMQServiceConfig::Socket::TCP:
                    endpoint = "tcp://*:" + as<std::string>(cfg.socket(i).ethernet_port());
                    break;
                    
                case protobuf::ZeroMQServiceConfig::Socket::PGM:
                    throw(goby::Exception("Cannot BIND to PGM socket (use CONNECT)"));
                    break;
                    
                case protobuf::ZeroMQServiceConfig::Socket::EPGM:
                    throw(goby::Exception("Cannot BIND to EPGM socket (use CONNECT)"));
                    break;
            }            

            try
            {
                this_socket->bind(endpoint.c_str());
                glog.is(DEBUG1, lock) && glog << cfg.socket(i).ShortDebugString() << "bound to endpoint - " << endpoint << std::endl << unlock;
            }    
            catch(std::exception& e)
            {
                glog.is(DIE, lock) &&
                    glog << "cannot bind to: " << endpoint << ": " << e.what() << std::endl << unlock;
            }

        }
    }
}

goby::core::ZeroMQService::~ZeroMQService()
{
}

int goby::core::ZeroMQService::socket_type(protobuf::ZeroMQServiceConfig::Socket::SocketType type)
{
    switch(type)
    {
        case protobuf::ZeroMQServiceConfig::Socket::PUBLISH: return ZMQ_PUB;
        case protobuf::ZeroMQServiceConfig::Socket::SUBSCRIBE: return ZMQ_SUB;
        case protobuf::ZeroMQServiceConfig::Socket::REPLY: return ZMQ_REP;
        case protobuf::ZeroMQServiceConfig::Socket::REQUEST: return ZMQ_REQ;
//        case protobuf::ZeroMQServiceConfig::Socket::ZMQ_PUSH: return ZMQ_PUSH;
//        case protobuf::ZeroMQServiceConfig::Socket::ZMQ_PULL: return ZMQ_PULL;
//        case protobuf::ZeroMQServiceConfig::Socket::ZMQ_DEALER: return ZMQ_DEALER;
//        case protobuf::ZeroMQServiceConfig::Socket::ZMQ_ROUTER: return ZMQ_ROUTER;
    }
    throw(goby::Exception("Invalid SocketType"));
}

goby::core::ZeroMQSocket& goby::core::ZeroMQService::socket_from_id(int socket_id)
{
    std::map<int, ZeroMQSocket >::iterator it = sockets_.find(socket_id);
    if(it != sockets_.end())
        return it->second;
    else
        throw(goby::Exception("Attempted to access socket_id " + as<std::string>(socket_id) + " which does not exist"));
}

void goby::core::ZeroMQService::subscribe_all(int socket_id)
{
    socket_from_id(socket_id).socket()->setsockopt(ZMQ_SUBSCRIBE, 0, 0);
}

void goby::core::ZeroMQService::unsubscribe_all(int socket_id)
{
    socket_from_id(socket_id).socket()->setsockopt(ZMQ_UNSUBSCRIBE, 0, 0);
}

void goby::core::ZeroMQService::subscribe(MarshallingScheme marshalling_scheme,
                                       const std::string& identifier,
                                       int socket_id)
{
    pre_subscribe_hooks(marshalling_scheme, identifier, socket_id);
    
    std::string zmq_filter = make_header(marshalling_scheme, identifier);
    int NULL_TERMINATOR_SIZE = 1;
    zmq_filter.resize(zmq_filter.size() - NULL_TERMINATOR_SIZE);
    socket_from_id(socket_id).socket()->setsockopt(ZMQ_SUBSCRIBE, zmq_filter.c_str(), zmq_filter.size());
    
    glog.is(DEBUG1, lock) && glog << "subscribed for marshalling " << marshalling_scheme << " with identifier: [" << identifier << "] using zmq_filter: " << goby::util::hex_encode(zmq_filter) << std::endl << unlock;

        
    post_subscribe_hooks(marshalling_scheme, identifier, socket_id);
}

void goby::core::ZeroMQService::unsubscribe(MarshallingScheme marshalling_scheme,
                                       const std::string& identifier,
                                       int socket_id)
{
    std::string zmq_filter = make_header(marshalling_scheme, identifier);
    int NULL_TERMINATOR_SIZE = 1;
    zmq_filter.resize(zmq_filter.size() - NULL_TERMINATOR_SIZE);
    socket_from_id(socket_id).socket()->setsockopt(ZMQ_UNSUBSCRIBE, zmq_filter.c_str(), zmq_filter.size());
    
    glog.is(DEBUG1, lock) && glog << "unsubscribed for marshalling " << marshalling_scheme << " with identifier: [" << identifier << "] using zmq_filter: " << goby::util::hex_encode(zmq_filter) << std::endl << unlock;

        
}


void goby::core::ZeroMQService::send(MarshallingScheme marshalling_scheme,
                                  const std::string& identifier,
                                  const void* body_data,
                                  int body_size,
                                  int socket_id)
{
    pre_send_hooks(marshalling_scheme, identifier, socket_id);
    
    std::string header = make_header(marshalling_scheme, identifier);

    zmq::message_t msg(header.size() + body_size);
    memcpy(msg.data(), header.c_str(), header.size()); // insert header
    memcpy(static_cast<char*>(msg.data()) + header.size(), body_data, body_size); // insert body

    glog.is(DEBUG2, lock) &&
        glog << "message hex: " << hex_encode(std::string(static_cast<const char*>(msg.data()),msg.size())) << std::endl << unlock;
    socket_from_id(socket_id).socket()->send(msg);

    post_send_hooks(marshalling_scheme, identifier, socket_id);
}


void goby::core::ZeroMQService::handle_receive(const void* data,
                                            int size,
                                            int message_part,
                                            int socket_id)
{
    std::string bytes(static_cast<const char*>(data),
                      size);
    

    glog.is(DEBUG2, lock) &&
        glog << "got a message: " << goby::util::hex_encode(bytes) << std::endl << unlock;
    
    
    MarshallingScheme marshalling_scheme = MARSHALLING_UNKNOWN;
    std::string identifier;

    switch(message_part)
    {
        case 0:
        {
            // byte size of marshalling id
            const unsigned MARSHALLING_SIZE = BITS_IN_UINT32 / BITS_IN_BYTE;
            
            if(bytes.size() < MARSHALLING_SIZE)
                throw(std::runtime_error("Message is too small"));

            
            google::protobuf::uint32 marshalling_int = 0;
            for(int i = MARSHALLING_SIZE-1, n = 0; i >= n; --i)
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
        
            
            identifier = bytes.substr(MARSHALLING_SIZE,
                                      bytes.find('\0', MARSHALLING_SIZE)-MARSHALLING_SIZE);

            glog.is(DEBUG1, lock) &&
                glog << "Got message of type: [" << identifier << "]" << std::endl << unlock;

            // +1 for null terminator
            const int HEADER_SIZE = MARSHALLING_SIZE+identifier.size() + 1;
            std::string body(static_cast<const char*>(data)+HEADER_SIZE,
                             size-HEADER_SIZE);
            
            glog.is(DEBUG2, lock) &&
                glog << "Body [" << goby::util::hex_encode(body)<< "]" << std::endl << unlock;
            

            
            if(socket_from_id(socket_id).check_blackout(marshalling_scheme, identifier))
            {
                inbox_signal_(marshalling_scheme,
                              identifier,
                              static_cast<const char*>(data)+HEADER_SIZE,
                              size-HEADER_SIZE,
                              socket_id);
            }
        }
        break;
            
            
        default:
            throw(std::runtime_error("Got more parts to the message than expecting (expecting only 1)"));    
            break;
    }
}

bool goby::core::ZeroMQService::poll(long timeout /* = -1 */)
{
    glog.is(DEBUG2, lock) && glog << "Have " << poll_items_.size() << " items to poll" << std::endl << unlock;   
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
                glog.is(DEBUG2, lock) && glog << "Had event for poll item " << i << std::endl << unlock;
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


std::string goby::core::ZeroMQService::make_header(MarshallingScheme marshalling_scheme,
                                                const std::string& identifier)
{
    std::string zmq_filter;
    
    google::protobuf::uint32 marshalling_int = boost::asio::detail::socket_ops::host_to_network_long(static_cast<google::protobuf::uint32>(marshalling_scheme));
    
    for(int i = 0, n = BITS_IN_UINT32 / BITS_IN_BYTE; i < n; ++i)
    {
        zmq_filter.push_back(marshalling_int & 0xFF);
        marshalling_int >>= BITS_IN_BYTE;
    }
    zmq_filter += identifier + '\0';

    glog.is(DEBUG2, lock) &&
        glog << "zmq header: " << goby::util::hex_encode(zmq_filter) << std::endl << unlock;

    return zmq_filter;
}


void goby::core::ZeroMQSocket::set_global_blackout(boost::posix_time::time_duration duration)
{
    glog.is(DEBUG2, lock) &&
        glog << "Global blackout set to " << duration << std::endl << unlock;
    global_blackout_ = duration;
    global_blackout_set_ = true;
}


void goby::core::ZeroMQSocket::set_blackout(MarshallingScheme marshalling_scheme,
                                            const std::string& identifier,
                                            boost::posix_time::time_duration duration)            
{
    glog.is(DEBUG2, lock) &&
        glog << "Blackout for marshalling scheme: " << marshalling_scheme << " and identifier " << identifier << " set to " << duration << std::endl << unlock;
    blackout_info_[std::make_pair(marshalling_scheme, identifier)] = BlackoutInfo(duration);
    local_blackout_set_ = true;
}

bool goby::core::ZeroMQSocket::check_blackout(MarshallingScheme marshalling_scheme,
                                              const std::string& identifier)
{
    if(!local_blackout_set_ && !global_blackout_set_)
    {
        return true;
    }
    else
    {
        boost::posix_time::ptime this_time = goby::util::goby_time();

        BlackoutInfo& blackout_info = blackout_info_[std::make_pair(marshalling_scheme, identifier)];
        
        const boost::posix_time::ptime& last_post_time =
            blackout_info.last_post_time;
        
        if((local_blackout_set_ && // local blackout
            this_time - last_post_time > blackout_info.blackout_interval))
        {
            blackout_info.last_post_time = this_time;
            return true;
        }
        else if((global_blackout_set_ && // global blackout
                 this_time - last_post_time > global_blackout_)) // fails global blackout
        {
            blackout_info.last_post_time = this_time;
            return true;
        }
        else
        {
            glog.is(DEBUG2, lock) && 
                glog << "Message (marshalling scheme: " << marshalling_scheme
                     << ", identifier: " << identifier << ")" << " is in blackout: this time:"
                     << this_time << ", last time: " << last_post_time << ", global blackout: "
                     << global_blackout_ << ", local blackout: " << blackout_info.blackout_interval
                     << ", difference last and this: " << this_time - last_post_time
                     << std::endl << unlock;
            return false;
        }
    }    
}
