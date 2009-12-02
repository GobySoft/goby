// t. schneider tes@mit.edu 06.05.08
// ocean engineering graudate student - mit / whoi joint program
// massachusetts institute of technology (mit)
// laboratory for autonomous marine sensing systems (lamss)
// 
// this is out_queue.h, part of pAcommsHandler
//
// see the readme file within this directory for information
// pertaining to usage and purpose of this script.
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


#ifndef OutQueueH
#define OutQueueH

#include <iostream>
#include <vector>
#include <deque>
#include <sstream>
#include <bitset>
#include <list>
#include <string>
#include <map>
#include <ctime>

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

#include "flex_cout.h"
#include "modem_message.h"

typedef std::list<micromodem::Message>::iterator messages_it;
typedef std::multimap<unsigned, messages_it>::iterator waiting_for_ack_it;

class OutQueue
{
public:
    OutQueue(FlexCout* ptout = NULL,
             unsigned modem_id = 0,
             bool is_ccl = false);

    bool push_message(micromodem::Message* new_message);
    micromodem::Message give_data(unsigned frame);
    bool pop_message(unsigned frame);    

    bool priority_values(double* priority, double* last_send_time, micromodem::Message* message, std::string* error);
    unsigned give_dest();

    void clear_ack_queue() { waiting_for_ack_.clear(); }
    void flush() { messages_.clear(); }        
    size_t size() { return messages_.size(); }
    
    
    
    std::string name() const       { return name_; }    
    std::string var_id() const     { return var_id_; }
    unsigned var_id_as_num() const { return !is_ccl_ ? boost::lexical_cast<unsigned>(var_id_) : 0; }
        
    
    bool ack() const              { return ack_; }
    double blackout_time() const  { return blackout_time_; }
    unsigned max_queue() const    { return max_queue_; }
    bool newest_first() const     { return newest_first_; }
    double priority() const       { return priority_; }
    double priority_T() const     { return priority_T_; }
    bool is_dccl() const          { return is_dccl_; }
    bool is_ccl() const           { return is_ccl_; }
    bool on_demand() const        { return on_demand_; }
    double last_send_time() const { return last_send_time_; }
    
    void set_name(const std::string& s)     { name_ = s; }
    void set_var_id(const std::string& s)      { var_id_ = s; }

    void set_ack(bool b)             { ack_ = b; }
    void set_blackout_time(double d) { blackout_time_ = d; }
    void set_max_queue(unsigned u)   { max_queue_ = u; }
    void set_newest_first(bool b)    { newest_first_ = b; }
    void set_priority(double d)      { priority_ = d; }
    void set_priority_T(double d)    { priority_T_ = d; }
    void set_is_dccl(bool b)         { is_dccl_ = b; }
    void set_is_ccl(bool b)          { is_ccl_ = b; }
    void set_modem_id(unsigned u)    { modem_id_ = u; }
    void set_on_demand(bool b)       { on_demand_ = b; }

    void set_var_id(unsigned u)                  { set_var_id(boost::lexical_cast<std::string>(u)); }
    void set_ack(const std::string& s)           { set_ack(boost::lexical_cast<bool>(s)); }
    void set_blackout_time(const std::string& s) { set_blackout_time(boost::lexical_cast<double>(s)); }
    void set_max_queue(const std::string& s)     { set_max_queue(boost::lexical_cast<unsigned>(s)); }
    void set_newest_first(const std::string& s)  { set_newest_first(boost::lexical_cast<bool>(s)); }
    void set_priority(const std::string& s)      { set_priority(boost::lexical_cast<double>(s)); }
    void set_priority_T(const std::string& s)    { set_priority_T(boost::lexical_cast<double>(s)); }
    void set_is_dccl(const std::string& s)       { set_is_dccl(boost::lexical_cast<bool>(s)); }
    void set_is_ccl(const std::string& s)        { set_is_ccl(boost::lexical_cast<bool>(s)); }
    void set_modem_id(const std::string& s)      { set_modem_id(boost::lexical_cast<unsigned>(s)); }
    void set_on_demand(const std::string& s)     { set_on_demand(boost::lexical_cast<bool>(s)); }
    
private:
    waiting_for_ack_it find_ack_value(messages_it it_to_find);
    messages_it next_message_it();    
    
private:
    std::string name_;
    std::string var_id_;
    bool ack_;
    double blackout_time_;
    unsigned max_queue_;
    bool newest_first_;
    double priority_;
    double priority_T_;
    // true for dccl messages
    // these already include the header
    bool is_dccl_;

    bool on_demand_;
    
    double last_send_time_;
    
    unsigned modem_id_;
    bool is_ccl_;
    
    FlexCout* ptout_;
    
    std::list<micromodem::Message> messages_;

    // map frame number onto messages list iterator
    // can have multiples in the same frame now
    std::multimap<unsigned, messages_it> waiting_for_ack_;
    
};

std::ostream & operator<< (std::ostream & os, const OutQueue & oq);

#endif
