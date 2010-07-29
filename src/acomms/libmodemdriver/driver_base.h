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

#include "goby/util/linebasedcomms.h"
#include "goby/acomms/modem_message.h"

namespace goby
{
    namespace util
    {
        class FlexOstream;
    }
    

    namespace acomms
    {
        /// \name Driver Library callback function type definitions
        //@{

        /// \brief boost::function for a function taking a single ModemMessage reference.
        ///
        /// Think of this as a generalized version of a function pointer (bool (*)(const ModemMessage&)). See http://www.boost.org/doc/libs/1_34_0/doc/html/function.html for more on boost:function.    
        typedef boost::function<void (const ModemMessage& message)> DriverMsgFunc1;

        /// \brief boost::function for a function taking a ModemMessage reference as input and filling a ModemMessage reference as output.
        ///
        /// Think of this as a generalized version of a function pointer (bool (*)(const ModemMessage&, ModemMessage&)). See http://www.boost.org/doc/libs/1_34_0/doc/html/function.html for more on boost:function.
        typedef boost::function<bool (const ModemMessage& message1, ModemMessage& message2)> DriverMsgFunc2;

        /// \brief boost::function for a function passed a string.
        ///
        /// Think of this as a generalized version of a function pointer (void (*)(const std::string&)). See http://www.boost.org/doc/libs/1_34_0/doc/html/function.html for more on boost:function.
        typedef boost::function<void (const std::string& s)> DriverStrFunc1;

        /// \brief boost::function for a function taking a unsigned and returning an integer.
        ///
        /// Think of this as a generalized version of a function pointer (int (*) (unsigned)). See http://www.boost.org/doc/libs/1_34_0/doc/html/function.html for more on boost:function.
        typedef boost::function<int (unsigned)> DriverIdFunc;
        
        /// provides a base class for acoustic %modem drivers (i.e. for different manufacturer %modems) to derive
        class ModemDriverBase
        {
          public:
            enum ConnectionType { serial,
                                  tcp_as_client,
                                  tcp_as_server /* unimplemented */,
                                  dual_udp_broadcast /* unimplemented */
            };
            
            /// Virtual startup method. see derived classes (e.g. MMDriver) for examples.
            virtual void startup() = 0;
            /// Virtual do_work method. see derived classes (e.g. MMDriver) for examples.
            virtual void do_work() = 0;
            /// Virtual initiate_transmission method. see derived classes (e.g. MMDriver) for examples.
            virtual void initiate_transmission(const ModemMessage& m) = 0;

            /// Virtual initiate_ranging method. see derived classes (e.g. MMDriver) for examples.
            virtual void initiate_ranging(const ModemMessage& m) = 0;

            /// Virtual request next destination method. Provided a rate (0-5), must return the size of the packet (in bytes) to be used
            virtual int request_next_destination(unsigned rate) = 0;
            
        
            /// Set configuration strings for the %modem. The contents of these strings depends on the specific %modem.
            void set_cfg(const std::vector<std::string>& cfg) { cfg_ = cfg; }

            void set_connection_type(ConnectionType ct) { connection_type_ = ct; }            
            
            /// Set the serial port name (e.g. /dev/ttyS0)
            void set_serial_port(const std::string& s) { serial_port_ = s; }
            /// Set the serial port baud rate (e.g. 19200)
            void set_serial_baud(unsigned u) { serial_baud_ = u; }

            /// Set the tcp server name (e.g. 192.168.1.111 or www.foo.com)
            void set_tcp_server(const std::string& s) { tcp_server_ = s; }

            /// Set the tcp server  (e.g. 5000)
            void set_tcp_port(unsigned u) { tcp_port_ = u; }

            /// \brief Set the callback to receive incoming %modem messages. 
            ///
            /// Any messages received before this callback is set will be discarded.  If using the queue::QueueManager, pass queue::QueueManager::receive_incoming_modem_data to this method.
            /// \param func Pointer to function (or any other object boost::function accepts) matching the signature of ModemDriverMsgFunc1.
            /// The callback (func) will be invoked with the following parameters:
            /// \param message A ModemMessage reference containing the contents of the received %modem message.       
            void set_receive_cb(DriverMsgFunc1 func)     { callback_receive = func; }

            /// \brief Set the callback to receive incoming ranging responses.
            ///
            /// \param func Pointer to function (or any other object boost::function accepts) matching the signature of ModemDriverMsgFunc1.
            /// The callback (func) will be invoked with the following parameters:
            /// \param message A ModemMessage reference containing the contents of the received ranging (in travel time in seconds)
            void set_range_reply_cb(DriverMsgFunc1 func)     { callback_range_reply = func; }
        
            /// \brief Set the callback to receive acknowledgements from the %modem.
            ///
            ///  If using the queue::QueueManager, pass queue::QueueManager::handle_modem_ack to this method.
            /// \param func Pointer to function (or any other object boost::function accepts) matching the signature of ModemDriverMsgFunc1.
            /// The callback (func) will be invoked with the following parameters:
            /// \param message A ModemMessage containing the acknowledgement message
            void set_ack_cb(DriverMsgFunc1 func)         { callback_ack = func; }

            /// \brief Set the callback to handle data requests from the %modem (or the %modem driver, if the %modem does not have this functionality).
            /// 
            /// If using the queue::QueueManager, pass queue::QueueManager::provide_outgoing_modem_data to this method.
            /// \param func Pointer to function (or other boost::function object) of the signature ModemDriverMsgFunc2. The callback (func) will be invoked with the following parameters:
            /// \param message1 (incoming) The ModemMessage containing the details of the request (source, destination, size, etc.)
            /// \param message2 (outgoing) The ModemMessage to be sent. This should be populated by the callback.
            void set_datarequest_cb(DriverMsgFunc2 func) { callback_datarequest = func; }

            /// \brief Set the callback to pass all parsed messages to (i.e. ModemMessage representation of the serial line)
            ///
            ///  If using the amac::MACManager, pass amac::MACManager::process_message to this method.
            /// \param func Pointer to function (or any other object boost::function accepts) matching the signature of ModemDriverMsgFunc1.
            /// The callback (func) will be invoked with the following parameters:
            /// \param message A ModemMessage containing the received modem message
            void set_in_parsed_cb(DriverMsgFunc1 func)   { callback_decoded = func; }
        
            /// \brief Set the callback to pass raw incoming modem strings to.
            ///
            ///  
            /// \param func Pointer to function (or any other object boost::function accepts) matching the signature of ModemStrFunc1.
            /// The callback (func) will be invoked with the following parameters:
            /// \param std::string The raw incoming string
            void set_in_raw_cb(DriverStrFunc1 func)      { callback_in_raw = func; }

            /// \brief Set the callback to pass all raw outgoing modem strings to.
            ///
            ///  
            /// \param func Pointer to function (or any other object boost::function accepts) matching the signature of ModemStrFunc1.
            /// The callback (func) will be invoked with the following parameters:
            /// \param std::string The raw outgoing string
            void set_out_raw_cb(DriverStrFunc1 func)     { callback_out_raw = func; }

            /// \brief Callback to call to request which vehicle id should be the next destination. Typically bound to queue::QueueManager::request_next_destination.
            // 
            // \param func has the form int next_dest(unsigned rate). the return value of func should be the next destination id, or -1 for no message to send.
            void set_destination_cb(DriverIdFunc func) { callback_dest = func; }

            ConnectionType connection_type() { return connection_type_; }            
            
            /// \return the serial port name
            std::string serial_port() { return serial_port_; }
            
            /// \return the serial port baud rate
            unsigned serial_baud() { return serial_baud_; }

            /// \return the tcp server name or IP address
            std::string tcp_server() { return tcp_server_; }
            
            /// \return the serial port baud rate
            unsigned tcp_port() { return tcp_port_; }

            void add_flex_groups(util::FlexOstream& tout);
            
            
          protected:
            /// \brief Constructor
            ///
            /// \param out pointer to std::ostream to log human readable debugging and runtime information
            /// \param line_delimiter string indicating the end-of-line character(s) from the serial port (usually newline ("\n") or carriage-return and newline ("\r\n"))
            /// \param connection_type enumeration indicating the type of physical connection to the modem
            ModemDriverBase(std::ostream* log = 0, const std::string& line_delimiter = "\r\n");
            /// Destructor
            ~ModemDriverBase();

            /// \brief write a line to the serial port. 
            ///
            /// \param out reference to string to write. Must already include any end-of-line character(s).
            void modem_write(const std::string& out);

        
            /// \brief read a line from the serial port, including end-of-line character(s)
            ///
            /// \param in reference to string to store line
            /// \return true if a line was available, false if no line available
            bool modem_read(std::string& in);

            /// \brief start the serial port. must be called before DriverBase::modem_read() or DriverBase::modem_write()
            ///
            /// \throw std::runtime_error Serial port could not be opened.
            void modem_start();

            /// vector containing the configuration parameters intended to be set during ::startup()
            std::vector<std::string> cfg_; 

            DriverMsgFunc1 callback_receive;
            DriverMsgFunc1 callback_range_reply;
            DriverMsgFunc1 callback_ack;
            DriverMsgFunc2 callback_datarequest;
            DriverMsgFunc1 callback_decoded;
    
            DriverStrFunc1 callback_in_raw;
            DriverStrFunc1 callback_out_raw;

            DriverIdFunc callback_dest;

        
          private:
            unsigned serial_baud_;
            std::string serial_port_;

            unsigned tcp_port_;
            std::string tcp_server_;
            
            std::string line_delimiter_;
            
            std::ostream* log_;

            // represents the line based communications interface to the modem
            util::LineBasedInterface* modem_;

            // identifies us to the singleton class controller the modem interface (SerialClient, etc.)
            ConnectionType connection_type_;
        };
    }
}
#endif

