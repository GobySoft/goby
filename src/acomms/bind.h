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

#include "acomms/dccl.h"
#include "acomms/queue.h"
#include "acomms/modem_driver.h"
#include "acomms/amac.h"

// binds the driver link-layer callbacks to the QueueManager
void bind(modem::DriverBase& driver, queue::QueueManager& queue_manager)
{
    using boost::bind;
    driver.set_receive_cb
        (bind(&queue::QueueManager::receive_incoming_modem_data, &queue_manager, _1));
    driver.set_ack_cb
        (bind(&queue::QueueManager::handle_modem_ack, &queue_manager, _1));
    driver.set_datarequest_cb
        (bind(&queue::QueueManager::provide_outgoing_modem_data, &queue_manager, _1, _2));
}

// binds the MAC initiate transmission callback to the driver
void bind(amac::MACManager& mac, modem::DriverBase& driver)
{
    using boost::bind;
    mac.set_initiate_transmission_cb(bind(&modem::DriverBase::initiate_transmission, &driver, _1));
}

// binds the MAC destination request to the queue_manager
void bind(amac::MACManager& mac, queue::QueueManager& queue_manager)
{
    using boost::bind;
    mac.set_destination_cb
        (bind(&queue::QueueManager::request_next_destination, &queue_manager, _1));
}

#endif
