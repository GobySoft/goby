// copyright 2009, 2010 t. schneider tes@mit.edu
// 
// this file is part of libamac, a medium access control for
// acoustic networks. 
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

#include <iostream>
#include <cmath>

#include <boost/date_time/gregorian/gregorian_types.hpp>
#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <boost/foreach.hpp>

#include "goby/acomms/modem_message.h"
#include "goby/acomms/libdccl/dccl_constants.h"
#include "goby/util/logger.h"

#include "mac_manager.h"

using goby::util::goby_time;

goby::acomms::MACManager::MACManager(std::ostream* os /* =0 */)
    : rate_(0),
      slot_time_(15),
      expire_cycles_(5),
      os_(os),
      modem_id_(0),
      timer_(io_),
      timer_is_running_(false),
      current_slot_(slot_order_.begin()),
      type_(mac_notype)
{ }

goby::acomms::MACManager::~MACManager()
{ }

void goby::acomms::MACManager::do_work()
{
 //    if(os_) *os_ << group("mac") << "timer is running: " << std::boolalpha << timer_is_running_ << std::endl;
    
    // let the io service execute ready handlers (in this case, is the timer up?)
    if(timer_is_running_) io_.poll();
}

void goby::acomms::MACManager::restart_timer()
{    
    // cancel any old timer jobs waiting
    timer_.cancel();
    timer_.expires_at(next_slot_t_);
    timer_.async_wait(boost::bind(&MACManager::send_poll, this, _1));
    timer_is_running_ = true;
}

void goby::acomms::MACManager::stop_timer()
{
    timer_is_running_ = false;
    timer_.cancel();
}


void goby::acomms::MACManager::startup()
{
    switch(type_)
    {
        case mac_slotted_tdma:
            if(os_) *os_ << group("mac")
                         << "Using the Slotted TDMA MAC scheme with autodiscovery"
                         << std::endl;
            blank_it_ = add_slot(Slot(acomms::BROADCAST_ID,
                                      Slot::query_destination,
                                      rate_,
                                      Slot::slot_data,
                                      slot_time_,
                                      boost::posix_time::ptime(boost::posix_time::pos_infin)));

            add_slot(Slot(modem_id_,
                          Slot::query_destination,
                          rate_,
                          Slot::slot_data,
                          slot_time_,
                          boost::posix_time::ptime(boost::posix_time::pos_infin)));

            slot_order_.sort();

            next_slot_t_ = next_cycle_time();
            position_blank();
            
            if(os_) *os_ << group("mac")
                         << "the MAC TDMA first cycle begins at time: "
                         << next_slot_t_ << std::endl;
            break;

        case mac_polled:
            if(os_) *os_ << group("mac")
                         << "Using the Centralized Polling MAC scheme" << std::endl;
            next_slot_t_ = goby_time();
            break;

        default:
            return;
    }
            
    if(!slot_order_.empty())
        restart_timer();
}

void goby::acomms::MACManager::send_poll(const asio::error_code& e)
{    
    // canceled the last timer
    if(e == asio::error::operation_aborted) return;   
    
    const Slot& s = (*current_slot_)->second;
    unsigned id = s.src();
    
    bool send_poll = true;
    
    int destination = (s.dest() == Slot::query_destination)
        ? callback_dest(s.rate()) : s.dest();
    
    switch(type_)
    {
        case mac_slotted_tdma:
            send_poll = (id == modem_id_ && destination != NO_AVAILABLE_DESTINATION);
            break;

        case mac_polled:
            send_poll = (destination != NO_AVAILABLE_DESTINATION);
            break;

        default:
            break;
    }

    if(os_)
    {
        *os_ << group("mac") << "cycle order: [";
    
        BOOST_FOREACH(id2slot_it it, slot_order_)
        {
            *os_ << " " << it->second.src() << it->second.type_as_string()[0];
        }

        *os_ << " ]" << std::endl;    

        *os_ << group("mac") << s.type_as_string() << " slot is starting: {" << s.src() << " to "
             << destination << " @ " << s.rate() << "}" << std::endl;
    }

    
    if(send_poll)
    {
        ModemMessage m;
        switch(s.type())
        {
            case Slot::slot_data:
                m.set_src(s.src());
                m.set_dest(destination);
                m.set_rate(s.rate());
                m.set_ack(0); // actually a free bit, not the ack (field was deprecated). right now we're not using it
                callback_initiate_transmission(m);
                break;

            case Slot::slot_ping:
                m.set_src(s.src());
                m.set_dest(destination);
                callback_initiate_ranging(m);
                break;

            default:
                break;
        }
    }
    
    ++current_slot_;
    
    switch(type_)
    {
        case mac_slotted_tdma:
            expire_ids();

            if (current_slot_ == slot_order_.end())
            {
                ++cycles_since_day_start_;
                if(os_) *os_ << group("mac") << "cycles since day start: "
                             << cycles_since_day_start_ << std::endl;    
                position_blank();
            }
            next_slot_t_ += boost::posix_time::seconds(slot_time_);
            break;
            
        case mac_polled:
            if (current_slot_ == slot_order_.end()) current_slot_ = slot_order_.begin();
            next_slot_t_ += boost::posix_time::seconds(s.slot_time());
            break;
            
        default:
            break;
    }

    restart_timer();
}

boost::posix_time::ptime goby::acomms::MACManager::next_cycle_time()
{
    using namespace boost::gregorian;
    using namespace boost::posix_time;

    int since_day_start = goby_time().time_of_day().total_seconds();
    cycles_since_day_start_ = (floor(since_day_start/cycle_length()) + 1);

    if(os_) *os_ << group("mac") << "cycles since day start: "
                 << cycles_since_day_start_ << std::endl;
    
    unsigned secs_to_next = cycles_since_day_start_*cycle_length();

    
    // day start plus the next cycle starting from now
    return ptime(day_clock::universal_day(), seconds(secs_to_next));
}

void goby::acomms::MACManager::process_message(const ModemMessage& m)
{
    unsigned id = m.src();
    
    if(type_ != mac_slotted_tdma)
        return;
    

// if we haven't heard from this id before we have to reset (since the cycle changed
    bool new_id = !id2slot_.count(id);
    
    if(new_id)
    {
        if(os_) *os_ << group("mac") << "discovered id " << id << std::endl;
        
        slot_order_.push_back
            (id2slot_.insert
             (std::pair<unsigned, Slot> (id, Slot(id, Slot::query_destination, rate_, Slot::slot_data, slot_time_, goby_time()))));

        slot_order_.sort();

        process_cycle_size_change();
    }
    else
    {
        std::pair<id2slot_it, id2slot_it> p = id2slot_.equal_range(id);
        for(id2slot_it it = p.first; it != p.second; ++it)
            it->second.set_last_heard_time(goby_time());
    }
}

void goby::acomms::MACManager::expire_ids()
{
    bool reset = false;
    
    for(id2slot_it it = id2slot_.begin(), n = id2slot_.end(); it != n; ++it)
    {
        if(it->second.last_heard_time() < goby_time()-boost::posix_time::seconds(cycle_length()*expire_cycles_) && it->first != modem_id_)
        {            
            if(os_) *os_ << group("mac") << "removed id " << it->first
                         << " after not hearing for " << expire_cycles_
                         << " cycles." << std::endl;

            id2slot_.erase(it);
            slot_order_.remove(it);
            reset = true;
        }
    }

    if(reset) process_cycle_size_change();
}

void goby::acomms::MACManager::process_cycle_size_change()
{
    next_slot_t_ = next_cycle_time();
    if(os_) *os_ << group("mac") << "the MAC TDMA next cycle begins at time: "
                 << next_slot_t_ << std::endl;

    position_blank();    
  
    restart_timer();
}


unsigned goby::acomms::MACManager::cycle_sum()
{
    unsigned s = 0;
    BOOST_FOREACH(id2slot_it it, slot_order_)
        s += it->second.src();
    return s;
}

void goby::acomms::MACManager::position_blank()
{
    unsigned blank_pos = cycle_length() - ((cycles_since_day_start_ % ENTROPY) == (cycle_sum() % ENTROPY)) - 1;
    
    slot_order_.remove(blank_it_);
    
    std::list<id2slot_it>::iterator id_it = slot_order_.begin();
    for(unsigned i = 0; i < blank_pos; ++i)
        ++id_it;

    slot_order_.insert(id_it, blank_it_);
    
    current_slot_ = slot_order_.begin();
}

std::map<unsigned, goby::acomms::Slot>::iterator goby::acomms::MACManager::add_slot(const Slot& s)    
{
    bool do_timer_start = slot_order_.empty();

    std::map<unsigned, Slot>::iterator it =
        id2slot_.insert(std::pair<unsigned, Slot>(s.src(), s));

    slot_order_.push_back(it);
    current_slot_ = slot_order_.begin();
    
    if(os_) *os_ << group("mac") << "added new slot " << s << std::endl;
    
    if(do_timer_start)
    {
        next_slot_t_ = goby_time();
        restart_timer();
    }
    
    return it;
}

bool goby::acomms::MACManager::remove_slot(const Slot& s)    
{
    bool removed_a_slot = false;
    
    for(id2slot_it it = id2slot_.begin(), n = id2slot_.end(); it != n; ++it)
    {
        if(s == it->second)
        {
            if(os_) *os_ << group("mac") << "removed slot " << it->second << std::endl;
            slot_order_.remove(it);
            id2slot_.erase(it);
            removed_a_slot = true;
            break;
        }
    }

    
    if(slot_order_.empty())
        stop_timer();

    if(removed_a_slot)
        current_slot_ = slot_order_.begin();    
    
    return removed_a_slot;
}
