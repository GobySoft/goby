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

#include "goby/acomms/dccl/dccl_common.h"
#include "goby/common/logger.h"
#include "goby/acomms/acomms_helpers.h"

#include "mac_manager.h"

using goby::util::goby_time;
using goby::util::as;
using namespace goby::util::tcolor;
using namespace goby::util::logger;
using goby::glog;

int goby::acomms::MACManager::count_;

goby::acomms::MACManager::MACManager()
    : timer_(io_),
      work_(io_),
      current_slot_(std::list<protobuf::ModemTransmission>::begin()),
      started_up_(false)
{
    ++count_;

    glog_mac_group_ = "goby::acomms::amac::" + as<std::string>(count_);
    goby::glog.add_group(glog_mac_group_, util::Colors::blue);
}

goby::acomms::MACManager::~MACManager()
{ }

void goby::acomms::MACManager::restart_timer()
{    
    // cancel any old timer jobs waiting
    timer_.cancel();
    timer_.expires_at(next_slot_t_);
    timer_.async_wait(boost::bind(&MACManager::begin_slot, this, _1));
}

void goby::acomms::MACManager::stop_timer()
{
    timer_.cancel();
}

void goby::acomms::MACManager::startup(const protobuf::MACConfig& cfg)
{
    if(started_up_)
    {
        glog.is(WARN) && glog << group(glog_mac_group_) << "startup() called but MAC is already started." << std::endl;
        return;
    }
    
    // create a copy for us
    cfg_ = cfg;
    
    started_up_ = true;

    switch(cfg_.type())
    {
        case protobuf::MAC_POLLED:
        case protobuf::MAC_FIXED_DECENTRALIZED:
            for(int i = 0, n = cfg_.slot_size(); i < n; ++i)
                std::list<protobuf::ModemTransmission>::push_back(cfg_.slot(i));
            
            if(cfg_.type() == protobuf::MAC_POLLED)
                glog.is(DEBUG1) && glog << group(glog_mac_group_) << "Using the Centralized Polling MAC scheme" << std::endl;
            else if(cfg_.type() == protobuf::MAC_FIXED_DECENTRALIZED)
                glog.is(DEBUG1) && glog << group(glog_mac_group_) << "Using the Decentralized (Fixed) Slotted TDMA MAC scheme" << std::endl;


            update();
            
            break;

        default:
            return;
    }

    glog.is(DEBUG1) && glog << group(glog_mac_group_)
                            << "the MAC TDMA first cycle begins at time: "
                            << next_slot_t_ << std::endl;
    

}

void goby::acomms::MACManager::shutdown()
{
    stop_timer();
    
    current_slot_ = std::list<protobuf::ModemTransmission>::begin();
    started_up_ = false;
}


void goby::acomms::MACManager::begin_slot(const boost::system::error_code& e)
{    
    // canceled the last timer
    if(e == boost::asio::error::operation_aborted) return;   
    
    const protobuf::ModemTransmission& s = *current_slot_;
    
    bool we_are_transmitting = true;    
    switch(cfg_.type())
    {
        case protobuf::MAC_FIXED_DECENTRALIZED:
            // we only transmit if the packet source is us
            we_are_transmitting = (s.src() == cfg_.modem_id());
            break;

        case protobuf::MAC_POLLED:
            // we always transmit (poll)
            // but be quiet in the case where src = 0
            we_are_transmitting = (s.src() != BROADCAST_ID);
            break;

        default:
            break;
    }

    if(glog.is(DEBUG1))
    {
        glog << group(glog_mac_group_) << "cycle order: [";
    
        for(std::list<protobuf::ModemTransmission>::iterator it = std::list<protobuf::ModemTransmission>::begin(), n = end(); it != n; ++it)
        {
            if(it==current_slot_)
                glog << group(glog_mac_group_) <<  " " << green;
        
            switch(it->type())
            {
                case protobuf::ModemTransmission::DATA: glog << "d"; break;
                case protobuf::ModemTransmission::MICROMODEM_TWO_WAY_PING: glog << "p"; break;
                case protobuf::ModemTransmission::MICROMODEM_REMUS_LBL_RANGING: glog << "r"; break; 
                case protobuf::ModemTransmission::MICROMODEM_NARROWBAND_LBL_RANGING: glog << "n"; break; 
                case protobuf::ModemTransmission::MICROMODEM_MINI_DATA: glog << "m"; break;
                case protobuf::ModemTransmission::ACK:
                case protobuf::ModemTransmission::UNKNOWN:
                    break;
            }
        
            glog << it->src() << "/" << it->dest() << "@" << it->rate() << " " << nocolor;
        }
        glog << " ]" << std::endl;
    }
    
    
    glog.is(DEBUG1) && glog << group(glog_mac_group_) << "starting slot: " << s << std::endl;
    
    if(we_are_transmitting) signal_initiate_transmission(s);

    increment_slot();
    restart_timer();
}

void goby::acomms::MACManager::increment_slot()
{
    switch(cfg_.type())
    {
        case protobuf::MAC_FIXED_DECENTRALIZED:
        case protobuf::MAC_POLLED:
            next_slot_t_ +=
                boost::posix_time::microseconds(current_slot_->GetExtension(protobuf::slot_seconds)*1e6);
            glog.is(DEBUG1) && glog << group(glog_mac_group_) << "next slot at " << next_slot_t_ << std::endl;
            
            ++current_slot_;
            if (current_slot_ == std::list<protobuf::ModemTransmission>::end())
                current_slot_ = std::list<protobuf::ModemTransmission>::begin();
            break;

        default:
            break;
 
    }    
}


boost::posix_time::ptime goby::acomms::MACManager::next_cycle_time()
{
    using namespace boost::gregorian;
    using namespace boost::posix_time;

    time_duration time_of_day = goby_time().time_of_day();
    double since_day_start = time_of_day.total_seconds()*1e6
        + (time_of_day-seconds(time_of_day.total_seconds())).total_microseconds();

    
    glog.is(DEBUG1) && glog << group(glog_mac_group_) << "microseconds since day start: "
                   << since_day_start << std::endl;

    glog.is(DEBUG1) && glog << group(glog_mac_group_) << "cycle duration: "
                   << cycle_duration() << std::endl;
    
    cycles_since_day_start_ = (floor(since_day_start/(cycle_duration()*1e6)) + 1);
    
    glog.is(DEBUG1) && glog << group(glog_mac_group_) << "cycles since day start: "
                 << cycles_since_day_start_ << std::endl;
    
    double secs_to_next = cycles_since_day_start_*cycle_duration();

    
    // day start plus the next cycle starting from now
    return ptime(day_clock::universal_day(), microseconds(secs_to_next*1e6));
}

void goby::acomms::MACManager::update()
{
    // reset the cycle to the beginning
    current_slot_ = std::list<protobuf::ModemTransmission>::begin();
    // advance the next slot time to the beginning of the next cycle
    next_slot_t_ = next_cycle_time();

    glog.is(DEBUG1) && glog << group(glog_mac_group_) << "the MAC TDMA next cycle begins at time: "
                 << next_slot_t_ << std::endl;
    
    // if we can start cycles in the middle, do it
    if(cfg_.start_cycle_in_middle() &&
       std::list<protobuf::ModemTransmission>::size() > 1 &&
       cfg_.type() == protobuf::MAC_FIXED_DECENTRALIZED)
    {
        glog.is(DEBUG1) && glog << group(glog_mac_group_) << "starting next available slot (in middle of cycle)"
                       << std::endl;

        // step back a cycle
        next_slot_t_ -= boost::posix_time::microseconds(cycle_duration()*1e6);
        glog.is(DEBUG1) && glog << group(glog_mac_group_) << "next slot at " << next_slot_t_ << std::endl;

        
        boost::posix_time::ptime now = goby_time();

        // skip slots until we're at a slot that is in the future
        while(next_slot_t_ < now)
            increment_slot();
    }    
    
    if(started_up_)
        restart_timer();
}

double goby::acomms::MACManager::cycle_duration()
{
    double length = 0;
    BOOST_FOREACH(const protobuf::ModemTransmission& slot, *this)
        length += slot.GetExtension(protobuf::slot_seconds);
    
    return length;
}
