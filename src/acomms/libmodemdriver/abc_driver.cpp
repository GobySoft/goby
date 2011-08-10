// copyright 2011 t. schneider tes@mit.edu
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

#include "abc_driver.h"
#include "driver_exception.h"

#include "goby/util/logger.h"
#include "goby/util/binary.h"

using goby::util::hex_encode;
using goby::util::hex_decode;


goby::acomms::ABCDriver::ABCDriver(std::ostream* log /*= 0*/)
    : ModemDriverBase(log),
      log_(log)
{
    // other initialization you can do before you have your goby::acomms::DriverConfig configuration object
}

void goby::acomms::ABCDriver::startup(const protobuf::DriverConfig& cfg)
{
    driver_cfg_ = cfg;
    // check `driver_cfg_` to your satisfaction and then start the modem physical interface
    if(!driver_cfg_.has_serial_baud())
        driver_cfg_.set_serial_baud(DEFAULT_BAUD);

    // log_ is allowed to be 0 (NULL), so always check it first
    if(log_) *log_ << group("modem_out") << "ABCDriver configuration good. Starting modem..." << std::endl;
    ModemDriverBase::modem_start(driver_cfg_);

    // set your local modem id (MAC address) 
    driver_cfg_.modem_id();
    

    {
        
        std::stringstream raw;
        raw << "CONF,MAC:" << driver_cfg_.modem_id() << "\r\n";
        signal_and_write(raw.str());
    }

    
    // now set our special configuration values
    {
        std::stringstream raw;
        raw << "CONF,FOO:" << driver_cfg_.GetExtension(ABCDriverConfig::enable_foo) << "\r\n";
        signal_and_write(raw.str());
    }
    {
        std::stringstream raw;
        raw << "CONF,BAR:" << driver_cfg_.GetExtension(ABCDriverConfig::enable_bar) << "\r\n";
        signal_and_write(raw.str());
    }
} // startup

void goby::acomms::ABCDriver::shutdown()
{
    // put the modem in a low power state?
    // ...
    ModemDriverBase::modem_close();
} // shutdown

void goby::acomms::ABCDriver::handle_initiate_transmission(protobuf::ModemDataInit* init_msg)
{
    protobuf::ModemMsgBase* base_msg = init_msg->mutable_base();
    
    if(log_)
    {
        // base_msg->rate() can be 0 (lowest), 1, 2, 3, 4, or 5 (lowest). Map these integers onto real bit-rates
        // in a meaningful way (on the WHOI Micro-Modem 0 ~= 80 bps, 5 ~= 5000 bps).
        *log_ <<  group("modem_out") << "We were asked to transmit from " << base_msg->src()
              << " to " << base_msg->dest()
              << " at bitrate code " << base_msg->rate() << std::endl;
    } 

    protobuf::ModemDataRequest request_msg; // used to request data from libqueue
    protobuf::ModemDataTransmission data_msg; // used to store the requested data

    // set up request_msg
    request_msg.mutable_base()->set_src(base_msg->src());
    request_msg.mutable_base()->set_dest(base_msg->dest());
    // let's say ABC modem uses 500 byte packet
    request_msg.set_max_bytes(500);

    ModemDriverBase::signal_data_request(request_msg, &data_msg);
    
    // do nothing with an empty message
    if(data_msg.data().empty()) return;
    
    if(log_)
    {
        *log_ <<  group("modem_out") << "Sending these data now: " << data_msg << std::endl;
    }
    
    // let's say we can send at three bitrates with ABC modem: map these onto 0-5
    const unsigned BITRATE [] = { 100, 1000, 10000, 10000, 10000, 10000};

    // I'm making up a syntax for the wire protocol...
    std::stringstream raw;
    raw << "SEND,TO:" << data_msg.base().dest()
        << ",FROM:" << data_msg.base().src()
        << ",HEX:" << hex_encode(data_msg.data())
        << ",BITRATE:" << BITRATE[base_msg->rate()]
        << ",ACK:TRUE"
        << "\r\n";

    // let anyone who is interested know
    signal_and_write(raw.str(), base_msg);    
} // handle_initiate_transmission

void goby::acomms::ABCDriver::do_work()
{
    std::string in;
    while(modem_read(&in))
    {
        std::map<std::string, std::string> parsed;

        // breaks `in`: "RECV,TO:3,FROM:6,HEX:ABCD015910"
        //   into `parsed`: "KEY"=>"RECV", "TO"=>"3", "FROM"=>"6", "HEX"=>"ABCD015910"
        try
        {
            boost::trim(in); // get whitespace off from either end
            parse_in(in, &parsed);
            
            protobuf::ModemMsgBase base_msg;
            base_msg.set_raw(in);
            
            using google::protobuf::int32;
            base_msg.set_src(goby::util::as<int32>(parsed["FROM"]));
            base_msg.set_dest(goby::util::as<int32>(parsed["TO"]));
            base_msg.set_time(goby::util::as<std::string>(
                                  goby::util::goby_time()));

            if(log_) *log_ << group("modem_in") << in << std::endl;
            ModemDriverBase::signal_all_incoming(base_msg);
            
            if(parsed["KEY"] == "RECV")
            {
                protobuf::ModemDataTransmission data_msg;
                data_msg.mutable_base()->CopyFrom(base_msg);
                data_msg.set_data(hex_decode(parsed["HEX"]));
                if(log_) *log_ << group("modem_in") << "received: " << data_msg << std::endl;
                ModemDriverBase::signal_receive(data_msg);
            }
            else if(parsed["KEY"] == "ACKN")
            {
                protobuf::ModemDataAck ack_msg;
                ack_msg.mutable_base()->CopyFrom(base_msg);
                ModemDriverBase::signal_ack(ack_msg);
            }
        }
        catch(std::exception& e)
        {
            if(log_) *log_ << warn << "Bad line: " << in << std::endl;
            if(log_) *log_ << warn << "Exception: " << e.what() << std::endl;
        }
    }
} // do_work

void goby::acomms::ABCDriver::signal_and_write(const std::string& raw, protobuf::ModemMsgBase* base_msg /* = 0 */)
{
    static protobuf::ModemMsgBase local_base_msg;
    if(!base_msg)
        base_msg = &local_base_msg;
    
    base_msg->set_raw(raw);
    ModemDriverBase::signal_all_outgoing(*base_msg);    
    if(log_) *log_ << group("modem_out") << boost::trim_copy(raw) << std::endl;
    ModemDriverBase::modem_write(raw); 
}

void goby::acomms::ABCDriver::parse_in(const std::string& in, std::map<std::string, std::string>* out)
{
    std::vector<std::string> comma_split;
    boost::split(comma_split, in, boost::is_any_of(","));
    out->insert(std::make_pair("KEY", comma_split.at(0)));
    for(int i = 1, n = comma_split.size(); i < n; ++i)
    {
        std::vector<std::string> colon_split;
        boost::split(colon_split, comma_split[i],
                     boost::is_any_of(":"));
        out->insert(std::make_pair(colon_split.at(0),
                                   colon_split.at(1)));
    }
}

