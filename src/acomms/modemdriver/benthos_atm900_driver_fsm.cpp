// Copyright 2009-2016 Toby Schneider (http://gobysoft.org/index.wt/people/toby)
//                     GobySoft, LLC (2013-)
//                     Massachusetts Institute of Technology (2007-2014)
//                     Community contributors (see AUTHORS file)
//
//
// This file is part of the Goby Underwater Autonomy Project Libraries
// ("The Goby Libraries").
//
// The Goby Libraries are free software: you can redistribute them and/or modify
// them under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 2.1 of the License, or
// (at your option) any later version.
//
// The Goby Libraries are distributed in the hope that they will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with Goby.  If not, see <http://www.gnu.org/licenses/>.

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string.hpp>

#include "goby/common/logger.h"
#include "goby/common/time.h"
#include "goby/util/binary.h"

#include "rudics_packet.h"
#include "benthos_atm900_driver_fsm.h"

using goby::glog;
using namespace goby::common::logger;
using goby::common::goby_time;

int goby::acomms::benthos_fsm::BenthosATM900FSM::count_ = 0;

void goby::acomms::benthos_fsm::BenthosATM900FSM::buffer_data_out( const goby::acomms::protobuf::ModemTransmission& msg)
{
    data_out_.push_back(msg);
}




void goby::acomms::benthos_fsm::Active::in_state_react(const EvRxSerial& e)
{
    std::string in = e.line;
    boost::trim(in);

    static const std::string data = "DATA";
    static const std::string user = "user:";

    if(in.compare(0, data.size(), data) == 0)
    {
        // data
        post_event(EvReceive(in));
    }
    else if(in.compare(0, user.size(), user) == 0 || // shell prompt
            in.empty() // empty string
        )
    {
        // do nothing
    }
    else     // presumably a response to something else
    {
        post_event(EvAck(in));
        
        static const std::string connect = "CONNECT";
        if(in.compare(0, connect.size(), connect) == 0)
            post_event(EvConnect());
    }
}

void goby::acomms::benthos_fsm::ReceiveData::in_state_react( const EvRxSerial& e)
{
    std::string in = e.line;
    boost::trim(in);
    std::cout << "ReceiveData: " << in << std::endl;
    if(in == "<EOP>")
        post_event(EvReceiveComplete());
}



void goby::acomms::benthos_fsm::Command::in_state_react( const EvTxSerial& )
{
    double now = goby_time<double>();

    static const std::string atd = "ATD";
    
    if(!at_out_.empty())
    {
        double timeout = COMMAND_TIMEOUT_SECONDS;

        if((at_out_.front().first.last_send_time_ + timeout) < now)
        {
            std::string at_command;
            if(at_out_.front().second == "+++")
            {
                // Benthos implementation of Hayes requires 1 second pause *before* +++
                sleep(1);
                // insert \r after +++ so that we accidentally send +++ during command mode, this is recognized as an (invalid) command
                at_command = at_out_.front().second + "\r";
            }
            else
            {
                at_command = at_out_.front().second + "\r";
            }
            
            if(++at_out_.front().first.tries_ > RETRIES_BEFORE_RESET)
            {
                glog.is(DEBUG1) && glog << group("benthosatm900") << warn << "No valid response after " << RETRIES_BEFORE_RESET << " tries. Resetting state machine" << std::endl;
                post_event(EvReset());
            }
            else
            {
                context<BenthosATM900FSM>().serial_tx_buffer().push_back(at_command);
                at_out_.front().first.last_send_time_ = now;
            }
        }
        
    }
}



void goby::acomms::benthos_fsm::TransmitData::in_state_react( const EvTxSerial& )
{
    boost::circular_buffer<protobuf::ModemTransmission>& data_out = context<BenthosATM900FSM>().data_out();
    if(!data_out.empty())
    {
        for(int i = 0, n = data_out.front().frame_size(); i < n; ++i)
        {
            if(data_out.front().frame(i).empty())
                break;
            
            // frame message
            std::string rudics_packet;
            serialize_rudics_packet(data_out.front().frame(i), &rudics_packet, "\r");

            context<BenthosATM900FSM>().serial_tx_buffer().push_back(rudics_packet);
        }
        data_out.pop_front();
    }
}

void goby::acomms::benthos_fsm::Command::in_state_react( const EvAck & e)
{
    bool valid = false;
    if(!at_out().empty())
    {
        const std::string& last_cmd = at_out().front().second;
        if(last_cmd.size() > 0 && (e.response_ == "OK"))
        {
            if(last_cmd == "ATH")
                post_event(EvNoCarrier());
        }

        static const std::string connect = "CONNECT";    
        static const std::string user = "user";


        if(e.response_ == "OK")
        {
            valid = true;
        }
        else if(e.response_.compare(0, connect.size(), connect) == 0 && last_cmd.substr(0, 3) != "+++")
        {
            valid = true;
        }
        else if(e.response_ == "Command '+++' not found" && last_cmd.substr(0, 3) == "+++") // response to '+++' during command mode, so this is OK
        {
            valid = true;
        }
        else if(e.response_.compare(0, user.size(), user) == 0 && last_cmd.substr(0, 3) == "+++")
        {
            // no response in command mode other than giving us a new user:N> prompt
            valid = true;
        }
        else if(last_cmd.size() > 0) // deal with varied CLAM responses
        {
            std::string::size_type eq_pos = last_cmd.find('=');
            static const std::string cfg_store = "cfg store";
            
            if(last_cmd[0] == '@' && eq_pos != std::string::npos && boost::iequals(last_cmd.substr(1, eq_pos-1), e.response_.substr(0, eq_pos-1)))
            {
                // configuration sentences
                // e.g. last_cmd: "@P1EchoChar=Dis"
                //            in: "P1EchoChar      | Dis"
                valid = true;
            }
            else if(e.response_ == "Configuration stored." && boost::iequals(last_cmd.substr(0, cfg_store.size()), cfg_store))
            {
                valid = true;
            }
            
        }
        
        if(valid)
        {
            glog.is(DEBUG2) && glog << "Popping: " << at_out().front().second << std::endl;
            at_out().pop_front();
            if(at_out().empty())
                post_event(EvAtEmpty());
        }
    }

    if(!valid)
    {
        glog.is(DEBUG1) && glog << group("benthosatm900") << "Ignoring: '" << e.response_ << "'" << std::endl;
    }
}


// void goby::acomms::benthos_fsm::OnCall::in_state_react(const EvRxOnCallSerial& e)
// {
//     std::string in = e.line;

//     // check that it's not "NO CARRIER"
//     static const std::string no_carrier = "NO CARRIER";
//     if(in.find(no_carrier) != std::string::npos)
//     {
//         post_event(EvNoCarrier());
//     }
//     else if(boost::trim_copy(in) == "goby")
//     {
//         glog.is(DEBUG1) && glog << group("benthosatm900") << "Detected start of Goby RUDICS call" << std::endl;        
//     }
//     else if(boost::trim_copy(in) == "bye")
//     {
//         glog.is(DEBUG1) && glog << group("benthosatm900") << "Detected remote completion of Goby RUDICS call" << std::endl;
//         set_bye_received(true);
//     }
//     else
//     {
//         std::string bytes;
//         try
//         {
//             parse_rudics_packet(&bytes, in);
            
//             protobuf::ModemTransmission msg;
//             msg.ParseFromString(bytes);
//             context< BenthosATM900FSM >().received().push_back(msg);
//             set_last_rx_time(goby_time<double>());
//         }
//         catch(RudicsPacketException& e)
//         {
//             glog.is(DEBUG1) && glog <<  warn  << group("benthosatm900") << "Could not decode packet: " << e.what() << std::endl;
//         }
//     }
// }


// void goby::acomms::benthos_fsm::OnCall::in_state_react( const EvTxOnCallSerial& )
// {
//     boost::circular_buffer<protobuf::ModemTransmission>& data_out = context<BenthosATM900FSM>().data_out();
//     if(!data_out.empty())
//     {
//         // serialize the (protobuf) message
//         std::string bytes;
//         data_out.front().SerializeToString(&bytes);

//         // frame message
//         std::string rudics_packet;
//         serialize_rudics_packet(bytes, &rudics_packet);

//         context<BenthosATM900FSM>().serial_tx_buffer().push_back(rudics_packet);
//         data_out.pop_front();

//         set_last_tx_time(goby_time<double>());
//     }
// }

