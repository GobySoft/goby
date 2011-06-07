// copyright 2009 t. schneider tes@mit.edu 
//
// this file is part of the Queue Library (libqueue),
// the goby-acomms message queue manager. goby-acomms is a collection of 
// libraries for acoustic underwater networking
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

#include <boost/foreach.hpp>

#include "goby/util/logger.h"
#include "goby/util/time.h"
#include "goby/util/binary.h"

#include "goby/acomms/dccl.h"
#include "goby/protobuf/dccl_option_extensions.pb.h"
#include "goby/protobuf/queue_option_extensions.pb.h"
#include "goby/util/logger.h"

#include "queue_manager.h"
#include "queue_constants.h"


using goby::glog;

int goby::acomms::QueueManager::modem_id_ = 0;
boost::shared_ptr<goby::acomms::QueueManager> goby::acomms::QueueManager::inst_;
goby::acomms::protobuf::ModemDataTransmission goby::acomms::QueueManager::latest_data_msg_;

goby::acomms::QueueManager::QueueManager()
    : packet_ack_(0),
      packet_dest_(BROADCAST_ID)
{
    goby::glog.add_group("queue.push", util::Colors::lt_cyan, "stack push - outgoing messages (goby_queue)");
    goby::glog.add_group("queue.pop",  util::Colors::lt_green, "stack pop - outgoing messages (goby_queue)");
    goby::glog.add_group("queue.priority",  util::Colors::yellow, "priority contest (goby_queue)");
    goby::glog.add_group("queue.out",  util::Colors::cyan, "outgoing queuing messages (goby_queue)");
    goby::glog.add_group("queue.in",  util::Colors::green, "incoming queuing messages (goby_queue)");

    goby::acomms::DCCLFieldCodecBase::register_wire_value_hook(
        make_hook_key(queue::is_dest.number(), goby::acomms::protobuf::HookKey::WIRE_VALUE),
        boost::bind(&QueueManager::set_latest_dest, this, _1, _2));

    goby::acomms::DCCLFieldCodecBase::register_wire_value_hook(
        make_hook_key(queue::is_src.number(), goby::acomms::protobuf::HookKey::WIRE_VALUE),
        boost::bind(&QueueManager::set_latest_src, this, _1, _2));

    goby::acomms::DCCLFieldCodecBase::register_wire_value_hook(
        make_hook_key(queue::is_time.number(), goby::acomms::protobuf::HookKey::FIELD_VALUE),
        boost::bind(&QueueManager::set_latest_time, this, _1, _2));
}

void goby::acomms::QueueManager::add_queue(const google::protobuf::Message& msg)
{
    goby::acomms::DCCLCodec* codec = goby::acomms::DCCLCodec::get();
    const google::protobuf::Descriptor* desc = msg.GetDescriptor();

    protobuf::QueueKey key;
    key.set_type(protobuf::QUEUE_DCCL);
    key.set_id(desc->options().GetExtension(dccl::id));
    
    try
    {
        //validate with DCCL first
        codec->validate(desc);
    }
    catch(DCCLException& e)
    {
        throw(queue_exception("could not create queue for message: " + desc->full_name() + " because it failed DCCL validation: " + e.what()));
    }

    protobuf::QueueConfig queue_cfg;
    queue_cfg.set_name(desc->full_name());
        
    if(!desc->options().HasExtension(queue::ack))
        glog.is(verbose) && glog << queue_cfg.name() << ": using default value of " << queue_cfg.ack() << " for queue::ack" << std::endl;
    else
        queue_cfg.set_ack(desc->options().GetExtension(queue::ack));    

    if(!desc->options().HasExtension(queue::blackout_time))
        glog.is(verbose) && glog << queue_cfg.name() << ": " << "using default value of " << queue_cfg.blackout_time() << " for queue::blackout_time" << std::endl;
    else
        queue_cfg.set_blackout_time(desc->options().GetExtension(queue::blackout_time));

    if(!desc->options().HasExtension(queue::max_queue))
        glog.is(verbose) && glog << queue_cfg.name() << ": "  << "using default value of " << queue_cfg.max_queue() << " for queue::max_queue" << std::endl;
    else
        queue_cfg.set_max_queue(desc->options().GetExtension(queue::max_queue));    
        
    if(!desc->options().HasExtension(queue::newest_first))
        glog.is(verbose) && glog << queue_cfg.name() << ": " << "using default value of " << queue_cfg.newest_first() << " for queue::newest_first" << std::endl;
    else
        queue_cfg.set_newest_first(desc->options().GetExtension(queue::newest_first));    

    if(!desc->options().HasExtension(queue::ttl))
        glog.is(verbose) && glog << queue_cfg.name() << ": "  << "using default value of " << queue_cfg.ttl() << " for queue::ttl" << std::endl;
    else
        queue_cfg.set_ttl(desc->options().GetExtension(queue::ttl));    

    if(!desc->options().HasExtension(queue::value_base))
        glog.is(verbose) && glog  << queue_cfg.name() << ": "  << "using default value of " << queue_cfg.value_base() << " for queue::value_base" << std::endl;
    else
        queue_cfg.set_value_base(desc->options().GetExtension(queue::value_base));    

    queue_cfg.mutable_key()->CopyFrom(key);
    // add the newly generated queue
    add_queue(queue_cfg);
}


void goby::acomms::QueueManager::add_queue(const protobuf::QueueConfig& cfg)
{
    Queue q(cfg);
    
    if(queues_.count(cfg.key()))
    {
        std::stringstream ss;
        ss << "Queue: duplicate key specified for key: " << cfg.key();
        throw queue_exception(ss.str());
    }
    else if((q.cfg().key().id() > MAX_ID || q.cfg().key().id() < MIN_ID) && q.cfg().key().type() == protobuf::QUEUE_DCCL)
    {
        std::stringstream ss;
        ss << "Queue: key (" << cfg.key() << ") is out of bounds for use with libqueue. Use a id from " << MIN_ID << " to " << MAX_ID;
        throw queue_exception(ss.str());
    }
    else
    {
        std::pair<std::map<goby::acomms::protobuf::QueueKey, Queue>::iterator,bool> new_q_pair =
            queues_.insert(std::make_pair(cfg.key(), q));

        qsize(&((new_q_pair.first)->second));
    }

    glog.is(debug1) && glog<< group("queue.out") << "added new queue: \n" << q << std::endl;
    
}

void goby::acomms::QueueManager::do_work()
{
    typedef std::pair<const protobuf::QueueKey, Queue> P;
    for(std::map<protobuf::QueueKey, Queue>::iterator it = queues_.begin(), n = queues_.end(); it != n; ++it)
    {
        std::vector<boost::shared_ptr<google::protobuf::Message> >expired_msgs =
            it->second.expire();

        BOOST_FOREACH(boost::shared_ptr<google::protobuf::Message> expire, expired_msgs)
        {
            signal_expire(*expire);
        }
        
    }
    
}

// for CCL
void goby::acomms::QueueManager::push_message(const protobuf::ModemDataTransmission& data_msg)
{
    // message is to us, auto-loopback
    if(data_msg.base().dest() == modem_id_)
    {
        glog.is(debug1) && glog<< group("queue.out") << "outgoing message is for us: using loopback, not physical interface" << std::endl;
        
        handle_modem_receive(data_msg);
    }
    else if(queues_.count(data_msg.queue_key()))
    {
        queues_[data_msg.queue_key()].push_message(data_msg);
        qsize(&queues_[data_msg.queue_key()]);
    }
    else
    {
        std::stringstream ss;
        ss << "no queue for key: " << data_msg.queue_key();
        throw queue_exception(ss.str());
    }
}

void goby::acomms::QueueManager::push_message(const google::protobuf::Message& dccl_msg)
{
    const google::protobuf::Descriptor* desc = dccl_msg.GetDescriptor();
    
    protobuf::QueueKey key;
    key.set_type(protobuf::QUEUE_DCCL);
    key.set_id(desc->options().GetExtension(dccl::id));
    
    if(!queues_.count(key))
        add_queue(dccl_msg);
    
    boost::shared_ptr<google::protobuf::Message> new_dccl_msg(dccl_msg.New());
    new_dccl_msg->CopyFrom(dccl_msg);

    latest_data_msg_.Clear();
    
    goby::acomms::DCCLCodec* codec = goby::acomms::DCCLCodec::get();
    latest_data_msg_.mutable_data()->resize(codec->size(&dccl_msg));
    codec->run_hooks(&dccl_msg);
    glog.is(debug2) && glog << "post hooks: " << latest_data_msg_ << std::endl;
    
    // message is to us, auto-loopback
    if(latest_data_msg_.base().dest() == modem_id_)
    {
        glog.is(debug1) && glog<< group("queue.out") << "outgoing message is for us: using loopback, not physical interface" << std::endl;
        
        signal_receive(*new_dccl_msg);
    }
    else
    {
        if(!latest_data_msg_.base().has_time())
            latest_data_msg_.mutable_base()->set_time(util::as<std::string>(goby::util::goby_time()));
        
        // add the message
        queues_[key].push_message(latest_data_msg_, new_dccl_msg);
        qsize(&queues_[key]);
    }
     
}
 

void goby::acomms::QueueManager::flush_queue(const protobuf::QueueFlush& flush)
{
    std::map<goby::acomms::protobuf::QueueKey, Queue>::iterator it = queues_.find(flush.key());
    
    if(it != queues_.end())
    {
        it->second.flush();
        glog.is(verbose) && glog << group("q_out") <<  " flushed queue: " << flush << std::endl;
        qsize(&it->second);
    }    
    else
    {
        glog.is(verbose) && glog << group("q_out") << warn << " cannot find queue to flush: " << flush << std::endl;
    }
}


std::string goby::acomms::QueueManager::summary() const
{
    std::string s;
    typedef std::pair<const protobuf::QueueKey, Queue> P;
    BOOST_FOREACH(const P& p, queues_)
        s += p.second.summary();

    return s;
}

    
std::ostream& goby::acomms::operator<< (std::ostream& out, const QueueManager& d)
{
    out << d.summary();
    return out;
}

// finds and publishes outgoing data for the modem driver
// first query every Queue for its priority data using
// priority_values(priority, last_send_time)
// priority_values returns false if that object has no data to give
// (either no data at all, or in blackout interval) 
// thus, from all the priority values that return true, pick the one with the lowest
// priority value, or given a tie, pick the one with the oldest last_send_time
void goby::acomms::QueueManager::handle_modem_data_request(const protobuf::ModemDataRequest& request_msg, protobuf::ModemDataTransmission* data_msg)
{
    data_msg->mutable_base()->set_src(modem_id_);
    if(request_msg.frame() == 0)
    {
        clear_packet();
        data_msg->mutable_base()->set_dest(request_msg.base().dest());
    }
    else
    {
        if(packet_ack_)
            data_msg->set_ack_requested(packet_ack_);

        data_msg->mutable_base()->set_dest(packet_dest_);    
    }
    // first (0th) user-frame
    Queue* winning_queue = find_next_sender(request_msg, *data_msg, true);
    
    // no data at all for this frame ... :(
    if(!winning_queue)
    {
        data_msg->set_ack_requested(packet_ack_);
        data_msg->mutable_base()->set_dest(packet_dest_);
        glog.is(debug1) && glog<< group("queue.out") << "no data found. sending blank to firmware" 
                               << ": " << *data_msg << std::endl; 
    }
    else if(winning_queue->cfg().key().type() == protobuf::QUEUE_CCL)
    {
        // new user frame (e.g. 32B)
        QueuedMessage next_user_frame = winning_queue->give_data(request_msg);

        glog.is(debug1) && glog << group("queue.out") << "sending data to firmware from: "
                                << winning_queue->cfg().name()
                                << ": " << next_user_frame.encoded_msg << std::endl;

        // not DCCL, copy the msg and we are done
        data_msg->CopyFrom(next_user_frame.encoded_msg);
    }
    else if(winning_queue->cfg().key().type() == protobuf::QUEUE_DCCL)
    {
        goby::acomms::DCCLCodec* codec = goby::acomms::DCCLCodec::get();
        std::list<boost::shared_ptr<google::protobuf::Message> > dccl_msgs;
        while(winning_queue)
        {
            // new user frame (e.g. 32B)
            QueuedMessage next_user_frame = winning_queue->give_data(request_msg);

            glog.is(debug1) && glog << group("queue.out") << "sending data to firmware from: "
                                    << winning_queue->cfg().name()
                                    << ": "<< *(next_user_frame.dccl_msg) << std::endl;
            
            //
            // ACK
            // 
            // insert ack if desired
            if(next_user_frame.encoded_msg.ack_requested())
            {
                glog.is(debug2) &&
                    glog << "inserting ack for queue: " << *winning_queue << std::endl;
                waiting_for_ack_.insert(std::pair<unsigned, Queue*>(request_msg.frame(),
                                                                    winning_queue));
            }
            else
            {
                glog.is(debug2) &&
                    glog << "no ack, popping from queue: " << *winning_queue << std::endl;
                if(!winning_queue->pop_message(request_msg.frame()))
                    glog.is(warn) &&
                        glog << "failed to pop from queue: " << *winning_queue << std::endl;

                qsize(winning_queue); // notify change in queue size
            }

            // if an ack been set, do not unset these
            if (packet_ack_ == false) packet_ack_ = next_user_frame.encoded_msg.ack_requested();
    

            //
            // DEST
            // 
            // update destination address
            if(request_msg.frame() == 0)
            {
                // discipline the destination of the packet if initially unset
                if(data_msg->base().dest() == QUERY_DESTINATION_ID)
                    data_msg->mutable_base()->set_dest(next_user_frame.encoded_msg.base().dest());

                if(packet_dest_ == BROADCAST_ID)
                    packet_dest_ = data_msg->base().dest();
            }
            else
            {
                data_msg->mutable_base()->set_dest(packet_dest_);
            }

            //
            // DCCL
            //
            // // e.g. 32B
            // std::string new_data = next_user_frame.data();
        
            // // insert the size of the next field (e.g. 32B) right after the header
            // std::string frame_size(USER_FRAME_NEXT_SIZE_BYTES,
            //                        static_cast<char>((next_user_frame.data().size()-DCCL_NUM_HEADER_BYTES)));
            // new_data.insert(DCCL_NUM_HEADER_BYTES, frame_size);
        
            // append without the CCL ID (old size + 31B)

            dccl_msgs.push_back(next_user_frame.dccl_msg);
            
            unsigned repeated_size_bytes = codec->size_repeated(dccl_msgs);
            
            glog.is(debug2) && glog << "Size repeated " << repeated_size_bytes << std::endl;
            data_msg->mutable_data()->resize(repeated_size_bytes);
        
            // if remaining bytes is greater than 0, we have a chance of adding another user-frame
            if((request_msg.max_bytes() - data_msg->data().size()) > 0)
            {
                // fetch the next candidate
                winning_queue = find_next_sender(request_msg, *data_msg, false);
            }
            else
            {
                break;
            }
        }
        
        data_msg->set_ack_requested(packet_ack_);
        // finally actually encode the message
        *(data_msg->mutable_data()) = codec->encode_repeated(dccl_msgs);
 
    }
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



goby::acomms::Queue* goby::acomms::QueueManager::find_next_sender(const protobuf::ModemDataRequest& request_msg, const protobuf::ModemDataTransmission& data_msg, bool first_user_frame)
{   
// competition between variable about who gets to send
    double winning_priority;
    boost::posix_time::ptime winning_last_send_time;

    Queue* winning_queue = 0;
    
    glog.is(verbose) && glog<< group("queue.priority") << "starting priority contest\n"
                            << "requesting: " << request_msg << "\n"
                            << "have " << data_msg.data().size() << "/" << request_msg.max_bytes() << "B: " << data_msg << std::endl;

    
    for(std::map<protobuf::QueueKey, Queue>::iterator it = queues_.begin(), n = queues_.end(); it != n; ++it)
    {
        Queue& oq = it->second;
        
        // // encode on demand
        // if(oq.cfg().key().type() == protobuf::QUEUE_DCCL &&
        //    manip_manager_.has(oq.cfg().key().id(), protobuf::MessageFile::ON_DEMAND) &&
        //    (!oq.size() || oq.newest_msg_time() + ON_DEMAND_SKEW < util::goby_time()))
        // {
        //     protobuf::ModemDataTransmission data_msg;
        //     data_msg.mutable_queue_key()->CopyFrom(it->first);
        //     signal_data_on_demand(request_msg, &data_msg);
        //     push_message(data_msg);
        // }
        
        double priority;
        boost::posix_time::ptime last_send_time;
        if(oq.priority_values(priority, last_send_time, request_msg, data_msg))
        {
            // no winner, better winner, or equal & older winner
            // AND not CCL when not the first user-frame
            if((!winning_queue || priority > winning_priority ||
                (priority == winning_priority && last_send_time < winning_last_send_time))
               && !(oq.cfg().key().type() == protobuf::QUEUE_CCL && !first_user_frame))
            {
                winning_priority = priority;
                winning_last_send_time = last_send_time;
                winning_queue = &oq;
            }
        }
    }

    glog.is(verbose) && glog<< group("queue.priority") << "\t"
                            << "all other queues have no messages" << std::endl;

    if(winning_queue)
    {
        glog.is(verbose) && glog<< group("queue.priority") << winning_queue->cfg().name()
                                << " has highest priority." << std::endl;
    }
    else
    {
        glog.is(verbose) && glog<< group("queue.priority") 
                                << "ending priority contest" << std::endl;    
    }
    
    return winning_queue;
}    


void goby::acomms::QueueManager::handle_modem_ack(const protobuf::ModemDataAck& ack_msg)
{
    
    if(ack_msg.base().dest() != modem_id_)
    {
        glog.is(warn) && glog<< group("queue.in")
                             << "ignoring ack for modem_id = " << ack_msg.base().dest() << std::endl;
        return;
    }
    else if(!waiting_for_ack_.count(ack_msg.frame()))
    {
        glog.is(debug1) && glog<< group("queue.in")
                               << "got ack but we were not expecting one" << std::endl;
        return;
    }
    else
    {
        
        // got an ack, let's pop this!
        glog.is(verbose) && glog<< group("queue.in") << "received ack for this id" << std::endl;
        
        
        std::multimap<unsigned, Queue *>::iterator it = waiting_for_ack_.find(ack_msg.frame());
        while(it != waiting_for_ack_.end())
        {
            Queue* oq = it->second;
            
            boost::shared_ptr<google::protobuf::Message> removed_msg;
            if(!oq->pop_message_ack(ack_msg.frame(), removed_msg))
            {
                glog.is(warn) && glog<< group("queue.in") 
                                     << "failed to pop message from "
                                     << oq->cfg().name() << std::endl;
            }
            else
            {
                qsize(oq);
                //ack_msg.mutable_orig_msg()->mutable_queue_key()->CopyFrom(oq->cfg().key());
                signal_ack(ack_msg, *removed_msg);
                
            }

            glog.is(verbose) && glog<< group("queue.in") << ack_msg << std::endl;
            
            waiting_for_ack_.erase(it);
            
            it = waiting_for_ack_.find(ack_msg.frame());
        }
    }
    
    return;    
}


// parses and publishes incoming data
// by matching the variableID field with the variable specified
// in a "receive = " line of the configuration file
void goby::acomms::QueueManager::handle_modem_receive(const protobuf::ModemDataTransmission& m)
{
    // copy so we can modify in various ways 
    protobuf::ModemDataTransmission message = m;
    
    glog.is(verbose) && glog<< group("queue.in") << "received message"
                            << ": " << message << std::endl;

    std::string data = message.data();
    if(data.size() < HEAD_CCL_ID_SIZE / BITS_IN_BYTE)
    {
        glog.is(warn) && glog << "message is too small!" << std::endl;
        return;
    }
    
    int ccl_id = data[0];
    // check for queue_dccl type
    if(ccl_id == DCCL_CCL_HEADER)
    {
        goby::acomms::DCCLCodec* codec = goby::acomms::DCCLCodec::get();
        std::list<boost::shared_ptr<google::protobuf::Message> > dccl_msgs =
            codec->decode_repeated(data);
        
        BOOST_FOREACH(boost::shared_ptr<google::protobuf::Message> msg, dccl_msgs)
        {
            latest_data_msg_.Clear();
            codec->run_hooks(msg.get());

            int32 dest = latest_data_msg_.base().dest();
            if(dest != BROADCAST_ID && dest != modem_id_)
            {
                glog.is(warn) && glog << group("queue.in")
                                      << "ignoring DCCL message for modem_id = "
                                      << message.base().dest() << std::endl;
            }
            else
            {
                signal_receive(*msg);
            }
        }
        
    }
    // check for ccl type
    else
    {
        if(message.base().dest() != BROADCAST_ID && message.base().dest() != modem_id_)
        {
            glog.is(warn) && glog << group("queue.in") << "ignoring CCL message for modem_id = "
                                  << message.base().dest() << std::endl;
            return;
        }

        protobuf::QueueKey key;
        key.set_type(protobuf::QUEUE_CCL);
        key.set_id(ccl_id);
        
        std::map<protobuf::QueueKey, Queue>::iterator it = queues_.find(key);
        
        if (it != queues_.end())
        {
            message.mutable_queue_key()->CopyFrom(key);
            signal_receive_ccl(message);
        }
        else
        {
            glog.is(warn) && glog << group("queue.in") << "incoming data string is not for us (not DCCL or known CCL)." << std::endl;
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
    queues_.clear();
    waiting_for_ack_.clear();
    
    for(int i = 0, n = cfg_.queue_size(); i < n; ++i)
        add_queue(cfg_.queue(i));

    modem_id_ = cfg_.modem_id();
}

void goby::acomms::QueueManager::qsize(Queue* q)            
{
    protobuf::QueueSize size;
    size.mutable_key()->CopyFrom(q->cfg().key());
    size.set_size(q->size());
    signal_queue_size_change(size);
}
