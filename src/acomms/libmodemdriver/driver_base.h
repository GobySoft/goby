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

#include "goby/acomms/acomms_constants.h"
#include "goby/util/linebasedcomms.h"
#include "goby/acomms/protobuf/acomms_proto_helpers.h"

namespace goby
{
    namespace util
    {
        class FlexOstream;
    }
    

    namespace acomms
    {
        /// provides a base class for acoustic %modem drivers (i.e. for different manufacturer %modems) to derive
        class ModemDriverBase
        {
          public:
            enum ConnectionType { CONNECTION_SERIAL,
                                  CONNECTION_TCP_AS_CLIENT,
                                  CONNECTION_TCP_AS_SERVER, 
                                  CONNECTION_DUAL_UDP_BROADCAST /* unimplemented */
            };
            
            /// Virtual startup method. see derived classes (e.g. MMDriver) for examples.
            virtual void startup() = 0;
            /// Virtual do_work method. see derived classes (e.g. MMDriver) for examples.
            virtual void do_work() = 0;
            /// Virtual initiate_transmission method. see derived classes (e.g. MMDriver) for examples.
            virtual void handle_initiate_transmission(const protobuf::ModemMsgBase& m) = 0;
            
            /// Virtual initiate_ranging method. see derived classes (e.g. MMDriver) for examples.
            virtual void handle_initiate_ranging(const protobuf::ModemRangingRequest& m) = 0;
            
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
            /// \param func Pointer to function (or any other object boost::function accepts) matching the signature of ModemDriverMsgFunc.
            /// The callback (func) will be invoked with the following parameters:
            /// \param message A ModemMessage reference containing the contents of the received %modem message.       
            void set_callback_receive(boost::function<void (const protobuf::ModemDataTransmission& message)> func)
            { callback_receive = func; }
            
            
            /// \brief Set the callback to receive incoming ranging responses.
            ///
            /// \param func Pointer to function (or any other object boost::function accepts) matching the signature of ModemDriverMsgFunc.
            /// The callback (func) will be invoked with the following parameters:
            /// \param message A ModemMessage reference containing the contents of the received ranging (in travel time in seconds)
            void set_callback_range_reply(boost::function<void (const protobuf::ModemRangingReply& message)> func)
            { callback_range_reply = func; }

            
            /// \brief Set the callback to receive acknowledgements from the %modem.
            ///
            ///  If using the queue::QueueManager, pass queue::QueueManager::handle_modem_ack to this method.
            /// \param func Pointer to function (or any other object boost::function accepts) matching the signature of ModemDriverMsgFunc.
            /// The callback (func) will be invoked with the following parameters:
            /// \param message A ModemMessage containing the acknowledgement message
            void set_callback_ack(boost::function<void (const protobuf::ModemDataAck& message)> func)
            { callback_ack = func; }

            /// \brief Set the callback to handle data requests from the %modem (or the %modem driver, if the %modem does not have this functionality).
            /// 
            /// If using the queue::QueueManager, pass queue::QueueManager::provide_outgoing_modem_data to this method.
            /// \param func Pointer to function (or other boost::function object) of the signature MutableDriverMsgFunc. The callback (func) will be invoked with the following parameters:
            /// \param message1 (incoming) The ModemMessage containing the details of the request (source, destination, size, etc.)
            /// \param message2 (outgoing) The ModemMessage to be sent. This should be populated by the callback.
            void set_callback_data_request(boost::function<void (const protobuf::ModemDataRequest& request_msg, protobuf::ModemDataTransmission* data_mgs)> func)
            { callback_data_request = func; }
            
            
            /// \brief Set the callback to pass all parsed messages to (i.e. ModemMessage representation of the serial line)
            ///
            ///  If using the amac::MACManager, pass amac::MACManager::process_message to this method.
            /// \param func Pointer to function (or any other object boost::function accepts) matching the signature of ModemDriverMsgFunc.
            /// The callback (func) will be invoked with the following parameters:
            /// \param message A ModemMessage containing the received modem message
            void set_callback_all_incoming(boost::function<void (const protobuf::ModemMsgBase& msg)> func)
            { callback_all_incoming = func; }

            void set_callback_all_outgoing(boost::function<void (const protobuf::ModemMsgBase& msg)> func)
            { callback_all_outgoing = func; }        
            
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
            
            // templated overloads of the callback set methods
            // to make binding of member functions simpler
            template<typename V, typename A1>
                void set_callback_out_raw(void(V::*mem_func)(A1), V* obj)
            { set_callback_out_raw(boost::bind(mem_func, obj, _1)); }
            template<typename V, typename A1>
                void set_callback_in_raw(void(V::*mem_func)(A1), V* obj)
            { set_callback_in_raw(boost::bind(mem_func, obj, _1)); }
            template<typename V, typename A1>
                void set_callback_all_incoming(void(V::*mem_func)(A1), V* obj)
            { set_callback_all_incoming(boost::bind(mem_func, obj, _1)); }
            template<typename V, typename A1>
                void set_callback_all_outgoing(void(V::*mem_func)(A1), V* obj)
            { set_callback_all_incoming(boost::bind(mem_func, obj, _1)); }
            template<typename V, typename A1, typename A2>
                void set_callback_data_request(void(V::*mem_func)(A1, A2), V* obj)
            { set_callback_data_request(boost::bind(mem_func, obj, _1, _2)); }
            template<typename V, typename A1>
                void set_callback_ack(void(V::*mem_func)(A1), V* obj)
            { set_callback_ack(boost::bind(mem_func, obj, _1)); }
            template<typename V, typename A1>
                void set_callback_range_reply(void(V::*mem_func)(A1), V* obj)
            { set_callback_range_reply(boost::bind(mem_func, obj, _1)); }
            template<typename V, typename A1>
                void set_callback_receive(void(V::*mem_func)(A1), V* obj)
            { set_callback_receive(boost::bind(mem_func, obj, _1)); }            
            
          
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
            
            boost::function<void (const protobuf::ModemDataTransmission& message)> callback_receive;
            boost::function<void (const protobuf::ModemDataRequest& msg_request, protobuf::ModemDataTransmission* msg_data)> callback_data_request;
            
            boost::function<void (const protobuf::ModemRangingReply& message)> callback_range_reply;
            boost::function<void (const protobuf::ModemDataAck& message)> callback_ack;

            boost::function<void (const protobuf::ModemMsgBase& msg_data)> callback_all_incoming;
            boost::function<void (const protobuf::ModemMsgBase& msg_data)> callback_all_outgoing;
            
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

