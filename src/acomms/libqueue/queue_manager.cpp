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

#include "goby/acomms/xml/xml_parser.h"
#include "goby/util/logger.h"
#include "goby/util/time.h"
#include "goby/util/binary.h"

#include "queue_manager.h"
#include "queue_constants.h"
#include "queue_xml_callbacks.h"

int goby::acomms::QueueManager::modem_id_ = 0;

goby::acomms::QueueManager::QueueManager(std::ostream* log /* =0 */)
    : log_(log),
      packet_ack_(0),
      packet_dest_(BROADCAST_ID)
{}

void goby::acomms::QueueManager::add_queue(const protobuf::QueueConfig& cfg)
{
    Queue q(cfg, log_, modem_id_);
    
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

    
    
    if(log_) *log_<< group("q_out") << "added new queue: \n" << q << std::endl;
    
}

std::set<unsigned> goby::acomms::QueueManager::add_xml_queue_file(const std::string& xml_file)
{
    std::vector<protobuf::QueueConfig> cfgs;

    // Register handlers for XML parsing
    QueueContentHandler content(cfgs);
    QueueErrorHandler error;
    // instantiate a parser for the xml message files
    XMLParser parser(content, error);
    std::set<unsigned> added_ids;
    
    parser.parse(xml_file);
    BOOST_FOREACH(const protobuf::QueueConfig& c, cfgs)
    {
        add_queue(c);
        added_ids.insert(c.key().id());
    }
    
    return added_ids;
}

void goby::acomms::QueueManager::do_work()
{
    typedef std::pair<const protobuf::QueueKey, Queue> P;
    for(std::map<protobuf::QueueKey, Queue>::iterator it = queues_.begin(), n = queues_.end(); it != n; ++it)
    {
        std::vector<protobuf::ModemDataTransmission> expired_msgs = it->second.expire();

        protobuf::ModemDataExpire expire;
        protobuf::ModemDataTransmission* data_msg = expire.mutable_orig_msg();
        BOOST_FOREACH(*data_msg, expired_msgs)
        {
            //expire.mutable_orig_msg()->mutable_queue_key()->CopyFrom(it->first);
            signal_expire(expire);
        }
        
    }
    
}

void goby::acomms::QueueManager::push_message(const protobuf::ModemDataTransmission& data_msg)
{
    // message is to us, auto-loopback
    if(data_msg.base().dest() == modem_id_)
    {
        if(log_) *log_<< group("q_out") << "outgoing message is for us: using loopback, not physical interface" << std::endl;
        
        handle_modem_receive(data_msg);
    }
// we have a queue with this key, so push message for sending
    else if(queues_.count(data_msg.queue_key()))
    {
        if(data_msg.queue_key().type() == protobuf::QUEUE_DCCL && manip_manager_.has(data_msg.queue_key().id(), protobuf::MessageFile::NO_QUEUE))
        {
            if(log_) *log_ << group("q_out") << "not queuing DCCL ID: " << data_msg.queue_key().id() << "; NO_QUEUE manipulator is set" << std::endl;
        }
        else
        {
            queues_[data_msg.queue_key()].push_message(data_msg);
            qsize(&queues_[data_msg.queue_key()]);
        }        

        if(data_msg.queue_key().type() == protobuf::QUEUE_DCCL && manip_manager_.has(data_msg.queue_key().id(), protobuf::MessageFile::LOOPBACK))
        {
            if(log_) *log_ << group("q_out") << data_msg.queue_key() << " LOOPBACK manipulator set, sending back to decoder" << std::endl;
            handle_modem_receive(data_msg);
        }        
    }
    else
    {
        std::stringstream ss;
        ss << "no queue for key: " << data_msg.queue_key();
        throw queue_exception(ss.str());
    }
}


void goby::acomms::QueueManager::flush_queue(const protobuf::QueueFlush& flush)
{
    std::map<goby::acomms::protobuf::QueueKey, Queue>::iterator it = queues_.find(flush.key());
    
    if(it != queues_.end())
    {
        it->second.flush();
        if(log_) *log_ << group("q_out") <<  " flushed queue: " << flush << std::endl;
        qsize(&it->second);
    }    
    else
    {
        if(log_) *log_ << group("q_out") << warn << " cannot find queue to flush: " << flush << std::endl;
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
        if(log_) *log_<< group("q_out") << "no data found. sending blank to firmware" 
                      << ": " << *data_msg << std::endl; 
    }
    else
    {
        stitch_recursive(request_msg, data_msg, winning_queue);
    }
}

bool goby::acomms::QueueManager::stitch_recursive(const protobuf::ModemDataRequest& request_msg, protobuf::ModemDataTransmission* complete_data_msg, Queue* winning_queue)
{
    const unsigned CCL_ID_BYTES = HEAD_CCL_ID_SIZE / BITS_IN_BYTE;
    
    // new user frame (e.g. 32B)
    protobuf::ModemDataTransmission next_data_msg = winning_queue->give_data(request_msg);

    if(log_) *log_<< group("q_out") << "sending data to firmware from: "
                  << winning_queue->cfg().name() 
                  << ": " << next_data_msg << std::endl;

    //
    // ACK
    // 
    // insert ack if desired
    if(next_data_msg.ack_requested())
        waiting_for_ack_.insert(std::pair<unsigned, Queue*>(request_msg.frame(), winning_queue));
    else
    {
        winning_queue->pop_message(request_msg.frame());
        qsize(winning_queue); // notify change in queue size
    }

    // if an ack been set, do not unset these
    if (packet_ack_ == false) packet_ack_ = next_data_msg.ack_requested();
    

    //
    // DEST
    // 
    // update destination address
    if(request_msg.frame() == 0)
    {
        // discipline the destination of the packet if initially unset
        if(complete_data_msg->base().dest() == QUERY_DESTINATION_ID)
            complete_data_msg->mutable_base()->set_dest(next_data_msg.base().dest());

        if(packet_dest_ == BROADCAST_ID)
            packet_dest_ = complete_data_msg->base().dest();
    }
    else
    {
        complete_data_msg->mutable_base()->set_dest(packet_dest_);
    }

    //
    // DCCL
    //
    if(winning_queue->cfg().key().type() == protobuf::QUEUE_DCCL)
    {   
        // e.g. 32B
        std::string new_data = next_data_msg.data();
        
        // insert the size of the next field (e.g. 32B) right after the header
        std::string frame_size(USER_FRAME_NEXT_SIZE_BYTES,
                               static_cast<char>((next_data_msg.data().size()-DCCL_NUM_HEADER_BYTES)));
        new_data.insert(DCCL_NUM_HEADER_BYTES, frame_size);
        
        // append without the CCL ID (old size + 31B)
        *(complete_data_msg->mutable_data()) += new_data.substr(CCL_ID_BYTES);
        
        bool is_last_user_frame = true;
        // if remaining bytes is greater than the DCCL header, we have a chance of adding another user-frame
        if((request_msg.max_bytes() - complete_data_msg->data().size()) > DCCL_NUM_HEADER_BYTES
           && winning_queue->cfg().key().type() != protobuf::QUEUE_CCL)
        {
            // fetch the next candidate
            winning_queue = find_next_sender(request_msg, *complete_data_msg, false);
            if(winning_queue) is_last_user_frame = false;
        }
        
        if(!is_last_user_frame)
        {
            replace_header(false, complete_data_msg, next_data_msg, new_data);
            return stitch_recursive(request_msg, complete_data_msg, winning_queue);
        }
        else
        {
            replace_header(true, complete_data_msg, next_data_msg, new_data);
            // add the CCL ID back on to the message (e.g. 33B)
            complete_data_msg->mutable_data()->insert(0, std::string(1, DCCL_CCL_HEADER));
            // remove the size of the next field from the last user-frame (e.g. 32B).
            complete_data_msg->mutable_data()->erase(complete_data_msg->data().size()-new_data.size()+DCCL_NUM_HEADER_BYTES, USER_FRAME_NEXT_SIZE_BYTES);
            // set the ack to conform to the entire message
            complete_data_msg->set_ack_requested(packet_ack_);
            
            return true;
        }
    }
    //
    // CCL
    //
    else
    {
        // not DCCL, copy the msg and we are done
        complete_data_msg->CopyFrom(next_data_msg);
        return true;
    }
}

void goby::acomms::QueueManager::replace_header(bool is_last_user_frame, protobuf::ModemDataTransmission* data_msg, const protobuf::ModemDataTransmission& next_data_msg, const std::string& new_data)
{
    // decode the header so that we can modify the flags
    DCCLHeaderDecoder head_decoder(new_data);

    // don't put the multimessage flag on the last user-frame
    head_decoder[HEAD_MULTIMESSAGE_FLAG] =
        (!is_last_user_frame) ? true : false;
    // put the broadcast flag on if needed 
    head_decoder[HEAD_BROADCAST_FLAG] =
        (next_data_msg.base().dest() == BROADCAST_ID) ? true : false;

    // re-encode the header
    DCCLHeaderEncoder head_encoder(head_decoder.get());

    // replace the header without the CCL ID
    const unsigned CCL_ID_BYTES = HEAD_CCL_ID_SIZE / BITS_IN_BYTE;
    data_msg->mutable_data()->replace(data_msg->data().size()-new_data.size()+CCL_ID_BYTES,
                                      head_encoder.str().size()-CCL_ID_BYTES,
                                      head_encoder.str().substr(CCL_ID_BYTES));
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
    
    if(log_) *log_<< group("priority") << "starting priority contest\n"
                  << "requesting: " << request_msg << "\n"
                  << "have " << data_msg.data().size() << "/" << request_msg.max_bytes() << "B: " << data_msg << std::endl;

    
    for(std::map<protobuf::QueueKey, Queue>::iterator it = queues_.begin(), n = queues_.end(); it != n; ++it)
    {
        Queue& oq = it->second;
        
        // encode on demand
        if(oq.cfg().key().type() == protobuf::QUEUE_DCCL &&
           manip_manager_.has(oq.cfg().key().id(), protobuf::MessageFile::ON_DEMAND) &&
           (!oq.size() || oq.newest_msg_time() + ON_DEMAND_SKEW < util::goby_time()))
        {
            protobuf::ModemDataTransmission data_msg;
            data_msg.mutable_queue_key()->CopyFrom(it->first);
            signal_data_on_demand(request_msg, &data_msg);
            push_message(data_msg);
        }
        
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

    if(log_) *log_<< group("priority") << "\t"
                  << "all other queues have no messages" << std::endl;

    if(winning_queue)
    {
        if(log_) *log_<< group("priority") << winning_queue->cfg().name()
                      << " has highest priority." << std::endl;
    }
    
    return winning_queue;
}    


void goby::acomms::QueueManager::handle_modem_ack(const protobuf::ModemDataAck& orig_ack_msg)
{
    protobuf::ModemDataAck ack_msg(orig_ack_msg);
    
    if(ack_msg.base().dest() != modem_id_)
    {
        if(log_) *log_<< group("q_in") << warn
                      << "ignoring ack for modem_id = " << ack_msg.base().dest() << std::endl;
        return;
    }
    else if(!waiting_for_ack_.count(ack_msg.frame()))
    {
        if(log_) *log_<< group("q_in")
                      << "got ack but we were not expecting one" << std::endl;
        return;
    }
    else
    {
        
        // got an ack, let's pop this!
        if(log_) *log_<< group("q_in") << "received ack for this id" << std::endl;
        
        
        std::multimap<unsigned, Queue *>::iterator it = waiting_for_ack_.find(ack_msg.frame());
        while(it != waiting_for_ack_.end())
        {
            Queue* oq = it->second;

            protobuf::ModemDataTransmission* removed_msg = ack_msg.mutable_orig_msg();
            if(!oq->pop_message_ack(ack_msg.frame(), removed_msg))
            {
                if(log_) *log_<< group("q_in") << warn
                              << "failed to pop message from "
                              << oq->cfg().name() << std::endl;
            }
            else
            {
                qsize(oq);
                //ack_msg.mutable_orig_msg()->mutable_queue_key()->CopyFrom(oq->cfg().key());
                signal_ack(ack_msg);
                
            }

            if(log_) *log_<< group("q_in") << ack_msg << std::endl;
            
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
    
    if(log_) *log_<< group("q_in") << "received message"
                  << ": " << message << std::endl;

    std::string data = message.data();
    if(data.size() < DCCL_NUM_HEADER_BYTES)
        return;
    
    DCCLHeaderDecoder head_decoder(data);
    int ccl_id = head_decoder[HEAD_CCL_ID];

    // check for queue_dccl type
    if(ccl_id == DCCL_CCL_HEADER)
    {
        unstitch_recursive(&data, &message);
    }
    // check for ccl type
    else
    {
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
            if(log_) *log_<< group("q_in") << warn << "incoming data string is not for us (not DCCL or known CCL)." << std::endl;
        }
    }
}

bool goby::acomms::QueueManager::unstitch_recursive(std::string* data, protobuf::ModemDataTransmission* data_msg)
{
    unsigned original_dest = data_msg->base().dest();
    DCCLHeaderDecoder head_decoder(*data);
    bool multimessage_flag = head_decoder[HEAD_MULTIMESSAGE_FLAG];
    bool broadcast_flag = head_decoder[HEAD_BROADCAST_FLAG];
    unsigned dccl_id = head_decoder[HEAD_DCCL_ID];
    
    // test multimessage bit
    if(multimessage_flag)
    {
        // extract frame_size
        // TODO (tes): Make this work properly for strings larger than one byte 
        unsigned frame_size = data->substr(DCCL_NUM_HEADER_BYTES, USER_FRAME_NEXT_SIZE_BYTES)[0];
        
        // erase the frame size byte
        data->erase(DCCL_NUM_HEADER_BYTES, USER_FRAME_NEXT_SIZE_BYTES);
        
        // extract the data for this user-frame
        data_msg->set_data(data->substr(0, (frame_size + DCCL_NUM_HEADER_BYTES)));
        
        data->erase(USER_FRAME_NEXT_SIZE_BYTES,
                    (frame_size + DCCL_NUM_HEADER_BYTES-USER_FRAME_NEXT_SIZE_BYTES));
    }
    else
    {
        data_msg->set_data(*data);
    }
    
    // reset these flags
    head_decoder[HEAD_MULTIMESSAGE_FLAG] = false;
    head_decoder[HEAD_BROADCAST_FLAG] = false;
    
    DCCLHeaderEncoder head_encoder(head_decoder.get());
    data_msg->mutable_data()->replace(0, DCCL_NUM_HEADER_BYTES, head_encoder.str());
    // overwrite destination as BROADCAST if broadcast bit is set
    data_msg->mutable_base()->set_dest(broadcast_flag ? BROADCAST_ID : original_dest);
    publish_incoming_piece(data_msg, dccl_id); 
    
    // put the destination back
    data_msg->mutable_base()->set_dest(original_dest);

    // unstitch until the multimessage flag is no longer set
    return multimessage_flag ? unstitch_recursive(data, data_msg) : true;
}

bool goby::acomms::QueueManager::publish_incoming_piece(protobuf::ModemDataTransmission* message, const unsigned incoming_var_id)
{
    if(message->base().dest() != BROADCAST_ID && message->base().dest() != modem_id_)
    {
        if(log_) *log_<< group("q_in") << warn << "ignoring message for modem_id = "
                      << message->base().dest() << std::endl;
        return false;
    }

    protobuf::QueueKey dccl_key;
    dccl_key.set_type(protobuf::QUEUE_DCCL);
    dccl_key.set_id(incoming_var_id);
    
    std::map<protobuf::QueueKey, Queue>::iterator it_dccl = queues_.find(dccl_key);
    
    if(it_dccl == queues_.end())
    {
        if(log_) *log_<< group("q_in") << warn << "no mapping for this variable ID: "
                      << incoming_var_id << std::endl;
        return false;
    }

    message->mutable_queue_key()->CopyFrom(dccl_key);
    signal_receive(*message);    

    return true;
}

void goby::acomms::QueueManager::add_flex_groups(util::FlexOstream* tout)
{
    tout->add_group("push", util::Colors::lt_cyan, "stack push - outgoing messages (goby_queue)");
    tout->add_group("pop",  util::Colors::lt_green, "stack pop - outgoing messages (goby_queue)");
    tout->add_group("priority",  util::Colors::yellow, "priority contest (goby_queue)");
    tout->add_group("q_out",  util::Colors::cyan, "outgoing queuing messages (goby_queue)");
    tout->add_group("q_in",  util::Colors::green, "incoming queuing messages (goby_queue)");
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
    manip_manager_.clear();
    
    for(int i = 0, n = cfg_.message_file_size(); i < n; ++i)
    {
        std::set<unsigned> new_ids = add_xml_queue_file(cfg_.message_file(i).path());
        BOOST_FOREACH(unsigned new_id, new_ids)
        {
            for(int j = 0, o = cfg_.message_file(i).manipulator_size(); j < o; ++j)
                manip_manager_.add(new_id, cfg_.message_file(i).manipulator(j));
        }
    }
    
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
