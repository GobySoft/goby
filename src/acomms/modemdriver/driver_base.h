// copyright 2009-2011 t. schneider tes@mit.edu
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
#include <boost/signal.hpp>

#include "goby/acomms/acomms_constants.h"
#include "goby/protobuf/acomms_modem_message.pb.h"
#include "goby/protobuf/acomms_driver_base.pb.h"
#include "goby/util/linebasedcomms.h"

namespace goby
{
    namespace util { class FlexOstream; }    

    namespace acomms
    {
        /// \class ModemDriverBase driver_base.h goby/acomms/modem_driver.h
        /// \ingroup acomms_api
        /// \brief provides an abstract base class for acoustic modem drivers. This is subclassed by the various drivers for different manufacturers' modems.
        /// \sa acomms_driver_base.proto and acomms_modem_message.proto for definition of Google Protocol Buffers messages (namespace goby::acomms::protobuf).
        class ModemDriverBase
        {
          public:
            /// \name Control
            //@{

            /// \brief Starts the modem driver. Must be called before poll(). 
            ///
            /// \param cfg Startup configuration for the driver and modem. DriverConfig is defined in acomms_driver_base.proto. Derived classes can define extensions (see http://code.google.com/apis/protocolbuffers/docs/proto.html#extensions) to DriverConfig to handle modem specific configuration.
            virtual void startup(const protobuf::DriverConfig& cfg) = 0;

            /// \brief Shuts down the modem driver. 
            virtual void shutdown() = 0;

            /// \brief Allows the modem driver to do its work. 
            ///
            /// Should be called regularly to perform the work of the driver as the driver *does not* run in its own thread. This allows us to guarantee that no signals are called except inside this method. Does not block. 
            virtual void do_work() = 0;
            
            //@}

            /// \name MAC Slots
            //@{
            /// \brief Virtual initiate_transmission method. Typically connected to MACManager::signal_initiate_transmission() using bind().
            ///
            /// \param m ModemTransmission (defined in acomms_modem_message.proto) containing the details of the transmission to be started. This may contain data frames. If not, data will be requested when the driver calls the data request signal (ModemDriverBase::signal_data_request)
            virtual void handle_initiate_transmission(const protobuf::ModemTransmission& m) = 0;
            //@}

            /// \name MAC / Queue Signals
            //@{

            /// \brief Called when a binary data transmission is received from the modem
            ///
            /// You should connect one or more slots (a function or member function) to this signal to receive incoming messages. Use the goby::acomms::connect family of functions to do this. This signal will only be called during a call to poll. ModemDataTransmission is defined in acomms_modem_message.proto.
            boost::signal<void (const protobuf::ModemTransmission& message)>
                signal_receive;


            /// \brief Called when a transmission is completed.
            ///
            /// You should connect one or more slots (a function or member function) to this signal to receive incoming messages. Use the goby::acomms::connect family of functions to do this. This signal will only be called during a call to poll. ModemDataTransmission is defined in acomms_modem_message.proto.
            boost::signal<void (const protobuf::ModemTransmission& message)>
                signal_transmit_result;

             /// \brief Called when the modem or modem driver needs data to send. The returned data should be stored in ModemTransmission::frame
            ///
            /// You should connect one or more slots (a function or member function) to this signal to handle data requests. Use the goby::acomms::connect family of functions to do this. This signal will only be called during a call to poll. ModemTransmission is defined in acomms_modem_message.proto.
            boost::signal<void (protobuf::ModemTransmission* msg)>
                signal_data_request;

            /// \brief Called before the modem driver begins processing a transmission. This allows a third party to modify the parameters of the transmission (such as destination or rate) on the fly.
            ///
            /// You may connect one or more slots (a function or member function) to this signal to handle data requests. Use the goby::acomms::connect family of functions to do this. This signal will only be called during a call to poll. ModemTransmission is defined in acomms_modem_message.proto.
            boost::signal<void (protobuf::ModemTransmission* msg_request)>
                signal_modify_transmission;
            

            /// \brief Called after any message is received from the modem by the driver. Used by the MACManager for auto-discovery of vehicles. Also useful for higher level analysis and debugging of the transactions between the driver and the modem.
            ///
            /// If desired, you should connect one or more slots (a function or member function) to this signal to listen on incoming transactions. Use the goby::acomms::connect family of functions to do this. This signal will only be called during a call to poll. ModemRaw is defined in acomms_modem_message.proto.
            boost::signal<void (const protobuf::ModemRaw& msg)>
                signal_raw_incoming;
            
            /// \brief Called after any message is sent from the driver to the modem. Useful for higher level analysis and debugging of the transactions between the driver and the modem.
            ///
            /// If desired, you should connect one or more slots (a function or member function) to this signal to listen on outgoing transactions. Use the goby::acomms::connect family of functions to do this. This signal will only be called during a call to poll. ModemRaw is defined in acomms_modem_message.proto.
            boost::signal<void (const protobuf::ModemRaw& msg)>
                signal_raw_outgoing;
            //@}

            /// Public Destructor
            virtual ~ModemDriverBase();

            
          protected:
            /// \name Constructors/Destructor
            //@{
            
            /// \brief Constructor
            ///
            /// \param log pointer to std::ostream to log human readable debugging and runtime information
            ModemDriverBase();
            
            //@}

            
            /// \name Write/read from the line-based interface to the modem
            //@{
            
            /// \brief write a line to the serial port. 
            ///
            /// \param out reference to string to write. Must already include any end-of-line character(s).
            void modem_write(const std::string& out);

        
            /// \brief read a line from the serial port, including end-of-line character(s)
            ///
            /// \param in pointer to string to store line
            /// \return true if a line was available, false if no line available
            bool modem_read(std::string* in);

            /// \brief start the physical connection to the modem (serial port, TCP, etc.). must be called before ModemDriverBase::modem_read() or ModemDriverBase::modem_write()
            ///
            /// \param cfg Configuration including the parameters for the physical connection. (protobuf::DriverConfig is defined in acomms_driver_base.proto).
            /// \throw ModemDriverException Problem opening the physical connection.
            /// 
            void modem_start(const protobuf::DriverConfig& cfg);

            /// \brief closes the serial port. Use modem_start to reopen the port.
            void modem_close();

            const std::string& glog_out_group() const { return glog_out_group_; }
            const std::string& glog_in_group() const { return glog_in_group_; }
            
            
            //@}

          private:
            // represents the line based communications interface to the modem
            util::LineBasedInterface* modem_;
            static int count_;

            std::string glog_out_group_;
            std::string glog_in_group_;

        };
    }
}
#endif

