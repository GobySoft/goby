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
#include "asio.hpp"

#include "goby/util/time.h"

namespace goby
{
    namespace util
    {
        class FlexOstream;
    }

    namespace acomms
    {
	class ModemMessage;
        /// \name Acoustic MAC Library callback function type definitions
        //@{

        /// \brief boost::function for a function taking a unsigned and returning an integer.
        ///
        /// Think of this as a generalized version of a function pointer (int (*) (unsigned)). See http://www.boost.org/doc/libs/1_34_0/doc/html/function.html for more on boost:function.
        typedef boost::function<int (unsigned)> MACIdFunc;

        /// \brief boost::function for a function taking a single acomms::ModemMessage reference.
        ///
        /// Think of this as a generalized version of a function pointer (void (*)(acomms::ModemMessage&)). See http://www.boost.org/doc/libs/1_34_0/doc/html/function.html for more on boost:function.
        typedef boost::function<void (const acomms::ModemMessage & message)> MACMsgFunc1;

        //@}

/// Enumeration of MAC types
        enum MACType {
            mac_notype, /*!< no MAC */
            mac_slotted_tdma, /*!< decentralized time division multiple access */
            mac_polled /*!< centralized polling */
        };    

        /// Represents a slot of the TDMA cycle
        class Slot
        {
          public:
            /// Enumeration of slot types
            enum SlotType
            {
                slot_notype, /*!< useless slot */
                slot_data, /*!< slot to send data packet */
                slot_ping /*!< slot to send ping (ranging) */
            };
        
            /// \name Constructors/Destructor
            //@{         
            /// \brief Default constructor.
            ///
            /// \param src id representing sender of the data packet or ping
            /// \param dest id representing destination of the data packet or ping
            /// \param rate value 0-5 representing modem transmission rate. 0 is slowest, 5 is fastest
            /// \param type amac::Slot::SlotType of this slot
            /// \param slot_time length of time this slot should last in seconds
            /// \param last_heard_time last time (in seconds since 1/1/70) the src vehicle was heard from
          Slot(unsigned src = 0,
               unsigned dest = 0,
               int rate = 0,
               SlotType type = slot_notype,
               unsigned slot_time = 15,
               boost::posix_time::ptime last_heard_time = boost::posix_time::not_a_date_time)
              : src_(src),
                dest_(dest),
                rate_(rate),
                type_(type),
                slot_time_(slot_time),
                last_heard_time_(last_heard_time)        
                {  }

            //@}

            /// \name Set
            //@{
            void set_src(unsigned src) { src_ = src; }
            void set_dest(int dest) { dest_ = dest; }
            void set_rate(int rate) { rate_ = rate; }
            void set_type(SlotType type) { type_ = type; }
            void set_last_heard_time(boost::posix_time::ptime t) { last_heard_time_ = t; }
            void set_slot_time(unsigned t) { slot_time_ = t; }



            //@}

            /// \name Get
            //@{
            unsigned src() const { return src_; }
            int dest() const { return dest_; }
            int rate() const { return rate_; } 
            SlotType type() const { return type_; }
            std::string type_as_string() const
            {
                switch(type_)
                {
                    case slot_data: return "data";
                    case slot_ping: return "ping";
                    default: return "unknown";
                }
            }
        
            boost::posix_time::ptime last_heard_time() const { return last_heard_time_; }
            unsigned slot_time() const { return slot_time_; }        
            //@}
        
            enum { query_destination = -1 };        
        
        
          private:
            unsigned src_;
            int dest_;
            int rate_;
            SlotType type_;
            unsigned slot_time_;
            boost::posix_time::ptime last_heard_time_;
        };
    
        inline std::ostream& operator<<(std::ostream& os, const Slot& s) 
        { return os << "type: " << s.type_as_string() <<  " | src: " << s.src() << " | dest: " << s.dest() << " | rate: " << s.rate() << " | slot_time: " << s.slot_time(); }


        /// provides an API to the goby-acomms MAC library.
        class MACManager
        {

          public:

            /// \name Constructors/Destructor
            //@{         
            /// \brief Default constructor.
            ///
            /// \param os std::ostream object or FlexOstream to capture all humanly readable runtime and debug information (optional).
            MACManager(std::ostream* os = 0);
            ~MACManager();
            //@}
        
            /// \brief Starts the MAC
            void startup();
            /// \brief Must be called regularly for the MAC to perform its work
            void do_work();

            /// \brief Manually initiate a transmission out of the normal cycle. This is not normally called by the user of MACManager
            void send_poll(const asio::error_code&);

            /// \brief Call every time a message is received from vehicle to "discover" this vehicle or reset the expire timer. Only needed when the type is amac::mac_slotted_tdma.
            ///
            /// \param message the new message (used to detect vehicles)
            void process_message(const acomms::ModemMessage& m);
            
            void add_flex_groups(util::FlexOstream& tout);
            
            /// \name Manipulate slots
            //@{
            /// \return iterator to newly added slot
            std::map<unsigned, Slot>::iterator add_slot(const Slot& s);
            /// \brief removes any slots in the cycle where amac::operator==(const Slot&, const Slot&) is true.
            ///
            /// \return true if one or more slots are removed
            bool remove_slot(const Slot& s);
            void clear_all_slots() { id2slot_.clear(); slot_order_.clear(); }    
            //@}

            
            

    
            /// \name Set
            //@{    
            void set_type(MACType type) { type_ = type; }
            void set_modem_id(unsigned modem_id) { modem_id_ = modem_id; }    
            void set_modem_id(const std::string& s) { set_modem_id(boost::lexical_cast<unsigned>(s)); }
            void set_rate(int rate) { rate_ = rate; }
            void set_rate(const std::string& s) { set_rate(boost::lexical_cast<int>(s)); }
            void set_slot_time(unsigned slot_time) { slot_time_ = slot_time; }
            void set_slot_time(const std::string& s) { set_slot_time(boost::lexical_cast<unsigned>(s)); } 
            void set_expire_cycles(unsigned expire_cycles) { expire_cycles_ = expire_cycles; }
            void set_expire_cycles(const std::string& s) { set_expire_cycles(boost::lexical_cast<unsigned>(s)); }
            /// \brief Callback to call to request which vehicle id should be the next destination. Typically bound to queue::QueueManager::request_next_destination.
            // 
            // \param func has the form int next_dest(unsigned size). the return value of func should be the next destination id, or -1 for no message to send.
            void set_destination_cb(MACIdFunc func) { callback_dest = func; }
            /// \brief Callback for initiate a tranmission. Typically bound to acomms::ModemDriverBase::initiate_transmission. 
            void set_initiate_transmission_cb(MACMsgFunc1 func) { callback_initiate_transmission = func; }

            /// \brief Callback for initiate ranging ("ping"). Typically bound to acomms::ModemDriverBase::initiate_ranging. 
            void set_initiate_ranging_cb(MACMsgFunc1 func) { callback_initiate_ranging = func; }

//@}

            /// \name Get
            //@{    
            int rate() { return rate_; }
            unsigned slot_time() { return slot_time_; }        
            MACType type() { return type_; }
            //@}


            /// \example libamac/examples/amac_simple/amac_simple.cpp
            /// amac_simple.cpp
        
            /// \example acomms/examples/chat/chat.cpp
        
          private:
            enum { NO_AVAILABLE_DESTINATION = -1 };
            MACIdFunc callback_dest;
            MACMsgFunc1 callback_initiate_transmission;
            MACMsgFunc1 callback_initiate_ranging;

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
            // (bit)-rate id number
            int rate_;
            // size of each slot (seconds)
            unsigned slot_time_;
            // how many cycles before we remove someone?
            unsigned expire_cycles_;
    
            std::ostream* os_;
    
            unsigned modem_id_;
    
            // asynchronous timer
            asio::io_service io_;
            asio::deadline_timer timer_;
            bool timer_is_running_;
    
            boost::posix_time::ptime next_cycle_t_;
            boost::posix_time::ptime next_slot_t_;

            // <id, last time heard from>
            typedef std::multimap<unsigned, Slot>::iterator id2slot_it;

            id2slot_it blank_it_;
            std::list<id2slot_it> slot_order_;
            std::multimap<unsigned, Slot> id2slot_;
    
            std::list<id2slot_it>::iterator current_slot_;
    
            unsigned cycles_since_day_start_;    
    
            // entropy value used to determine how the "blank" slot moves around relative to the values of the modem ids. determining the proper value for this is a bit of work and i will detail when i have time.
            enum { ENTROPY = 5 };
            MACType type_;
        };

        ///
        inline bool operator<(const std::map<unsigned, Slot>::iterator& a, const std::map<unsigned, Slot>::iterator& b)
        { return a->second.src() < b->second.src(); }
    
        /// Are two amac::Slot equal?
        inline bool operator==(const Slot& a, const Slot& b)
        {
            return a.src() == b.src() && a.dest() == b.dest() && a.rate() == b.rate() && a.type() == b.type() && a.slot_time() == b.slot_time() && a.type() == b.type();
        }


    }
}

#endif
