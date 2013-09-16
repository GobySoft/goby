// Copyright 2009-2013 Toby Schneider (https://launchpad.net/~tes)
//                     Massachusetts Institute of Technology (2007-)
//                     Woods Hole Oceanographic Institution (2007-)
//                     Goby Developers Team (https://launchpad.net/~goby-dev)
// 
//
// This file is part of the Goby Underwater Autonomy Project Libraries
// ("The Goby Libraries").
//
// The Goby Libraries are free software: you can redistribute them and/or modify
// them under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// The Goby Libraries are distributed in the hope that they will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with Goby.  If not, see <http://www.gnu.org/licenses/>.


#include <boost/foreach.hpp>

#include "goby/common/logger.h"
#include "goby/common/time.h"
#include "goby/util/binary.h"

#include "goby/acomms/dccl.h"
#include "goby/common/logger.h"
#include "goby/util/dynamic_protobuf_manager.h"

#include "queue_manager.h"
#include "queue_constants.h"


using goby::glog;
using goby::util::as;
using namespace goby::common::logger;

int goby::acomms::QueueManager::count_ = 0;

goby::acomms::QueueManager::QueueManager()
    : packet_ack_(0),
      packet_dest_(BROADCAST_ID),
      codec_(goby::acomms::DCCLCodec::get())
{
    ++count_;
    
    glog_push_group_ = "goby::acomms::queue::push::" + as<std::string>(count_);
    glog_pop_group_ = "goby::acomms::queue::pop::" + as<std::string>(count_);
    glog_priority_group_ = "goby::acomms::queue::priority::" + as<std::string>(count_);
    glog_out_group_ = "goby::acomms::queue::out::" + as<std::string>(count_);
    glog_in_group_ = "goby::acomms::queue::in::" + as<std::string>(count_);

    
    goby::glog.add_group(glog_push_group_, common::Colors::lt_cyan);
    goby::glog.add_group(glog_pop_group_,  common::Colors::lt_green);
    goby::glog.add_group(glog_priority_group_,  common::Colors::yellow);
    goby::glog.add_group(glog_out_group_,  common::Colors::cyan);
    goby::glog.add_group(glog_in_group_,  common::Colors::green);

}

void goby::acomms::QueueManager::add_queue(const google::protobuf::Descriptor* desc,
                                           const protobuf::QueuedMessageEntry& queue_cfg /*= protobuf::QueuedMessageEntry()*/)
{
    
    // does the queue exist?
    unsigned dccl_id = codec_->id(desc);    
    if(queues_.count(dccl_id))
    {
        glog.is(DEBUG1) && glog << group(glog_push_group_) << "Updating config for queue: " << desc->full_name() << " with: " << queue_cfg.ShortDebugString() << std::endl;
        
        queues_.find(dccl_id)->second->set_cfg(queue_cfg);
        return;
    }
    
    try
    {
        //validate with DCCL first
        codec_->validate(desc);
    }
    catch(DCCLException& e)
    {
        throw(QueueException("could not create queue for message: " + desc->full_name() + " because it failed DCCL validation: " + e.what()));
    }
    
    // add the newly generated queue
    if(queues_.count(dccl_id))
    {
        std::stringstream ss;
        ss << "Queue: duplicate message specified for DCCL ID: " << dccl_id;
        throw QueueException(ss.str());
    }
    else
    {
        std::pair<std::map<unsigned, boost::shared_ptr<Queue> >::iterator,bool> new_q_pair =
            queues_.insert(std::make_pair(dccl_id, boost::shared_ptr<Queue>( new Queue(desc, this, queue_cfg))));

        Queue& new_q = *((new_q_pair.first)->second);
        
        qsize(&new_q);
        
        glog.is(DEBUG1) && glog<< group(glog_out_group_) << "Added new queue: \n" << new_q << std::endl;
        
    }

}


void goby::acomms::QueueManager::do_work()
{
    typedef std::pair<unsigned, Queue> P;
    for(std::map<unsigned, boost::shared_ptr<Queue> >::iterator it = queues_.begin(), n = queues_.end(); it != n; ++it)
    {
        std::vector<boost::shared_ptr<google::protobuf::Message> >expired_msgs =
            it->second->expire();

        BOOST_FOREACH(boost::shared_ptr<google::protobuf::Message> expire, expired_msgs)
        {
            signal_expire(*expire);
        }
        
    }
    
}

void goby::acomms::QueueManager::push_message(const google::protobuf::Message& dccl_msg)
{
    push_message(dccl_msg, 0);
}

void goby::acomms::QueueManager::push_message(const google::protobuf::Message& dccl_msg,
                                              const protobuf::QueuedMessageMeta* meta)
{
    const google::protobuf::Descriptor* desc = dccl_msg.GetDescriptor();
    unsigned dccl_id = codec_->id(desc);

    if(!queues_.count(dccl_id))
        throw(QueueException("No queue exists for message: " + desc->full_name() + "; you must configure it first."));
        
    // add the message
    boost::shared_ptr<google::protobuf::Message> new_dccl_msg(dccl_msg.New());
    new_dccl_msg->CopyFrom(dccl_msg);

    if(meta)
        queues_.find(dccl_id)->second->push_message(new_dccl_msg, *meta);
    else
        queues_.find(dccl_id)->second->push_message(new_dccl_msg);
    
    qsize(queues_[dccl_id].get());     
}
 

void goby::acomms::QueueManager::flush_queue(const protobuf::QueueFlush& flush)
{
    std::map<unsigned, boost::shared_ptr<Queue> >::iterator it = queues_.find(flush.dccl_id());
    
    if(it != queues_.end())
    {
        it->second->flush();
        glog.is(DEBUG1) && glog << group(glog_out_group_) << msg_string(it->second->descriptor())
                                << ": flushed queue" << std::endl;
        qsize(it->second.get());
    }    
    else
    {
        glog.is(DEBUG1) && glog << group(glog_out_group_) << warn << "Cannot find queue to flush: " << flush << std::endl;
    }
}

void goby::acomms::QueueManager::info_all(std::ostream* os) const
{
    *os << "= Begin QueueManager [[" << queues_.size() << " queues total]] =" << std::endl;
    for(std::map<unsigned, boost::shared_ptr<Queue> >::const_iterator it = queues_.begin(), n = queues_.end(); it != n; ++it)
        info(it->second->descriptor(), os);
    *os << "= End QueueManager =";
}



void goby::acomms::QueueManager::info(const google::protobuf::Descriptor* desc, std::ostream* os) const
{

    std::map<unsigned, boost::shared_ptr<Queue> >::const_iterator it = queues_.find(codec_->id(desc));

    if(it != queues_.end())
        it->second->info(os);
    else
        *os << "No such queue [[" << desc->full_name() << "]] loaded" << "\n";

    
    codec_->info(desc, os);
}
    
std::ostream& goby::acomms::operator<< (std::ostream& out, const QueueManager& d)
{
    d.info_all(&out);
    return out;
}

// finds and publishes outgoing data for the modem driver
// first query every Queue for its priority data using
// priority_values(priority, last_send_time)
// priority_values returns false if that object has no data to give
// (either no data at all, or in blackout interval) 
// thus, from all the priority values that return true, pick the one with the lowest
// priority value, or given a tie, pick the one with the oldest last_send_time
void goby::acomms::QueueManager::handle_modem_data_request(protobuf::ModemTransmission* msg)
{
    // clear old waiting acknowledgments and reset packet defaults
    clear_packet();    
    
    for(int frame_number = msg->frame_size(), total_frames = msg->max_num_frames();
        frame_number < total_frames; ++frame_number)
    {
        std::string* data = msg->add_frame();

        glog.is(DEBUG2) && glog << group(glog_priority_group_)
                                << "Finding next sender: "
                                << *msg << std::flush;
        
        // first (0th) user-frame
        Queue* winning_queue = find_next_sender(*msg, *data, true);
    
        // no data at all for this frame ... :(
        if(!winning_queue)
        {
            msg->set_dest(packet_dest_);
            glog.is(DEBUG1) && glog << group(glog_out_group_)
                                    << "No data found. sending empty message to modem driver."
                                    << std::endl;
        }
        else
        {
            std::list<boost::shared_ptr<google::protobuf::Message> > dccl_msgs;

            // set true if we are passing on encrypted data untouched
            bool using_encrypted_body = false;
            std::string passthrough_message;
            
            while(winning_queue)
            {
                // new user frame (e.g. 32B)
                QueuedMessage next_user_frame = winning_queue->give_data(frame_number);

                if(next_user_frame.meta.has_encoded_message())
                {
                    using_encrypted_body = true;
                    passthrough_message = next_user_frame.meta.encoded_message();
                }
                
                glog.is(DEBUG1) && glog << group(glog_out_group_)
                                        << msg_string(winning_queue->descriptor())
                                        << ": sending data to modem driver (destination: "
                                        << next_user_frame.meta.dest() << ")" << std::endl;
                
                if(manip_manager_.has(codec_->id(winning_queue->descriptor()), protobuf::LOOPBACK_AS_SENT))
                {
                    glog.is(DEBUG1) && glog << group(glog_out_group_)
                                            << winning_queue->descriptor()->full_name()
                                            << ": LOOPBACK_AS_SENT manipulator set, sending back to decoder" << std::endl;
                    signal_receive(*next_user_frame.dccl_msg);
                }

                
                //
                // ACK
                // 
                // insert ack if desired
                if(next_user_frame.meta.ack_requested())
                {
                    glog.is(DEBUG2) &&
                        glog << group(glog_out_group_) << "inserting ack for queue: " << *winning_queue << std::endl;
                    waiting_for_ack_.insert(std::pair<unsigned, Queue*>(frame_number,
                                                                        winning_queue));
                }
                else
                {
                    glog.is(DEBUG2) &&
                        glog << group(glog_out_group_) << "no ack, popping from queue: " << *winning_queue << std::endl;
                    if(!winning_queue->pop_message(frame_number))
                        glog.is(DEBUG1) &&
                            glog << group(glog_out_group_) <<  "failed to pop from queue: " << *winning_queue << std::endl;
                    
                    qsize(winning_queue); // notify change in queue size
                }

                // if an ack been set, do not unset these
                if (packet_ack_ == false) packet_ack_ = next_user_frame.meta.ack_requested();
    

                //
                // DEST
                // 
                // update destination address
                if(frame_number == 0)
                {
                    // discipline the destination of the packet if initially unset
                    if(msg->dest() == QUERY_DESTINATION_ID)
                        msg->set_dest(next_user_frame.meta.dest());

                    if(msg->src() == QUERY_SOURCE_ID)
                        msg->set_src(modem_id_);
                    
                    if(packet_dest_ == BROADCAST_ID)
                        packet_dest_ = msg->dest();
                }
                
                //
                // DCCL
                //
                // // e.g. 32B
                // std::string new_data = next_user_frame;
        
                // // insert the size of the next field (e.g. 32B) right after the header
                // std::string frame_size(USER_FRAME_NEXT_SIZE_BYTES,
                //                        static_cast<char>((next_user_frame.data().size()-DCCL_NUM_HEADER_BYTES)));
                // new_data.insert(DCCL_NUM_HEADER_BYTES, frame_size);        

                dccl_msgs.push_back(next_user_frame.dccl_msg);

                // 
                if(using_encrypted_body)
                {
                    // can't pack multiple messages here
                    break;
                }
                else
                {
                    unsigned repeated_size_bytes = codec_->size_repeated(dccl_msgs);
                    
                    glog.is(DEBUG2) && glog << group(glog_out_group_) << "Size repeated " << repeated_size_bytes << std::endl;
                    data->resize(repeated_size_bytes);
                    
                    // if remaining bytes is greater than 0, we have a chance of adding another user-frame
                    if((msg->max_frame_bytes() - data->size()) > 0)
                    {
                        // fetch the next candidate
                        winning_queue = find_next_sender(*msg, *data, false);
                    }
                    else
                    {
                        break;
                    }
                }
            }
        
            // finally actually encode the message
            try
            {
                if(using_encrypted_body)
                {
                    glog.is(DEBUG3) && glog << group(glog_out_group_) << "Encoding head only, passing through (encrypted?) body." << std::endl;
                    std::string head;
                    codec_->encode(&head, *dccl_msgs.back(), true);
                    *data = head + passthrough_message.substr(head.size());
                }
                else
                {
                    *data = codec_->encode_repeated<boost::shared_ptr<google::protobuf::Message> >(dccl_msgs);
                }    
            }
            catch(DCCLException& e)
            {
                *data = "";
                glog.is(DEBUG1) && glog << group(glog_out_group_) << warn << "Failed to encode, discarding message." << std::endl;
            }
        }
    }
    // only discipline the ACK value at the end, after all chances of making packet_ack_ = true are done
    msg->set_ack_requested(packet_ack_);
}


void goby::acomms::QueueManager::clear_packet()
{
    for (std::multimap<unsigned, Queue*>::iterator it = waiting_for_ack_.begin(),
             end = waiting_for_ack_.end(); it != end;)
    {
        if (it->second->clear_ack_queue())
            waiting_for_ack_.erase(it++);
        else
            ++it;  
    }
    
    packet_ack_ = false;
    packet_dest_ = BROADCAST_ID;
}



goby::acomms::Queue* goby::acomms::QueueManager::find_next_sender(const protobuf::ModemTransmission& request_msg, const std::string& data, bool first_user_frame)
{   
    // competition between variable about who gets to send
    double winning_priority;
    boost::posix_time::ptime winning_last_send_time;

    Queue* winning_queue = 0;
    
    glog.is(DEBUG1) && glog<< group(glog_priority_group_) << "Starting priority contest\n"
                           << "\tRequesting "<< request_msg.max_num_frames()
                           <<  " frame(s), have " << data.size()
                           << "/" << request_msg.max_frame_bytes() << "B" << std::endl;

    
    for(std::map<unsigned, boost::shared_ptr<Queue> >::iterator it = queues_.begin(), n = queues_.end(); it != n; ++it)
    {
        Queue& q = *(it->second);
        
         // encode on demand
        if(manip_manager_.has(codec_->id(q.descriptor()), protobuf::ON_DEMAND) && 
           (!q.size() ||
            q.newest_msg_time() +
            boost::posix_time::microseconds(cfg_.on_demand_skew_seconds() * 1e6) <
            common::goby_time()))
        {
            boost::shared_ptr<google::protobuf::Message> new_msg = goby::util::DynamicProtobufManager::new_protobuf_message(q.descriptor());
            signal_data_on_demand(request_msg, new_msg.get());
            
            if(new_msg->IsInitialized())
                push_message(*new_msg);
        }
        
        double priority;
        boost::posix_time::ptime last_send_time;
        if(q.get_priority_values(&priority, &last_send_time, request_msg, data))
        {
            // no winner, better winner, or equal & older winner
            if((!winning_queue || priority > winning_priority ||
                (priority == winning_priority && last_send_time < winning_last_send_time)))
            {
                winning_priority = priority;
                winning_last_send_time = last_send_time;
                winning_queue = &q;
            }
        }
    }

    glog.is(DEBUG1) && glog<< group(glog_priority_group_) << "\t"
                            << "all other queues have no messages" << std::endl;

    if(winning_queue)
    {
        glog.is(DEBUG1) && glog<< group(glog_priority_group_) << winning_queue->name()
                                << " has highest priority." << std::endl;
    }
    else
    {
        glog.is(DEBUG1) && glog<< group(glog_priority_group_) 
                                << "ending priority contest" << std::endl;    
    }
    
    return winning_queue;
}    


void goby::acomms::QueueManager::process_modem_ack(const protobuf::ModemTransmission& ack_msg)
{

    for(int i = 0, n = ack_msg.acked_frame_size(); i < n; ++i)
    {
        int frame_number = ack_msg.acked_frame(i);
        if(ack_msg.dest() != modem_id_)
        {
            glog.is(WARN) && glog << group(glog_in_group_)
                                  << "ignoring ack for modem_id = " << ack_msg.dest() << std::endl;
            continue;
        }
        else if(!waiting_for_ack_.count(frame_number))
        {
            glog.is(DEBUG1) && glog << group(glog_in_group_)
                                    << "got ack but we were not expecting one from " << ack_msg.src() << " for frame " << frame_number << std::endl;
            continue;
        }
        else
        {
        
            // got an ack, let's pop this!
            glog.is(DEBUG1) && glog << group(glog_in_group_) << "received ack for us from " << ack_msg.src() << " for frame " << frame_number << std::endl;
        
        
            std::multimap<unsigned, Queue *>::iterator it = waiting_for_ack_.find(frame_number);
            while(it != waiting_for_ack_.end())
            {
                Queue* q = it->second;
            
                boost::shared_ptr<google::protobuf::Message> removed_msg;
                if(!q->pop_message_ack(frame_number, removed_msg))
                {
                    glog.is(DEBUG1) && glog<< group(glog_in_group_)  << warn
                                         << "failed to pop message from "
                                         << q->name() << std::endl;
                }
                else
                {
                    qsize(q);
                    signal_ack(ack_msg, *removed_msg);
                
                }

                glog.is(DEBUG2) && glog<< group(glog_in_group_) << ack_msg << std::endl;
                
                waiting_for_ack_.erase(it);
                
                it = waiting_for_ack_.find(frame_number);
            }
        }
    }   
}


// parses and publishes incoming data
// by matching the variableID field with the variable specified
// in a "receive = " line of the configuration file
void goby::acomms::QueueManager::handle_modem_receive(const protobuf::ModemTransmission& modem_message)
{    
    glog.is(DEBUG2) && glog<< group(glog_in_group_) << "Received message"
                           << ": " << modem_message << std::endl;

    if(modem_message.type() == protobuf::ModemTransmission::ACK)
    {
        process_modem_ack(modem_message);
    }
    else
    {
        for(int frame_number = 0, total_frames = modem_message.frame_size();
            frame_number < total_frames;
            ++frame_number)
        {
            try
            {
                glog.is(DEBUG1) && glog << group(glog_in_group_)
                                       << "Received DATA message from "
                                        << modem_message.src() << std::endl;
                
                std::list<boost::shared_ptr<google::protobuf::Message> > dccl_msgs;

                try
                {
                    if(!cfg_.skip_decoding())
                    {
                        dccl_msgs = codec_->decode_repeated<boost::shared_ptr<google::protobuf::Message> >(modem_message.frame(frame_number));
                    }
                    
                    if(!signal_in_route.empty())
                    {
                        // decode only header
                        boost::shared_ptr<google::protobuf::Message> decoded_message =
                            codec_->decode<boost::shared_ptr<google::protobuf::Message> >(
                                modem_message.frame(frame_number), true);
                        protobuf::QueuedMessageMeta meta_msg = meta_from_msg(*decoded_message);
                        // messages addressed to us on the link
                        if(modem_message.dest() == modem_id_)
                        {
                            meta_msg.set_encoded_message(modem_message.frame(frame_number));
                            signal_in_route(meta_msg, *decoded_message, modem_id_);
                        }
                    }
                }
                catch(DCCLException& e)
                {
                    glog.is(DEBUG1) && glog << group(glog_in_group_) << warn
                                            << "Failed to decode, discarding message." << std::endl;
                }
                

                BOOST_FOREACH(boost::shared_ptr<google::protobuf::Message> decoded_message, dccl_msgs)
                {
                    protobuf::QueuedMessageMeta meta_msg = meta_from_msg(*decoded_message);
                    
                    int32 dest = meta_msg.dest();
                    
                    const google::protobuf::Descriptor* desc = decoded_message->GetDescriptor();
                    
                    if(dest != BROADCAST_ID &&
                       dest != modem_id_ &&
                       !manip_manager_.has(codec_->id(desc),
                                           protobuf::PROMISCUOUS))
                    {
                        glog.is(DEBUG1) && glog << group(glog_in_group_)
                                              << "ignoring DCCL message for modem_id = "
                                              << dest << std::endl;
                    }
                    else if(manip_manager_.has(codec_->id(desc),
                                                protobuf::NO_DEQUEUE))
                    {
                        glog.is(DEBUG1) && glog << group(glog_in_group_)
                                                << "ignoring message:  " << desc->full_name()
                                                << " because NO_DEQUEUE manipulator set" << std::endl;
                    }
                    else
                    {
                        signal_receive(*decoded_message);
                    }
                }
            }
            catch(DCCLException& e)
            {
                glog.is(DEBUG1) && glog << group(glog_in_group_) << warn
                                        << "failed to decode " << e.what() << std::endl;
            }            
        }
    }
}


void goby::acomms::QueueManager::set_cfg(const protobuf::QueueManagerConfig& cfg)
{
    cfg_ = cfg;
    process_cfg();
}

void goby::acomms::QueueManager::merge_cfg(const protobuf::QueueManagerConfig& cfg)
{
    cfg_.MergeFrom(cfg);
    process_cfg();
}


void goby::acomms::QueueManager::process_cfg()
{
    modem_id_ = cfg_.modem_id();
    manip_manager_.clear();

    for(int i = 0, n = cfg_.message_entry_size(); i < n; ++i)
    {
        const google::protobuf::Descriptor* desc =
            goby::util::DynamicProtobufManager::find_descriptor(cfg_.message_entry(i).protobuf_name());
        if(desc)
        {
            
            add_queue(desc, cfg_.message_entry(i));
            
            for(int j = 0, m = cfg_.message_entry(i).manipulator_size(); j < m; ++j)
            {    
                manip_manager_.add(codec_->id(desc), cfg_.message_entry(i).manipulator(j));
            }
        }
        else
        {
            glog.is(DEBUG1) && glog << group(glog_push_group_) << warn << "No message by the name: " << cfg_.message_entry(i).protobuf_name() << " found, not setting Queue options for this type." << std::endl;
        }
    }
}

void goby::acomms::QueueManager::qsize(Queue* q)            
{
    protobuf::QueueSize size;
    size.set_dccl_id(codec_->id(q->descriptor()));
    size.set_size(q->size());
    signal_queue_size_change(size);
}

