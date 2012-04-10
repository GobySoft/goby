// Copyright 2009-2012 Toby Schneider (https://launchpad.net/~tes)
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
#include "goby/common/protobuf/acomms_option_extensions.pb.h"
#include "goby/common/protobuf/acomms_option_extensions.pb.h"
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

    goby::acomms::DCCLFieldCodecBase::register_wire_value_hook(
        goby::field.number(),
        boost::bind(&QueueManager::set_latest_metadata, this, _1, _2, _3));
}

void goby::acomms::QueueManager::add_queue(const google::protobuf::Descriptor* desc)
{
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
    unsigned dccl_id = codec_->id(desc);
    if(queues_.count(dccl_id))
    {
        std::stringstream ss;
        ss << "Queue: duplicate message specified for DCCL ID: " << dccl_id;
        throw QueueException(ss.str());
    }
    else
    {
        std::pair<std::map<unsigned, Queue>::iterator,bool> new_q_pair =
            queues_.insert(std::make_pair(dccl_id, Queue(desc, this)));

        Queue& new_q = (new_q_pair.first)->second;
        
        qsize(&new_q);
        
        glog.is(DEBUG1) && glog<< group(glog_out_group_) << "Added new queue: \n" << new_q << std::endl;
        
    }

}


void goby::acomms::QueueManager::do_work()
{
    typedef std::pair<unsigned, Queue> P;
    for(std::map<unsigned, Queue>::iterator it = queues_.begin(), n = queues_.end(); it != n; ++it)
    {
        std::vector<boost::shared_ptr<google::protobuf::Message> >expired_msgs =
            it->second.expire();

        BOOST_FOREACH(boost::shared_ptr<google::protobuf::Message> expire, expired_msgs)
        {
            signal_expire(*expire);
        }
        
    }
    
}


void goby::acomms::QueueManager::push_message(const google::protobuf::Message& dccl_msg)
{
    const google::protobuf::Descriptor* desc = dccl_msg.GetDescriptor();
    unsigned dccl_id = codec_->id(desc);
    
    if(!queues_.count(dccl_id))
        add_queue(dccl_msg.GetDescriptor());

    latest_meta_.Clear();
    
    latest_meta_.set_non_repeated_size(codec_->size(dccl_msg));

    codec_->run_hooks(dccl_msg);
    glog.is(DEBUG3) && glog << group(glog_push_group_) << "Message post hooks: " << latest_meta_ << std::endl;

    signal_out_route(&latest_meta_, dccl_msg);
    
    glog.is(DEBUG1) && glog << group(glog_push_group_) << msg_string(desc) << ": attempting to push message (destination: " << latest_meta_.dest() << ")" << std::endl;

    
    
    // loopback if set
    if(manip_manager_.has(dccl_id, protobuf::LOOPBACK))
    {
        glog.is(DEBUG1) && glog << group(glog_push_group_) << msg_string(desc) << ": LOOPBACK manipulator set, sending back to decoder" << std::endl;
        signal_receive(dccl_msg);
    }
    
    // no queue manipulator set
    if(manip_manager_.has(dccl_id, protobuf::NO_QUEUE))
    {
        glog.is(DEBUG1) && glog << group(glog_push_group_)
                                << msg_string(desc)
                                << ": not queuing: NO_QUEUE manipulator is set" << std::endl;
    }
    // message is to us, auto-loopback
    else if(latest_meta_.dest() == modem_id_)
    {
        glog.is(DEBUG1) && glog << group(glog_push_group_) << "Message is for us: using loopback, not physical interface" << std::endl;
        
        signal_receive(dccl_msg);

        // provide an ACK if desired 
        if((latest_meta_.has_ack_requested() && latest_meta_.ack_requested()) ||
           queues_[dccl_id].queue_message_options().ack())
        {
            protobuf::ModemTransmission ack_msg;
            ack_msg.set_time(goby::common::goby_time<uint64>());
            ack_msg.set_src(latest_meta_.dest());
            ack_msg.set_dest(latest_meta_.dest());
            ack_msg.set_type(protobuf::ModemTransmission::ACK);
            
            signal_ack(ack_msg, dccl_msg);
        }
    }
    // queue normally
    else 
    {
        if(!latest_meta_.has_time())
            latest_meta_.set_time(goby::common::goby_time<uint64>());
        
        // add the message
        boost::shared_ptr<google::protobuf::Message> new_dccl_msg(dccl_msg.New());
        new_dccl_msg->CopyFrom(dccl_msg);
        
        queues_[dccl_id].push_message(latest_meta_, new_dccl_msg);
        qsize(&queues_[dccl_id]);
    }
     
}
 

void goby::acomms::QueueManager::flush_queue(const protobuf::QueueFlush& flush)
{
    std::map<unsigned, Queue>::iterator it = queues_.find(flush.dccl_id());
    
    if(it != queues_.end())
    {
        it->second.flush();
        glog.is(DEBUG1) && glog << group(glog_out_group_) << msg_string(it->second.descriptor())
                                << ": flushed queue" << std::endl;
        qsize(&it->second);
    }    
    else
    {
        glog.is(DEBUG1) && glog << group(glog_out_group_) << warn << "Cannot find queue to flush: " << flush << std::endl;
    }
}

void goby::acomms::QueueManager::info_all(std::ostream* os) const
{
    *os << "=== Begin QueueManager [[" << queues_.size() << " queues total]] ===" << std::endl;
    for(std::map<unsigned, Queue>::const_iterator it = queues_.begin(), n = queues_.end(); it != n; ++it)
        info(it->second.descriptor(), os);
    *os << "=== End QueueManager ===";
}



void goby::acomms::QueueManager::info(const google::protobuf::Descriptor* desc, std::ostream* os) const
{

    std::map<unsigned, Queue>::const_iterator it = queues_.find(codec_->id(desc));

    if(it != queues_.end())
        it->second.info(os);
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
            while(winning_queue)
            {
                // new user frame (e.g. 32B)
                QueuedMessage next_user_frame = winning_queue->give_data(frame_number);

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
        
            // finally actually encode the message
            *data = codec_->encode_repeated<boost::shared_ptr<google::protobuf::Message> >(dccl_msgs);
            
        }
    }
    // only discipline the ACK value at the end, after all chances of making packet_ack_ = true are done
    msg->set_ack_requested(packet_ack_);
}


void goby::acomms::QueueManager::clear_packet()
{
    typedef std::pair<unsigned, Queue*> P;
    BOOST_FOREACH(const P& p, waiting_for_ack_)
        p.second->clear_ack_queue();
    
    waiting_for_ack_.clear();
    
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

    
    for(std::map<unsigned, Queue>::iterator it = queues_.begin(), n = queues_.end(); it != n; ++it)
    {
        Queue& q = it->second;
        
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

                std::list<boost::shared_ptr<google::protobuf::Message> > dccl_msgs =
                    codec_->decode_repeated<boost::shared_ptr<google::protobuf::Message> >(
                        modem_message.frame(frame_number));

                BOOST_FOREACH(boost::shared_ptr<google::protobuf::Message> decoded_message, dccl_msgs)
                {
                    latest_meta_.Clear();
                    codec_->run_hooks(*decoded_message);

                    // messages addressed to us on the link
                    if(modem_message.dest() == modem_id_)
                        signal_in_route(latest_meta_, *decoded_message);
                    
                    int32 dest = latest_meta_.dest();
                    
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

    for(int i = 0, n = cfg_.manipulator_entry_size(); i < n; ++i)
    {
        for(int j = 0, m = cfg_.manipulator_entry(i).manipulator_size(); j < m; ++j)
        {
            const google::protobuf::Descriptor* desc =
                goby::util::DynamicProtobufManager::descriptor_pool().FindMessageTypeByName(cfg_.manipulator_entry(i).protobuf_name());
            
            if(desc)
                manip_manager_.add(codec_->id(desc), cfg_.manipulator_entry(i).manipulator(j));
            else
                glog.is(DEBUG1) && glog << group(glog_push_group_) << warn << "No message by the name: " << cfg_.manipulator_entry(i).protobuf_name() << "found, not setting manipulators for this type." << std::endl;
            
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


void goby::acomms::QueueManager::set_latest_metadata(const boost::any& field_value,
                                                     const boost::any& wire_value,
                                                     const boost::any& extension_value)
{
    const google::protobuf::Message* options_msg = boost::any_cast<const google::protobuf::Message*>(extension_value);
                
    GobyFieldOptions field_options;
    field_options.CopyFrom(*options_msg);
    
    
    if(field_options.queue().is_dest())
    {
        int dest = BROADCAST_ID;
        if(wire_value.type() == typeid(int32))
            dest = boost::any_cast<int32>(wire_value);
        else if(wire_value.type() == typeid(int64))
            dest = boost::any_cast<int64>(wire_value);
        else if(wire_value.type() == typeid(uint32))
            dest = boost::any_cast<uint32>(wire_value);
        else if(wire_value.type() == typeid(uint64))
            dest = boost::any_cast<uint64>(wire_value);
        else
            throw(QueueException("Invalid type " + std::string(wire_value.type().name()) + " given for (queue_field).is_dest. Expected integer type"));
                    
        goby::glog.is(DEBUG2) &&
            goby::glog << group(glog_push_group_) << "setting dest to " << dest << std::endl;
                
        latest_meta_.set_dest(dest);
    }
    else if(field_options.queue().is_src())
    {
        int src = BROADCAST_ID;
        if(wire_value.type() == typeid(int32))
            src = boost::any_cast<int32>(wire_value);
        else if(wire_value.type() == typeid(int64))
            src = boost::any_cast<int64>(wire_value);
        else if(wire_value.type() == typeid(uint32))
            src = boost::any_cast<uint32>(wire_value);
        else if(wire_value.type() == typeid(uint64))
            src = boost::any_cast<uint64>(wire_value);
        else
            throw(QueueException("Invalid type " + std::string(wire_value.type().name()) + " given for (queue_field).is_src. Expected integer type"));

        goby::glog.is(DEBUG2) &&
            goby::glog << group(glog_push_group_) <<  "setting source to " << src << std::endl;
                
        latest_meta_.set_src(src);

    }
    else if(field_options.queue().is_time())
    {
        if(field_value.type() == typeid(uint64)) 
            latest_meta_.set_time(boost::any_cast<uint64>(field_value));
        else if(field_value.type() == typeid(double))
            latest_meta_.set_time(static_cast<uint64>(boost::any_cast<double>(field_value))*1e6);
        else if(field_value.type() == typeid(std::string))
            latest_meta_.set_time(as<uint64>(as<boost::posix_time::ptime>(boost::any_cast<std::string>(field_value))));
        else
            throw(QueueException("Invalid type " + std::string(field_value.type().name()) + " given for (goby.field).queue.is_time. Expected uint64 contained microseconds since UNIX, double containing seconds since UNIX or std::string containing as<std::string>(boost::posix_time::ptime)"));

        goby::glog.is(DEBUG2) &&
            goby::glog << group(glog_push_group_) <<  "setting time to " << as<boost::posix_time::ptime>(latest_meta_.time()) << std::endl;            
        
    }
                
}
