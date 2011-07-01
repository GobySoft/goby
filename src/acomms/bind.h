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

#include "goby/acomms/connect.h"
#include "goby/acomms/dccl.h"
#include "goby/acomms/queue.h"
#include "goby/acomms/modem_driver.h"
#include "goby/acomms/amac.h"
#include "goby/util/logger.h"

namespace goby
{
    namespace acomms
    {   
        
        /// binds the driver link-layer callbacks to the QueueManager
        inline void bind(ModemDriverBase& driver, QueueManager& queue_manager)
        {
            connect(&driver.signal_receive,
                    &queue_manager, &QueueManager::handle_modem_receive);
            
            connect(&driver.signal_data_request,
                    &queue_manager, &QueueManager::handle_modem_data_request);
            
            connect(&driver.signal_ack,
                    &queue_manager, &QueueManager::handle_modem_ack);
        }

        /// binds the MAC initiate transmission callback to the driver
        /// and the driver parsed message callback to the MAC
        inline void bind(MACManager& mac, ModemDriverBase& driver)
        {
            connect(&mac.signal_initiate_transmission,
                    &driver, &ModemDriverBase::handle_initiate_transmission);
            
            connect(&mac.signal_initiate_ranging,
                    &driver, &ModemDriverBase::handle_initiate_ranging);

            connect(&driver.signal_all_incoming,
                    &mac, &MACManager::handle_modem_all_incoming);
        }
    

        /// bind all three (shortcut to calling the other three bind functions)
        inline void bind(ModemDriverBase& driver, QueueManager& queue_manager, MACManager& mac)
        {
            bind(driver, queue_manager);
            bind(mac, driver);
        }
        
        // examples
        /// \example acomms/chat/chat.cpp
        /// chat.proto
        /// \verbinclude chat.proto
        /// chat.cpp

    }

}

#endif
