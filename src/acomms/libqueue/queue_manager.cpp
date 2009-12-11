// copyright 2009 t. schneider tes@mit.edu
// ocean engineering graudate student - mit / whoi joint program
// massachusetts institute of technology (mit)
// laboratory for autonomous marine sensing systems (lamss)
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

#include <map>

#include <boost/foreach.hpp>

#include "queue_constants.h"
#include "queue_manager.h"
#include "queue_xml_callbacks.h"
#include "xml_parser.h"
#include "logger_manipulators.h"

queue::QueueManager::QueueManager(std::ostream* os /* =0 */)
    : modem_id_(0),
      start_time_(time(NULL)),
      os_(os),
      packet_dest_(0),
      packet_ack_(0)
{}
    
queue::QueueManager::QueueManager(const std::string& file, const std::string schema, std::ostream* os /* =0 */)
    : modem_id_(0),
      start_time_(time(NULL)),
      os_(os),
      packet_dest_(0),
      packet_ack_(0)

{
    add_xml_queue_file(file, schema);
}
    
queue::QueueManager::QueueManager(const std::set<std::string>& files,
                                  const std::string schema, std::ostream* os /* =0 */)
    : modem_id_(0),
      start_time_(time(NULL)),
      os_(os),
      packet_dest_(0),
      packet_ack_(0)
{
    BOOST_FOREACH(const std::string& s, files)
        add_xml_queue_file(s, schema);
}

queue::QueueManager::QueueManager(const QueueConfig& cfg, std::ostream* os /* =0 */)
    : modem_id_(0),
      start_time_(time(NULL)),
      os_(os),
      packet_dest_(0),
      packet_ack_(0)
{
    add_queue(cfg);
}

queue::QueueManager::QueueManager(const std::set<QueueConfig>& cfgs, std::ostream* os /* =0 */)
    : modem_id_(0),
      start_time_(time(NULL)),
      os_(os),
      packet_dest_(0),
      packet_ack_(0)
{
    BOOST_FOREACH(const QueueConfig& c, cfgs)
        add_queue(c);    
}

void queue::QueueManager::add_queue(const QueueConfig& cfg)
{
    QueueKey k(cfg.type(), cfg.id());

    Queue q(cfg, os_, modem_id_);
    
    if(queues_.count(k))
    {
        std::stringstream ss;
        ss << "Queue: duplicate key specified for key: " << k;
        throw std::runtime_error(ss.str());
    }
    else if(q.cfg().id() > acomms_util::MAX_ID && q.cfg().type() != queue_ccl)
    {
        std::stringstream ss;
        ss << "Queue: key (" << k << ") is too large for use with libqueue. Use a id smaller than " << acomms_util::MAX_ID;
        throw std::runtime_error(ss.str());
    }
    else
        queues_.insert(std::pair<QueueKey, Queue>(k, q));

    if(os_) *os_<< group("q_out") << "added new queue: \n" << q << std::endl;
    
}

void queue::QueueManager::add_xml_queue_file(const std::string& xml_file,
                                             const std::string xml_schema)
{
    std::vector<QueueConfig> cfgs;
    
    // Register handlers for XML parsing
    QueueContentHandler content(cfgs);
    QueueErrorHandler error;
    // instantiate a parser for the xml message files
    XMLParser parser(content, error);
    // parse(file, [schema])
    if(xml_schema != "")
        xml_schema_ = xml_schema;
        
    parser.parse(xml_file, xml_schema_);

    BOOST_FOREACH(const QueueConfig& c, cfgs)
        add_queue(c);
}


void queue::QueueManager::push_message(QueueKey key, micromodem::Message& new_message)
{
    if(queues_.count(key))
    {
        queues_[key].push_message(new_message);
        qsize(&queues_[key]);
    }
    else
    {
        std::stringstream ss;
        ss << "no queue for key: " << key;
        throw std::runtime_error(ss.str());
    }
    
}

void queue::QueueManager::push_message(unsigned id, micromodem::Message& new_message, QueueType type /* = dccl_queue */)
{ push_message(QueueKey(type, id), new_message); }

void queue::QueueManager::set_on_demand(QueueKey key)
{
    if(queues_.count(key))
        queues_[key].set_on_demand(true);
    else
    {
        std::stringstream ss;
        ss << "no queue for key: " << key;
        throw std::runtime_error(ss.str());
    }
}

void queue::QueueManager::set_on_demand(unsigned id, QueueType type /* = dccl_queue */)
{ set_on_demand(QueueKey(type, id)); }


std::string queue::QueueManager::summary() const
{
    std::string s;
    typedef std::pair<const QueueKey, Queue> P;
    BOOST_FOREACH(const P& p, queues_)
        s += p.second.summary();

    return s;
}

    
std::ostream& queue::operator<< (std::ostream& out, const QueueManager& d)
{
    out << d.summary();
    return out;
}

// combine a number of user frames into one modem frame
// in_frames[0] is on the LSBs of the message
//
// message layout
// [ccl identifier (1 byte)][multimessage flag set to 0 (1 bit)][broadcast flag (true / false) (1 bit)][dccl message id (6 bits)][message hex (30, 62 or 254 bytes)]
// [ccl identifier (1 byte)][multimessage flag set to 1 (1 bit)][broadcast flag (true / false) (1 bit)][dccl message id (6 bits)][user-frame counter (8 bits)][message hex for first user frame] ... [multimessage flag set to 0 (1 bit)][broadcast flag (true / false) (1 bit)][dccl message id (6 bits)][message hex for last user-frame]
//
// ccl identifier - we use 0x20
// multimessage flag - 0 means last user-frame within message, 1 means more user-frames follow within this modem frame
// broadcast flag - 0 means message is NOT broadcast (respect the destination in the message meta-data). 1 means the message is broadcast (decode regardless of who you are)
// user frame counter - if type flag is 1, the byte indicates the size of the following data (in bytes).
// message moos variable id - user set value 0-63 that allows mapping of moos variable names from the receive and send ends
// message hex - the message body
micromodem::Message queue::QueueManager::stitch(const std::vector<micromodem::Message>& in)
{
    micromodem::Message out = in.at(0);
    out.set_dest(packet_dest_);
    out.set_ack(packet_ack_);

    std::string data = out.data();
    unsigned i = 0;
    BOOST_FOREACH(const micromodem::Message& message, in)
    {
        if(out.empty()) throw (std::runtime_error("empty message passed to stitch"));
        
        if(i)
        {
            // chunk off the first byte of the last frame
            data.erase(0, acomms_util::NIBS_IN_BYTE);

            data = message.data() + data;
            std::string frame_size;
            tes_util::number2hex_string(frame_size, message.size()-acomms_util::NUM_HEADER_BYTES);
            data.insert(2*acomms_util::NIBS_IN_BYTE, frame_size);    
        }        
        
        unsigned second_byte;
        tes_util::hex_string2number(data.substr(1*acomms_util::NIBS_IN_BYTE, acomms_util::NIBS_IN_BYTE), second_byte);

        // 10000000 if not the first (i = 0) frame
        unsigned multimessage_mask = i ? acomms_util::MULTIMESSAGE_MASK : 0;

        // 01000000 if destination is broadcast
        unsigned broadcast_mask = (message.dest() == acomms_util::BROADCAST_ID) ? acomms_util::BROADCAST_MASK : 0;
        
        second_byte |= multimessage_mask | broadcast_mask;
        std::string new_second_byte;
        tes_util::number2hex_string(new_second_byte, second_byte);
        
        data.replace(1*acomms_util::NIBS_IN_BYTE, acomms_util::NIBS_IN_BYTE, new_second_byte);        
        ++i;
    }

    out.set_data(data);    
    return out;
}

void queue::QueueManager::clear_packet()
{
    typedef std::pair<unsigned, Queue*> P;
    BOOST_FOREACH(const P& p, waiting_for_ack_)
        p.second->clear_ack_queue();
    
    waiting_for_ack_.clear();
    
    packet_dest_ = 0;
    packet_ack_ = 0;
}

// finds and publishes outgoing data for the modem driver
// first query every Queue for its priority data using
// priority_values(priority, last_send_time)
// priority_values returns false if that object has no data to give
// (either no data at all, or in blackout interval) 
// thus, from all the priority values that return true, pick the one with the lowest
// priority value, or given a tie, pick the one with the oldest last_send_time
bool queue::QueueManager::provide_outgoing_modem_data(micromodem::Message& message_in, micromodem::Message& message_out)
{
    if(message_in.frame() == 1 || message_in.frame() == 0)
    {
        clear_packet();
    }
    else // discipline remaining frames to the first frame dest and ack values
    {
        message_in.set_dest(packet_dest_);
        message_in.set_ack(packet_ack_);
    }
    
    Queue* winning_var = find_next_sender(message_in);

    // no data at all for this frame ... :(
    if(!winning_var)
    {
        message_out = micromodem::Message();

        // we have to conform to the rest of the packet...
        message_out.set_src(modem_id_);
        message_out.set_dest(packet_dest_);
        message_out.set_ack(packet_ack_);

        if(os_) *os_<< group("q_out") << "no data found. sending blank to firmware" 
                    << ": " << message_out.snip() << std::endl;
        
        return true;
    }    

    // keep filling up the frame with messages until we have nothing small enough to fit...
    std::vector<micromodem::Message> user_frames;
    while(winning_var)
    {
        micromodem::Message next_message = winning_var->give_data(message_in.frame());
        user_frames.push_back(next_message);

        // if a destination has been set or ack been set, do not unset these
        if (packet_dest_ == acomms_util::BROADCAST_ID) packet_dest_ = next_message.dest();
        if (packet_ack_ == false) packet_ack_ = next_message.ack();
        
        if(os_) *os_<< group("q_out") << "sending data to firmware from: "
                    << winning_var->cfg().name() 
                    << ": " << next_message.snip() << std::endl;
        
        if(winning_var->cfg().ack() == false)
        {
            winning_var->pop_message(message_in.frame());
            qsize(winning_var);
        }
        else
            waiting_for_ack_.insert(std::pair<unsigned, Queue*>(message_in.frame(), winning_var));
        
        message_in.set_size(message_in.size() - next_message.size());

        // if there's no room for more, don't bother looking
        // also end if the message you have is a CCL message
        if(message_in.size() > acomms_util::NUM_HEADER_BYTES && winning_var->cfg().type() != queue_ccl)
            winning_var = find_next_sender(message_in);
        else
            break;
    }

    message_out = (user_frames.size() > 1) ? stitch(user_frames) : user_frames[0];
    
    return true;
}


queue::Queue* queue::QueueManager::find_next_sender(micromodem::Message& message)
{   
// competition between variable about who gets to send
    double winning_priority;
    double winning_last_send_time;
    bool found_data = false;
    bool all_queues_empty = true;

    Queue* winning_var = NULL;
    
    if(os_) *os_<< group("priority") << "starting priority contest"
                << "... request: " << message.snip() << std::endl;
    
    for(std::map<QueueKey, Queue>::iterator it = queues_.begin(), n = queues_.end(); it != n; ++it)
    {
        Queue& oq = it->second;
        
        // flush and encode on demands
        if(oq.on_demand())
        {
            if(oq.size()) oq.flush();
            if(callback_ondemand)
            {
                micromodem::Message new_message;
                callback_ondemand(it->first, message, new_message);
                push_message(it->first, new_message);
            }
        }
        
        double priority, last_send_time;
        std::string error;
        if(oq.priority_values(&priority, &last_send_time, &message, &error))
        { 
            if(!found_data || priority > winning_priority ||
               (priority == winning_priority && last_send_time < winning_last_send_time))
            {
                winning_priority = priority;
                winning_last_send_time = last_send_time;
                winning_var = &oq;
                found_data = true;
            }
            if(os_) *os_<< group("priority") << "\t" << oq.cfg().name() << " has priority value"
                   << ": " << priority << std::endl;
            all_queues_empty = false;
        }
        else
        {
            if(error != "no available messages")
            {
                if(os_) *os_<< group("priority") << "\t" << error << std::endl;
                all_queues_empty = false;
            }
        }
    }

    if(all_queues_empty)
    {
        if(os_) *os_<< group("priority") << "\t"
                    << "all queues have no messages" << std::endl;
    }
    else
    {
        if(os_) *os_<< group("priority") << "\t"
                    << "all other queues have no messages" << std::endl;
    }

    if(winning_var)
    {
        if(os_) *os_<< group("priority") << winning_var->cfg().name()
                    << " has highest priority." << std::endl;
    }
    
    return winning_var;
}    


bool queue::QueueManager::handle_modem_ack(micromodem::Message& message)
{
    if(!waiting_for_ack_.count(message.frame()))
    {
        if(os_) *os_<< group("q_in")
                    << "got ack but we were not expecting one" << std::endl;
        return false;
    }
    else
    {
        unsigned dest = message.dest();
        
        if(dest != modem_id_)
        {
            if(os_) *os_<< group("q_in") << warn
                        << "ignoring ack for modem_id = " << dest << std::endl;
            return false;
        }
        else
        {
            // got an ack, let's pop this!
            if(os_) *os_<< group("q_in") << "received ack for this id" << std::endl;
            
            std::multimap<unsigned, Queue *>::iterator it = waiting_for_ack_.find(message.frame());
            while(it != waiting_for_ack_.end())
            {
                Queue* oq = it->second;

                micromodem::Message removed_msg;
                if(!oq->pop_message_ack(message.frame(), removed_msg))
                {
                    if(os_) *os_<< group("q_in") << warn
                                << "failed to pop message from "
                                << oq->cfg().name() << std::endl;
                }
                else
                {
                    qsize(oq);
                    if(callback_ack)
                        callback_ack(QueueKey(oq->cfg().type(), oq->cfg().id()), removed_msg);
                }
                
                waiting_for_ack_.erase(it);

                it = waiting_for_ack_.find(message.frame());
            }
            
            return true;
        }
    }    
}


// parses and publishes incoming data
// by matching the variableID field with the variable specified
// in a "receive = " line of the configuration file
bool queue::QueueManager::receive_incoming_modem_data(micromodem::Message& message)
{    
    if(os_) *os_<< group("q_in") << "received message"
                << ": " << message.snip() << std::endl;
   
    std::string data = message.data();
   
    if(data.size() <= 4)
        return false;

    std::string header_byte_hex = data.substr(0, acomms_util::NIBS_IN_BYTE);
    // check for queue_dccl type
    if(header_byte_hex == acomms_util::DCCL_CCL_HEADER_STR)
    {
        unsigned original_dest = message.dest();

        // grab the second byte, which contains multimessage flag, broadcast flag, message id
        // xxxxxxxx
        // x - multimessage flag (1 = more message to come, 0 = last user frame in message)
        //  x - broadcast flag (1 = use broadcast as destination, 0 = use message destination)
        //   xxxxxx - 6 bit message id (0-63)
        unsigned second_byte;
        tes_util::hex_string2number(data.substr(1*acomms_util::NIBS_IN_BYTE, acomms_util::NIBS_IN_BYTE), second_byte);
        
        // test multimessage bit
        while(second_byte & acomms_util::MULTIMESSAGE_MASK)
        {
            // extract frame_size
            unsigned frame_size;
            tes_util::hex_string2number(data.substr(2*acomms_util::NIBS_IN_BYTE, acomms_util::NIBS_IN_BYTE), frame_size);
            // erase the frame size byte
            data.erase(2*acomms_util::NIBS_IN_BYTE, acomms_util::NIBS_IN_BYTE);

            // extract the data for this user-frame
            message.set_data(data.substr(0, (frame_size + acomms_util::NUM_HEADER_BYTES)*acomms_util::NIBS_IN_BYTE));
            // erase the data for this user frame, leaving the CCL header intact
            data.erase(1*acomms_util::NIBS_IN_BYTE, (frame_size+1)*acomms_util::NIBS_IN_BYTE);

            // overwrite destination as BROADCAST if broadcast bit is set
            message.set_dest((second_byte & acomms_util::BROADCAST_MASK) ? acomms_util::BROADCAST_ID : original_dest);
            
            publish_incoming_piece(message, second_byte & acomms_util::VAR_ID_MASK);

            // pull the new second byte
            tes_util::hex_string2number(data.substr(1*acomms_util::NIBS_IN_BYTE, acomms_util::NIBS_IN_BYTE), second_byte);
        }
        // get the last part
        message.set_data(data);
        message.set_dest((second_byte & acomms_util::BROADCAST_MASK) ? acomms_util::BROADCAST_ID : original_dest);
        publish_incoming_piece(message, second_byte & acomms_util::VAR_ID_MASK);
    }
    // check for ccl type
    else
    {
        std::string ccl_header = data.substr(0, acomms_util::NIBS_IN_BYTE);
        unsigned ccl_id;
        tes_util::hex_string2number(ccl_header, ccl_id);

        QueueKey key(queue_ccl, ccl_id);
        std::map<QueueKey, Queue>::iterator it = queues_.find(key);
            
        if (it != queues_.end())
        {
            if(callback_receive_ccl) callback_receive_ccl(key, message);
        }
        else
        {
            if(os_) *os_<< group("q_in") << warn
                        << "incoming data string is not for us (first byte is not 0x"
                        << acomms_util::DCCL_CCL_HEADER_STR 
                        << " and not one of the alternative CCL types)." << std::endl;
            return false;
        }
    }
    
    return true;
}


bool queue::QueueManager::publish_incoming_piece(micromodem::Message message, const unsigned incoming_var_id)
{
    if(message.dest() != 0 && message.dest() != modem_id_)
    {
        if(os_) *os_<< group("q_in") << warn << "ignoring message for modem_id = "
                    << message.dest() << std::endl;
        return false;
    }

    QueueKey dccl_key(queue_dccl, incoming_var_id);
    QueueKey data_key(queue_data, incoming_var_id);
    std::map<QueueKey, Queue>::iterator it_dccl = queues_.find(dccl_key);
    std::map<QueueKey, Queue>::iterator it_data = queues_.find(data_key);
    
    if(it_dccl == queues_.end() && it_data == queues_.end())
    {
        if(os_) *os_<< group("q_in") << warn << "no mapping for this variable ID: "
                    << incoming_var_id << std::endl;
        return false;
    }
    
    if(it_data != queues_.end())
    {
        message.remove_header();
        if(callback_receive) callback_receive(data_key, message);    
    }
    else
    {
        // restore the second byte
        std::string new_second_byte;
        tes_util::number2hex_string(new_second_byte, incoming_var_id);
        message.replace_in_data(1*acomms_util::NIBS_IN_BYTE, acomms_util::NIBS_IN_BYTE, new_second_byte);
        if(callback_receive) callback_receive(dccl_key, message);    
    }

    return true;
}

int queue::QueueManager::request_next_destination(unsigned size /* = std::numeric_limits<unsigned>::max() */)
{
    clear_packet();

    micromodem::Message message;
    message.set_size(size);
    
    Queue* winning_var = find_next_sender(message);
    if(winning_var)
    {
        unsigned dest = winning_var->give_dest();
        if(os_) *os_ << group("q_out") <<  "got dest request for size " << size
                     << ", giving dest: " << dest << std::endl;
        return dest;
    }
    else
    {
        return no_available_destination;
    }
    
}
