// copyright 2009 t. schneider tes@mit.edu
// 
// this file is part of the goby-acomms acoustic modem driver.
// goby-acomms is a collection of libraries 
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

#ifndef DriverBase20091214H
#define DriverBase20091214H

#include <boost/thread.hpp>
#include "asio.hpp"

#include "util/serial.h"
#include "acomms/modem_message.h"

namespace modem
{    
    class DriverBase
    {
      public:
        DriverBase(std::ostream* out, const std::string& line_delimiter);
        ~DriverBase() { }

        virtual void startup() = 0;
        virtual void do_work() = 0;
        virtual void initiate_transmission(const modem::Message& m) = 0;

        void set_cfg(const std::vector<std::string>& cfg) { cfg_ = cfg; }

        void set_serial_port(const std::string& s) { serial_port_ = s; }
        void set_baud(unsigned u) { baud_ = u; }
        
        typedef boost::function<bool (modem::Message& message)> MsgFunc1;
        typedef boost::function<bool (modem::Message& message1, modem::Message& message2)> MsgFunc2;
        typedef boost::function<void (const std::string& s)> StrFunc1;

        void set_receive_cb(MsgFunc1 func)     { callback_receive = func; }
        void set_ack_cb(MsgFunc1 func)         { callback_ack = func; }
        void set_datarequest_cb(MsgFunc2 func) { callback_datarequest = func; }
        void set_in_raw_cb(StrFunc1 func)      { callback_in_raw = func; }
        void set_out_raw_cb(StrFunc1 func)     { callback_out_raw = func; }
        void set_in_parsed_cb(MsgFunc1 func)   { callback_decoded = func; }

        unsigned baud() { return baud_; }

        // would like this to be protected,
        // but higher level still demands direct access to serial port
        void serial_write(const std::string& out);
      protected:
        bool serial_read(std::string& in);
        void serial_start();
        
        // callbacks to the parent class
        MsgFunc1 callback_receive;
        MsgFunc1 callback_ack;
        MsgFunc2 callback_datarequest;
        MsgFunc1 callback_decoded;
    
        StrFunc1 callback_in_raw;
        StrFunc1 callback_out_raw;

        std::vector<std::string> cfg_; // what config params do we need to send?      
      private:
        unsigned baud_;
        std::string serial_port_;
        std::string serial_delimiter_;
        
        std::ostream* os_;
        
        std::deque<std::string> in_;

        // serial port io service
        asio::io_service io_;
        boost::mutex in_mutex_;
        SerialClient serial_;
    };

}

#endif

