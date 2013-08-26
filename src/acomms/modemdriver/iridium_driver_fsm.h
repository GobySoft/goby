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
#include "goby/acomms/protobuf/iridium_driver.pb.h"

namespace goby
{
    namespace acomms
    {
        namespace fsm
        {
            // events
            struct EvRxSerial : boost::statechart::event< EvRxSerial >
            {
                std::string line;
            };
            struct EvTxSerial : boost::statechart::event< EvTxSerial > { };
            
            struct EvOk : boost::statechart::event< EvOk > {};

            struct EvDial : boost::statechart::event< EvDial > {};
            struct EvNoCarrier : boost::statechart::event< EvNoCarrier > {};
            struct EvConnect : boost::statechart::event< EvConnect > {};
            struct EvRing : boost::statechart::event< EvRing > {};
            struct EvATA : boost::statechart::event< EvATA > {};
            struct EvATH0 : boost::statechart::event< EvATH0 > {};

            struct EvTriplePlus : boost::statechart::event< EvTriplePlus > {};
            struct EvATO : boost::statechart::event< EvATO > {};

            struct EvReturnToReady  : boost::statechart::event< EvReturnToReady > {};
            struct EvConfigured  : boost::statechart::event< EvConfigured > {};
            
            struct EvSendData : boost::statechart::event< EvSendData > 
            {
                goby::acomms::protobuf::ModemTransmission msg;
            };
            
            // pre-declare
            struct Command;
            struct Configure;
            struct Ready;
            struct Answer;
            struct Dial;

            struct Online;
            struct OnCall;
            struct NotOnCall;

// state machine
            struct IridiumDriverFSM : boost::statechart::state_machine<IridiumDriverFSM, Command> 
            {
              public:
                IridiumDriverFSM(const protobuf::DriverConfig& driver_cfg)
                    : driver_cfg_(driver_cfg)
                { }
                
                std::deque<std::string>& send() {return send_;}
                std::deque<protobuf::ModemTransmission>& received() {return received_;}
                const protobuf::DriverConfig& driver_cfg() const {return driver_cfg_;}

              private:
                std::deque<std::string> send_;
                std::deque<protobuf::ModemTransmission> received_;
                const protobuf::DriverConfig& driver_cfg_;
            };

            // Command
            struct Command : boost::statechart::simple_state<Command, IridiumDriverFSM, Configure>
            {
              public:
                void in_state_react( const EvRxSerial& );
                void in_state_react( const EvTxSerial& );

                Command() {
                    std::cout << "Command\n";
                } 
                ~Command() {
                    std::cout << "~Command\n";
                    // Clear any AT messages we were trying to send
                    at_out_.clear();
                }

                typedef boost::mpl::list<
                    boost::statechart::in_state_reaction< EvRxSerial, Command, &Command::in_state_react >,
                    boost::statechart::in_state_reaction< EvTxSerial, Command, &Command::in_state_react >
                    > reactions;
                
                std::deque<std::string>& at_out() {return at_out_;}

              private:
                std::deque<std::string> at_out_;
            };

            struct Configure : boost::statechart::state<Configure, Command>
            {
    
                typedef boost::mpl::list<
                    boost::statechart::transition< EvConfigured, Ready >
                    > reactions;
    
              Configure(my_context ctx) : my_base(ctx)
                {
                    std::cout << "Configure\n";
                }

                ~Configure() {
                    std::cout << "~Configure\n";
                    context<Command>().at_out().push_back("");
                    context<Command>().at_out().push_back("E");
                    for(int i = 0,
                            n = context<IridiumDriverFSM>().driver_cfg().ExtensionSize(
                                IridiumDriverConfig::config);
                        i < n; ++i)
                    {
                        context<Command>().at_out().push_back(
                            context<IridiumDriverFSM>().driver_cfg().GetExtension(
                                IridiumDriverConfig::config, i));
                    }
                } // exit
            };


            struct Ready : boost::statechart::simple_state<Ready, Command>
            {
              public:
                Ready() {
                    std::cout << "Ready\n";
                } 
                ~Ready() {
                    std::cout << "~Ready\n";
                }

                void in_state_react( const EvOk & );
                void in_state_react( const EvSendData & )
                {
                    post_event(EvDial());
                }            

                typedef boost::mpl::list<
                    boost::statechart::transition< EvRing, Answer >,
                    boost::statechart::transition< EvDial, Dial >,        
                    boost::statechart::transition< EvATO, OnCall >,
                    boost::statechart::in_state_reaction< EvOk, Ready, &Ready::in_state_react >,
                    boost::statechart::in_state_reaction< EvSendData, Ready, &Ready::in_state_react>
                    > reactions;

              private:
            };

            struct Dial : boost::statechart::state<Dial, Command>
            {
                typedef boost::mpl::list<
                    boost::statechart::transition< EvConnect, OnCall >,
                    boost::statechart::transition< EvNoCarrier, Dial >
                    > reactions;
    
              Dial(my_context ctx) : my_base(ctx)
                {
                    std::cout << "Dial\n";
                    context<Command>().at_out().push_back(
                        "D" +
                        context<IridiumDriverFSM>().driver_cfg().GetExtension(
                            IridiumDriverConfig::remote).iridium_number());
                } // entry
                ~Dial() { std::cout << "~Dial\n"; } // exit
            };

            struct Answer : boost::statechart::state<Answer, Command>
            {
                typedef boost::mpl::list<
                    boost::statechart::transition< EvATA, OnCall >
                    > reactions;

              Answer(my_context ctx) : my_base(ctx) 
                {
                    std::cout << "Answer\n";
                    context<Command>().at_out().push_back("A");
                }
                ~Answer() { std::cout << "~Answer\n"; } // exit
            };

// Online

            struct Online :  boost::statechart::simple_state<Online, IridiumDriverFSM, NotOnCall>
            {
            };

            struct NotOnCall : boost::statechart::state<NotOnCall, Online>
            {
    
                typedef boost::mpl::list<
                    boost::statechart::transition< EvReturnToReady, Ready >
                    > reactions;
    
              NotOnCall(my_context ctx) : my_base(ctx)
                {
                    std::cout << "NotOnCall\n";
                    post_event(EvReturnToReady());
                }

                ~NotOnCall() { std::cout << "~NotOnCall\n"; } // exit
            };


            struct OnCall : boost::statechart::state<OnCall, Online>
            {
              public:

              OnCall(my_context ctx) : my_base(ctx)
                {
                    std::cout << "OnCall\n";
                    data_out_.push_back("\r");
                } // entry
                ~OnCall() { std::cout << "~OnCall\n"; } // exit

                void in_state_react( const EvRxSerial& );
                void in_state_react( const EvSendData& e);
                void in_state_react( const EvTxSerial& );

                typedef boost::mpl::list<
                    boost::statechart::transition< EvATH0, NotOnCall >,
                    boost::statechart::in_state_reaction< EvRxSerial, OnCall, &OnCall::in_state_react >,
                    boost::statechart::in_state_reaction< EvTxSerial, OnCall, &OnCall::in_state_react >,
                    boost::statechart::in_state_reaction< EvSendData, OnCall, &OnCall::in_state_react >
                    > reactions;    

              private:
                std::deque<std::string> data_out_;
            };
        }
    }
}


#endif

