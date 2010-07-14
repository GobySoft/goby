// copyright 2009 t. schneider tes@mit.edu
// 
// this file is part of goby-acomms, which is a collection of libraries 
// for acoustic underwater networking
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

// gives functions for binding the goby-acomms libraries together

#ifndef BIND20100120H
#define BIND20100120H

#include <boost/bind.hpp>

#include "goby/acomms/dccl.h"
#include "goby/acomms/queue.h"
#include "goby/acomms/modem_driver.h"
#include "goby/acomms/amac.h"

namespace goby
{
/// utilites for dealing with goby-acomms
namespace acomms
{
    
/// binds the driver link-layer callbacks to the QueueManager
    void bind(modem::DriverBase& driver, queue::QueueManager& queue_manager)
    {
        using boost::bind;
        driver.set_receive_cb
            (bind(&queue::QueueManager::receive_incoming_modem_data, &queue_manager, _1));
        driver.set_ack_cb
            (bind(&queue::QueueManager::handle_modem_ack, &queue_manager, _1));
        driver.set_datarequest_cb
            (bind(&queue::QueueManager::provide_outgoing_modem_data, &queue_manager, _1, _2));
        
        driver.set_destination_cb(boost::bind(&queue::QueueManager::request_next_destination, &queue_manager, _1));

    }
    
/// binds the MAC initiate transmission callback to the driver and the driver parsed message callback to the MAC
    void bind(amac::MACManager& mac, modem::DriverBase& driver)
    {
        mac.set_initiate_transmission_cb(boost::bind(&modem::DriverBase::initiate_transmission, &driver, _1));
        mac.set_initiate_ranging_cb(boost::bind(&modem::DriverBase::initiate_ranging, &driver, _1));
        mac.set_destination_cb(boost::bind(&modem::DriverBase::request_next_destination, &driver, _1));
        driver.set_in_parsed_cb(boost::bind(&amac::MACManager::process_message, &mac, _1));
    }
    

    /// bind all three (shortcut to calling the other three bind functions)
    void bind(modem::DriverBase& driver, queue::QueueManager& queue_manager, amac::MACManager& mac)
    {
        bind(driver, queue_manager);
        bind(mac, driver);
    }

    // examples
    /// \example acomms/examples/chat/chat.cpp
    /// chat.xml
    /// \verbinclude chat.xml
    /// chat.cpp


}

}

#endif
