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

#ifndef MAC20091019H
#define MAC20091019H

#include <boost/lexical_cast.hpp>
#include <boost/function.hpp>
#include <boost/foreach.hpp>
#include <boost/bind.hpp>
#include <boost/asio.hpp>

#include "goby/protobuf/modem_message.pb.h"
#include "goby/protobuf/amac.pb.h"
#include "goby/acomms/modem_driver.h"
#include "goby/util/time.h"
#include "goby/util/string.h"

namespace goby
{
    namespace util
    {
        class FlexOstream;
    }

    namespace acomms
    {
        /// \name Acoustic MAC Library callback function type definitions
        //@{

        /// provides an API to the goby-acomms MAC library.
        class MACManager
        {
            
          public:

            /// \name Constructors/Destructor
            //@{         
            /// \brief Default constructor.
            ///
            /// \param os std::ostream object or FlexOstream to capture all humanly readable runtime and debug information (optional).
            MACManager(std::ostream* log = 0);
            ~MACManager();
            //@}
        
            /// \brief Starts the MAC
            void startup(const protobuf::MACConfig& cfg);

            /// \brief Shutdown the MAC
            void shutdown();
            
            /// \brief Must be called regularly for the MAC to perform its work
            void do_work();

            /// \brief Manually initiate a transmission out of the normal cycle. This is not normally called by the user of MACManager
            void send_poll(const boost::system::error_code&);

            /// \brief Call every time a message is received from vehicle to "discover" this vehicle or reset the expire timer. Only needed when the type is amac::mac_auto_decentralized.
            ///
            /// \param message the new message (used to detect vehicles)
            void handle_modem_all_incoming(const protobuf::ModemMsgBase& m);
            
            void add_flex_groups(util::FlexOstream* tout);
            
            /// \name Manipulate slots
            //@{
            /// \return iterator to newly added slot
            std::map<int, protobuf::Slot>::iterator add_slot(const protobuf::Slot& s);
            /// \brief removes any slots in the cycle where amac::operator==(const protobuf::Slot&, const protobuf::Slot&) is true.
            ///
            /// \return true if one or more slots are removed
            bool remove_slot(const protobuf::Slot& s);
            void clear_all_slots() { id2slot_.clear(); slot_order_.clear(); }    
            //@}            
            
            boost::signal<void (protobuf::ModemMsgBase* m)> signal_initiate_transmission;
            boost::signal<void (protobuf::ModemRangingRequest* m)> signal_initiate_ranging;
            
            /// \example libamac/examples/amac_simple/amac_simple.cpp
            /// amac_simple.cpp
        
            /// \example acomms/examples/chat/chat.cpp
        
          private:
            boost::posix_time::ptime next_cycle_time();

            void restart_timer();
            void stop_timer();
    
            void expire_ids();
            void process_cycle_size_change();

            unsigned cycle_count() { return slot_order_.size(); }
            unsigned cycle_length();
            unsigned cycle_sum();
            void position_blank();
    
          private:
            std::ostream* log_;

            protobuf::MACConfig cfg_;
            
            // asynchronous timer
            boost::asio::io_service io_;
            boost::asio::deadline_timer timer_;
            bool timer_is_running_;
    
            boost::posix_time::ptime next_cycle_t_;
            boost::posix_time::ptime next_slot_t_;

            // <id, last time heard from>
            typedef std::multimap<int, protobuf::Slot>::iterator id2slot_it;

            id2slot_it blank_it_;
            std::list<id2slot_it> slot_order_;
            std::multimap<int, protobuf::Slot> id2slot_;
    
            std::list<id2slot_it>::iterator current_slot_;
    
            unsigned cycles_since_day_start_;    
    
            // entropy value used to determine how the "blank" slot moves around relative to the values of the modem ids. determining the proper value for this is a bit of work and i will detail when i have time.
            enum { ENTROPY = 5 };

            bool startup_done_;
        };

        ///
        namespace protobuf
        {
            
            inline bool operator<(const std::map<int, protobuf::Slot>::iterator& a, const std::map<int, protobuf::Slot>::iterator& b)
            { return a->second.src() < b->second.src(); }
            
            /// Are two amac::protobuf::Slot equal?
            inline bool operator==(const protobuf::Slot& a, const protobuf::Slot& b)
            {
                return (a.src() == b.src() &&
                        a.dest() == b.dest() &&
                        a.rate() == b.rate() &&
                        a.type() == b.type() &&
                        a.slot_seconds() == b.slot_seconds() &&
                        a.type() == b.type());
            }
        }
    }
}

#endif
