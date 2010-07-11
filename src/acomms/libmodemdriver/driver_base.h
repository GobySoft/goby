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
    /// \name Driver Library callback function type definitions
    //@{

    /// \brief boost::function for a function taking a single modem::Message reference.
    ///
    /// Think of this as a generalized version of a function pointer (bool (*)(const modem::Message&)). See http://www.boost.org/doc/libs/1_34_0/doc/html/function.html for more on boost:function.    
    typedef boost::function<void (const modem::Message& message)> MsgFunc1;

    /// \brief boost::function for a function taking a modem::Message reference as input and filling a modem::Message reference as output.
    ///
    /// Think of this as a generalized version of a function pointer (bool (*)(const modem::Message&, modem::Message&)). See http://www.boost.org/doc/libs/1_34_0/doc/html/function.html for more on boost:function.
    typedef boost::function<bool (const modem::Message& message1, modem::Message& message2)> MsgFunc2;

    /// \brief boost::function for a function passed a string.
    ///
    /// Think of this as a generalized version of a function pointer (void (*)(const std::string&)). See http://www.boost.org/doc/libs/1_34_0/doc/html/function.html for more on boost:function.
    typedef boost::function<void (const std::string& s)> StrFunc1;

    /// provides a base class for acoustic %modem drivers (i.e. for different manufacturer %modems) to derive
    class DriverBase
    {
      public:

        /// Virtual startup method. see derived classes (e.g. micromodem::MMDriver) for examples.
        virtual void startup() = 0;
        /// Virtual do_work method. see derived classes (e.g. micromodem::MMDriver) for examples.
        virtual void do_work() = 0;
        /// Virtual initiate_transmission method. see derived classes (e.g. micromodem::MMDriver) for examples.
        virtual void initiate_transmission(const modem::Message& m) = 0;

        /// Virtual initiate_ranging method. see derived classes (e.g. micromodem::MMDriver) for examples.
        virtual void initiate_ranging(const modem::Message& m) = 0;

        
        /// Set configuration strings for the %modem. The contents of these strings depends on the specific %modem.
        void set_cfg(const std::vector<std::string>& cfg) { cfg_ = cfg; }

        /// Set the serial port name (e.g. /dev/ttyS0)
        void set_serial_port(const std::string& s) { serial_port_ = s; }
        /// Set the serial port baud rate (e.g. 19200)
        void set_baud(unsigned u) { baud_ = u; }

        /// \brief Set the callback to receive incoming %modem messages. 
        ///
        /// Any messages received before this callback is set will be discarded.  If using the queue::QueueManager, pass queue::QueueManager::receive_incoming_modem_data to this method.
        /// \param func Pointer to function (or any other object boost::function accepts) matching the signature of modem::MsgFunc1.
        /// The callback (func) will be invoked with the following parameters:
        /// \param message A modem::Message reference containing the contents of the received %modem message.       
        void set_receive_cb(MsgFunc1 func)     { callback_receive = func; }

        /// \brief Set the callback to receive incoming ranging responses.
        ///
        /// \param func Pointer to function (or any other object boost::function accepts) matching the signature of modem::MsgFunc1.
        /// The callback (func) will be invoked with the following parameters:
        /// \param message A modem::Message reference containing the contents of the received ranging (in travel time in seconds)
        void set_range_reply_cb(MsgFunc1 func)     { callback_range_reply = func; }
        
        /// \brief Set the callback to receive acknowledgements from the %modem.
        ///
        ///  If using the queue::QueueManager, pass queue::QueueManager::handle_modem_ack to this method.
        /// \param func Pointer to function (or any other object boost::function accepts) matching the signature of modem::MsgFunc1.
        /// The callback (func) will be invoked with the following parameters:
        /// \param message A modem::Message containing the acknowledgement message
        void set_ack_cb(MsgFunc1 func)         { callback_ack = func; }

        /// \brief Set the callback to handle data requests from the %modem (or the %modem driver, if the %modem does not have this functionality).
        /// 
        /// If using the queue::QueueManager, pass queue::QueueManager::provide_outgoing_modem_data to this method.
        /// \param func Pointer to function (or other boost::function object) of the signature modem::MsgFunc2. The callback (func) will be invoked with the following parameters:
        /// \param message1 (incoming) The modem::Message containing the details of the request (source, destination, size, etc.)
        /// \param message2 (outgoing) The modem::Message to be sent. This should be populated by the callback.
        void set_datarequest_cb(MsgFunc2 func) { callback_datarequest = func; }

        /// \brief Set the callback to pass all parsed messages to (i.e. modem::Message representation of the serial line)
        ///
        ///  If using the amac::MACManager, pass amac::MACManager::process_message to this method.
        /// \param func Pointer to function (or any other object boost::function accepts) matching the signature of modem::MsgFunc1.
        /// The callback (func) will be invoked with the following parameters:
        /// \param message A modem::Message containing the received modem message
        void set_in_parsed_cb(MsgFunc1 func)   { callback_decoded = func; }
        
        /// \brief Set the callback to pass raw incoming modem strings to.
        ///
        ///  
        /// \param func Pointer to function (or any other object boost::function accepts) matching the signature of modem::StrFunc1.
        /// The callback (func) will be invoked with the following parameters:
        /// \param std::string The raw incoming string
        void set_in_raw_cb(StrFunc1 func)      { callback_in_raw = func; }

        /// \brief Set the callback to pass all raw outgoing modem strings to.
        ///
        ///  
        /// \param func Pointer to function (or any other object boost::function accepts) matching the signature of modem::StrFunc1.
        /// The callback (func) will be invoked with the following parameters:
        /// \param std::string The raw outgoing string
        void set_out_raw_cb(StrFunc1 func)     { callback_out_raw = func; }


        /// \return the serial port name
        std::string serial_port() { return serial_port_; }

        /// \return the serial port baud rate
        unsigned baud() { return baud_; }
        
        
      protected:
        /// \brief Constructor
        ///
        /// \param out pointer to std::ostream to log human readable debugging and runtime information
        /// \param line_delimiter string indicating the end-of-line character(s) from the serial port (usually newline ("\n") or carriage-return and newline ("\r\n"))
        DriverBase(std::ostream* out, const std::string& line_delimiter);
        /// Destructor
        ~DriverBase() { }

        /// \brief write a line to the serial port. 
        ///
        /// \param out reference to string to write. Must already include any end-of-line character(s).
        void serial_write(const std::string& out);

        
        /// \brief read a line from the serial port, including end-of-line character(s)
        ///
        /// \param in reference to string to store line
        /// \return true if a line was available, false if no line available
        bool serial_read(std::string& in);

        /// \brief start the serial port. must be called before DriverBase::serial_read() or DriverBase::serial_write()
        ///
        /// \throw std::runtime_error Serial port could not be opened.
        void serial_start();

        /// vector containing the configuration parameters intended to be set during ::startup()
        std::vector<std::string> cfg_; 

        MsgFunc1 callback_receive;
        MsgFunc1 callback_range_reply;
        MsgFunc1 callback_ack;
        MsgFunc2 callback_datarequest;
        MsgFunc1 callback_decoded;
    
        StrFunc1 callback_in_raw;
        StrFunc1 callback_out_raw;

      private:
        unsigned baud_;
        std::string serial_port_;
        std::string serial_delimiter_;
        
        std::ostream* os_;
        
        std::deque<std::string> in_;

        // serial port io service
        boost::mutex in_mutex_;
        serial::SerialClient* serial_;
    };

}

#endif

