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
#include "goby/protobuf/modem_message.pb.h"
#include "goby/protobuf/driver_base.pb.h"
#include "goby/util/linebasedcomms.h"
#include "goby/protobuf/acomms_proto_helpers.h"

namespace goby
{
    namespace util { class FlexOstream; }    

    namespace acomms
    {         
        /// provides an abstract base class for acoustic modem drivers. This is subclassed by the various drivers for different manufacturers' modems.
        class ModemDriverBase
        {
          public:
            /// \name Pure virtual
            //@{

            /// \brief Virtual startup method. see derived classes (e.g. MMDriver) for examples.
            ///
            /// \param cfg Startup configuration for the driver and modem. DriverConfig is defined in driver_base.proto. Derived classes can define extensions (see http://code.google.com/apis/protocolbuffers/docs/proto.html#extensions) to DriverConfig to handle modem specific configuration.
            virtual void startup(const protobuf::DriverConfig& cfg) = 0;

            virtual void shutdown() = 0;

            /// \brief Virtual do_work method. See derived classes (e.g. MMDriver) for examples.
            ///
            /// Should be called regularly to perform the work of the driver as the driver *does not* run in its own thread. This allows us to guarantee that no signals are called except inside this do_work method.
            virtual void do_work() = 0;
            /// \brief Virtual initiate_transmission method. See derived classes (e.g. MMDriver) for examples.
            ///
            /// \param m ModemMsgBase (defined in modem_message.proto) containing the details of the transmission to be started. This does *not* contain data, which will be requested when the driver calls the data request signal (ModemDriverBase::signal_data_request)
            virtual void handle_initiate_transmission(protobuf::ModemMsgBase* m) = 0;            
            /// \brief Virtual initiate_ranging method. see derived classes (e.g. MMDriver) for examples.
            ///
            /// \param m ModemRangingRequest (defined in modem_message.proto) containing the details of the ranging request to be started: source, destination, type, etc.
            virtual void handle_initiate_ranging(protobuf::ModemRangingRequest* m) = 0;
            
            //@}

            /// \name Signals
            //@{

            /// \brief Called when a binary data transmission is received from the modem
            ///
            /// You should connect one or more slots (a function or member function) to this signal to receive incoming messages. Use the goby::acomms::connect family of functions to do this. This signal will only be called during a call to do_work. ModemDataTransmission is defined in modem_message.proto.
            boost::signal<void (const protobuf::ModemDataTransmission& message)>
                signal_receive;

            /// \brief Called when the modem or modem driver needs data to send. msg_request contains details on the data request, and the returned data should be stored in msg_data.
            ///
            /// You should connect one or more slots (a function or member function) to this signal to handle data requests. Use the goby::acomms::connect family of functions to do this. This signal will only be called during a call to do_work. ModemDataRequest and ModemDataTransmission are defined in modem_message.proto.
            boost::signal<void (const protobuf::ModemDataRequest& msg_request, protobuf::ModemDataTransmission* msg_data)>
                signal_data_request;

            
            /// \brief Called when the modem receives ranging information (time of flight to another vehicle or LBL ranging beacons)
            ///
            /// You should connect one or more slots (a function or member function) to this signal to handle ranging replies. Use the goby::acomms::connect family of functions to do this. This signal will only be called during a call to do_work. ModemRangingReply is defined in modem_message.proto.
            boost::signal<void (const protobuf::ModemRangingReply& message)>
                signal_range_reply;

            /// \brief Called when the modem receives an acknowledgment of proper receipt of a prior data transmission. The frame number of the acknowledgment must match the frame number of the original message.
            ///
            /// You should connect one or more slots (a function or member function) to this signal to handle acknowledgments. Use the goby::acomms::connect family of functions to do this. This signal will only be called during a call to do_work. ModemDataAck is defined in modem_message.proto.
            boost::signal<void (const protobuf::ModemDataAck& message)>
                signal_ack;

            /// \brief Called after any message is received from the modem by the driver. Useful for higher level analysis and debugging of the transactions between the driver and the modem.
            ///
            /// If desired, you should connect one or more slots (a function or member function) to this signal to listen on incoming transactions. Use the goby::acomms::connect family of functions to do this. This signal will only be called during a call to do_work. ModemMsgBase is defined in modem_message.proto.
            boost::signal<void (const protobuf::ModemMsgBase& msg_data)>
                signal_all_incoming;
            
            /// \brief Called after any message is sent from the driver to the modem. Useful for higher level analysis and debugging of the transactions between the driver and the modem.
            ///
            /// If desired, you should connect one or more slots (a function or member function) to this signal to listen on outgoing transactions. Use the goby::acomms::connect family of functions to do this. This signal will only be called during a call to do_work. ModemMsgBase is defined in modem_message.proto.
            boost::signal<void (const protobuf::ModemMsgBase& msg_data)>
                signal_all_outgoing;
            //@}

            /// \name Static helpers
            //@{
            /// \brief Set the output groups for the modem driver if using the util::FlexOstream class for human readable debugging out. Setting groups allows the util::FlexOstream class to differentiate between different types of debugging messages.
            ///
            /// \param tout pointer to util::FlexOstream stream object to add groups to.
            static void add_flex_groups(util::FlexOstream* tout);
            //@}
            
          protected:
            /// \name Constructors/Destructor
            //@{
            
            /// \brief Constructor
            ///
            /// \param log pointer to std::ostream to log human readable debugging and runtime information
            /// \param line_delimiter string indicating the end-of-line character(s) from the serial port (usually newline ("\n") or carriage-return and newline ("\r\n"))
            ModemDriverBase(std::ostream* log = 0);
            /// Destructor
            ~ModemDriverBase();

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

            /// \brief start the serial port. must be called before DriverBase::modem_read() or DriverBase::modem_write()
            ///
            /// \throw std::runtime_error Serial port could not be opened.
            void modem_start(const protobuf::DriverConfig& cfg);

            /// \brief closes the serial port. Use modem_start to reopen the port.
            void modem_close();

            
            //@}

          private:
            std::ostream* log_;

            // represents the line based communications interface to the modem
            util::LineBasedInterface* modem_;
        };
    }
}
#endif

