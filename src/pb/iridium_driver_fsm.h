// Copyright 2009-2013 Toby Schneider (https://launchpad.net/~tes)
//                     Massachusetts Institute of Technology (2007-)
//                     Woods Hole Oceanographic Institution (2007-)
//                     Goby Developers Team (https://launchpad.net/~goby-dev)
// 
//
// This file is part of the Goby Underwater Autonomy Project Libraries
// ("The Goby Libraries").
//
// The Goby Libraries are free software: you can redistribute them and/or modify
// them under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// The Goby Libraries are distributed in the hope that they will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with Goby.  If not, see <http://www.gnu.org/licenses/>.


#ifndef IridiumModemDriverFSM20130826H
#define IridiumModemDriverFSM20130826H

#include <boost/circular_buffer.hpp>

#include <boost/statechart/event.hpp>
#include <boost/statechart/state_machine.hpp>
#include <boost/statechart/simple_state.hpp>
#include <boost/statechart/state.hpp>
#include <boost/statechart/transition.hpp>
#include <boost/statechart/in_state_reaction.hpp>
#include <boost/statechart/custom_reaction.hpp>

#include <boost/mpl/list.hpp>
#include <iostream>

#include "goby/acomms/protobuf/modem_message.pb.h"
#include "goby/pb/protobuf/iridium_driver.pb.h"

namespace goby
{
    namespace acomms
    {
        namespace fsm
        {
            
            // events
            struct EvRxSerial : boost::statechart::event< EvRxSerial > { std::string line; };
            struct EvTxSerial : boost::statechart::event< EvTxSerial > { };

            struct EvRxOnCallSerial : boost::statechart::event< EvRxOnCallSerial > { std::string line; };
            struct EvTxOnCallSerial : boost::statechart::event< EvTxOnCallSerial > { };
            
            struct EvAck : boost::statechart::event< EvAck >
            {
                EvAck(const std::string& response)
                    : response_(response) { }
                
                std::string response_;
            };
            
            struct EvAtEmpty : boost::statechart::event< EvAtEmpty > {};

            struct EvDial : boost::statechart::event< EvDial > {};
            struct EvRing : boost::statechart::event< EvRing > {};
            struct EvOnline : boost::statechart::event< EvOnline > {};
            struct EvOffline : boost::statechart::event< EvOffline > {};

            struct EvHangup : boost::statechart::event< EvHangup > {};

            struct EvConnect : boost::statechart::event< EvConnect > {};
            struct EvNoCarrier : boost::statechart::event< EvNoCarrier > {};

            struct EvZMQConnect : boost::statechart::event< EvZMQConnect > {};
            struct EvZMQDisconnect : boost::statechart::event< EvZMQDisconnect > {};

            
            struct EvTriplePlus : boost::statechart::event< EvTriplePlus > {};
            
            // pre-declare
            struct Active;
            struct Command;
            struct Configure;
            struct Ready;
            struct Answer;
            struct Dial;

            struct Online;
            struct OnCall;
            struct OnZMQCall;
            struct NotOnCall;

// state machine
            struct IridiumDriverFSM : boost::statechart::state_machine<IridiumDriverFSM, Active > 
            {
              public:
              IridiumDriverFSM(const protobuf::DriverConfig& driver_cfg)
                    : serial_tx_buffer_(SERIAL_BUFFER_CAPACITY),
                    received_(RECEIVED_BUFFER_CAPACITY),
                    driver_cfg_(driver_cfg),
                    data_out_(DATA_BUFFER_CAPACITY),
                    last_rx_tx_time_(0)
                {
                    ++count_;
                    glog_ir_group_ = "iridiumdriver::" + goby::util::as<std::string>(count_);
                }
                
                void buffer_data_out(const goby::acomms::protobuf::ModemTransmission& msg);
                
                // messages for the serial line at next opportunity
                boost::circular_buffer<std::string>& serial_tx_buffer() {return serial_tx_buffer_;}

                // received messages to be passed up out of the ModemDriver
                boost::circular_buffer<protobuf::ModemTransmission>& received() {return received_;}

                // data that should (eventually) be sent out across the Iridium connection
                boost::circular_buffer<protobuf::ModemTransmission>& data_out() {return data_out_;}
                
                const protobuf::DriverConfig& driver_cfg() const {return driver_cfg_;}

                const std::string& glog_ir_group() const { return glog_ir_group_; }                
                double& last_rx_tx_time() { return last_rx_tx_time_; }
                

              private:
                enum  { SERIAL_BUFFER_CAPACITY = 10 };
                enum  { RECEIVED_BUFFER_CAPACITY = 10 };
                
                boost::circular_buffer<std::string> serial_tx_buffer_;
                boost::circular_buffer<protobuf::ModemTransmission> received_;
                const protobuf::DriverConfig& driver_cfg_;

                enum  { DATA_BUFFER_CAPACITY = 5 };
                boost::circular_buffer<protobuf::ModemTransmission> data_out_;

                std::string glog_ir_group_;
                
                static int count_;

                double last_rx_tx_time_;            
            };

            struct Active: boost::statechart::simple_state< Active, IridiumDriverFSM,
                boost::mpl::list<Command, NotOnCall> >
            { };
            
            // Command
            struct Command : boost::statechart::simple_state<Command, Active::orthogonal<0>, Configure>
            {
              public:
                void in_state_react( const EvRxSerial& );
                void in_state_react( const EvTxSerial& );
                void in_state_react( const EvAck & );

              Command()
                  : at_out_(AT_BUFFER_CAPACITY)
                {
                    glog.is(goby::common::logger::DEBUG1) && glog << group("iridiumdriver") << "Command" << std::endl;
                } 
                ~Command() {
                    glog.is(goby::common::logger::DEBUG1) && glog << group("iridiumdriver") << "~Command" << std::endl;
                }

                typedef boost::mpl::list<
                    boost::statechart::in_state_reaction< EvRxSerial, Command, &Command::in_state_react >,
                    boost::statechart::in_state_reaction< EvTxSerial, Command, &Command::in_state_react >,
                    boost::statechart::transition< EvOnline, Online >,
                    boost::statechart::transition< EvOffline, Ready >,
                    boost::statechart::in_state_reaction< EvAck, Command, &Command::in_state_react >
                    > reactions;

                void push_at_command(const std::string& cmd)
                {
                    at_out_.push_back(std::make_pair(0, cmd));
                }
                
                boost::circular_buffer< std::pair<double, std::string> >& at_out() {return at_out_;}

              private:
                enum  { AT_BUFFER_CAPACITY = 100 };
                boost::circular_buffer< std::pair<double, std::string> > at_out_;
                enum { COMMAND_TIMEOUT_SECONDS = 2,
                       DIAL_TIMEOUT_SECONDS = 60,
                       TRIPLE_PLUS_TIMEOUT_SECONDS = 6,
                       ANSWER_TIMEOUT_SECONDS = 30};
            };

            struct Configure : boost::statechart::state<Configure, Command>
            {    
                typedef boost::mpl::list<
                    boost::statechart::transition< EvAtEmpty, Ready >
                    > reactions;
                    
              Configure(my_context ctx) : my_base(ctx)
                {
                    glog.is(goby::common::logger::DEBUG1) && glog << group("iridiumdriver") << "Configure" << std::endl;
                    context<Command>().push_at_command("");
                    context<Command>().push_at_command("E");
                    for(int i = 0,
                            n = context<IridiumDriverFSM>().driver_cfg().ExtensionSize(
                                IridiumDriverConfig::config);
                        i < n; ++i)
                    {
                        context<Command>().push_at_command(
                            context<IridiumDriverFSM>().driver_cfg().GetExtension(
                                IridiumDriverConfig::config, i));
                    }
                }

                ~Configure() {
                    glog.is(goby::common::logger::DEBUG1) && glog << group("iridiumdriver") << "~Configure" << std::endl;
                } 
            };


            struct Ready : boost::statechart::simple_state<Ready, Command>
            {
              public:
                Ready() {
                    glog.is(goby::common::logger::DEBUG1) && glog << group("iridiumdriver") << "Ready" << std::endl;
                } 
                ~Ready() {
                    glog.is(goby::common::logger::DEBUG1) && glog << group("iridiumdriver") << "~Ready" << std::endl;
                }

                void in_state_react( const EvHangup & );
                void in_state_react( const EvTriplePlus & );

                typedef boost::mpl::list<
                    boost::statechart::transition< EvRing, Answer >,
                    boost::statechart::transition< EvDial, Dial >,
                    boost::statechart::in_state_reaction< EvTriplePlus, Ready, &Ready::in_state_react >,
                    boost::statechart::in_state_reaction< EvHangup, Ready, &Ready::in_state_react >
                    > reactions;

              private:
            };

            struct Dial : boost::statechart::state<Dial, Command>
            {
                typedef boost::mpl::list<
                    boost::statechart::custom_reaction< EvNoCarrier >
                    > reactions;
    
              Dial(my_context ctx) : my_base(ctx),
                    dial_attempts_(0)
                {
                    glog.is(goby::common::logger::DEBUG1) && glog << group("iridiumdriver") << "Dial" << std::endl;
                    dial();
                } 
                ~Dial() { glog.is(goby::common::logger::DEBUG1) && glog << group("iridiumdriver") << "~Dial" << std::endl; }

                boost::statechart::result react( const EvNoCarrier& );
                void dial();
                
                
              private:
                int dial_attempts_;
            };

            struct Answer : boost::statechart::state<Answer, Command>
            {
//                typedef boost::mpl::list<
//                    boost::statechart::transition< EvOnline, Online >
//                    > reactions;

              Answer(my_context ctx) : my_base(ctx) 
                {
                    glog.is(goby::common::logger::DEBUG1) && glog << group("iridiumdriver") << "Answer" << std::endl;
                    context<Command>().push_at_command("A");
                }
                ~Answer() { glog.is(goby::common::logger::DEBUG1) && glog << group("iridiumdriver") << "~Answer" << std::endl; } 
            };

// Online
            struct Online : boost::statechart::simple_state<Online, Active::orthogonal<0> >
            {
                
                Online() { glog.is(goby::common::logger::DEBUG1) && glog << group("iridiumdriver") << "Online" << std::endl; }
                ~Online() {
                    glog.is(goby::common::logger::DEBUG1) && glog << group("iridiumdriver") << "~Online" << std::endl;
                }
                
                void in_state_react( const EvRxSerial& );
                void in_state_react( const EvTxSerial& );
                boost::statechart::result react( const EvTriplePlus& );
                boost::statechart::result react( const EvHangup& );

                typedef boost::mpl::list<
                    boost::statechart::transition< EvOffline, Ready >,
                    boost::statechart::custom_reaction< EvHangup >,
                    boost::statechart::custom_reaction< EvTriplePlus>,
                    boost::statechart::in_state_reaction< EvRxSerial, Online, &Online::in_state_react >,
                    boost::statechart::in_state_reaction< EvTxSerial, Online, &Online::in_state_react >
                    > reactions;
                
                
            };


            // Orthogonal on-call / not-on-call
            struct NotOnCall : boost::statechart::simple_state<NotOnCall, Active::orthogonal<1> >
            {
                
                typedef boost::mpl::list<
                    boost::statechart::transition< EvConnect, OnCall >,
                    boost::statechart::transition< EvZMQConnect, OnZMQCall >
                    > reactions;
                
                NotOnCall() { glog.is(goby::common::logger::DEBUG1) && glog << group("iridiumdriver") << "NotOnCall" << std::endl; }
                ~NotOnCall() { glog.is(goby::common::logger::DEBUG1) && glog << group("iridiumdriver") << "~NotOnCall" << std::endl; } 
            };

            struct OnZMQCall : boost::statechart::simple_state<OnZMQCall, Active::orthogonal<1> >
            {
                typedef boost::mpl::list<
                    boost::statechart::transition< EvZMQDisconnect, NotOnCall >
                    > reactions;
                
                OnZMQCall() { glog.is(goby::common::logger::DEBUG1) && glog << group("iridiumdriver") << "OnZMQCall" << std::endl; }
                ~OnZMQCall() { glog.is(goby::common::logger::DEBUG1) && glog << group("iridiumdriver") << "~OnZMQCall" << std::endl; } 
            };

            struct OnCall : boost::statechart::state<OnCall, Active::orthogonal<1> >
            {
              public:

              OnCall(my_context ctx) : my_base(ctx)
                {
                    glog.is(goby::common::logger::DEBUG1) && glog << group("iridiumdriver") << "OnCall" << std::endl;

                    // add a brief identifier that is *different* than the "~" which is what PPP uses
                    // add a carriage return to clear out any garbage
                    // at the *beginning* of transmission
                    context<IridiumDriverFSM>().serial_tx_buffer().push_front("goby\r");

                    context<IridiumDriverFSM>().last_rx_tx_time() = goby::common::goby_time<double>();

                    // connecting necessarily puts the DTE online
                    post_event(EvOnline());
                } 
                ~OnCall() {
                    glog.is(goby::common::logger::DEBUG1) && glog << group("iridiumdriver") << "~OnCall" << std::endl;
                    
                    // disconnecting necessarily puts the DTE offline
                    post_event(EvOffline());
                } 

                void in_state_react( const EvRxOnCallSerial& );
                void in_state_react( const EvTxOnCallSerial& );

                typedef boost::mpl::list<
                    boost::statechart::transition< EvNoCarrier, NotOnCall >,
                    boost::statechart::in_state_reaction< EvRxOnCallSerial, OnCall, &OnCall::in_state_react >,
                    boost::statechart::in_state_reaction< EvTxOnCallSerial, OnCall, &OnCall::in_state_react >
                    > reactions;    

              private:

            };
        }
    }
}


#endif

