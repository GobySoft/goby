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

#include "moos_ufield_sim_driver.h"
#include "goby/acomms/modemdriver/driver_exception.h"
#include "goby/common/logger.h"
#include "goby/util/binary.h"
#include "goby/moos/protobuf/ufield_sim_driver.pb.h"
#include "goby/moos/moos_string.h"

using goby::util::hex_encode;
using goby::util::hex_decode;
using goby::glog;
using namespace goby::common::logger;
using goby::acomms::operator<<;


goby::moos::UFldDriver::UFldDriver()
{
}

void goby::moos::UFldDriver::startup(const goby::acomms::protobuf::DriverConfig& cfg)
{
    glog.is(DEBUG1) && glog << group(glog_out_group()) << "Goby MOOS uField Toolbox driver starting up." << std::endl;

    driver_cfg_ = cfg;


    for(int i = 0, n = driver_cfg_.ExtensionSize(protobuf::Config::id_entry);
        i < n; ++ i)
    {
        const protobuf::ModemIdEntry& entry =
            driver_cfg_.GetExtension(protobuf::Config::id_entry, i);
        
        modem_id2name_.left.insert(std::make_pair(entry.modem_id(), entry.name()));
    }

    const std::string& moos_server = driver_cfg_.GetExtension(protobuf::Config::moos_server);
    int moos_port = driver_cfg_.GetExtension(protobuf::Config::moos_port);
    moos_client_.Run(moos_server.c_str(), moos_port, "goby.moos.UFldDriver");

    int i = 0;
    while(!moos_client_.IsConnected())
    {
        glog.is(DEBUG1) &&
            glog << group(glog_out_group())
                 << "Trying to connect to MOOSDB at "<< moos_server << ":" << moos_port << ", try " << i++ << std::endl;
        sleep(1);
    }
    glog.is(DEBUG1) &&
        glog << group(glog_out_group())
             << "Connected to MOOSDB." << std::endl;
    
    moos_client_.Register(driver_cfg_.GetExtension(protobuf::Config::ufield_incoming_moos_var), 0);
} 

void goby::moos::UFldDriver::shutdown()
{
    moos_client_.Close();
} 

void goby::moos::UFldDriver::handle_initiate_transmission(
    const goby::acomms::protobuf::ModemTransmission& orig_msg)
{
    // copy so we can modify
    goby::acomms::protobuf::ModemTransmission msg = orig_msg;    
    
    if(msg.rate() < driver_cfg_.ExtensionSize(protobuf::Config::rate_to_bytes))
        msg.set_max_frame_bytes(driver_cfg_.GetExtension(protobuf::Config::rate_to_bytes, msg.rate()));
    else
        msg.set_max_frame_bytes(DEFAULT_PACKET_SIZE);

    // no data given to us, let's ask for some
    if(msg.frame_size() == 0)
        ModemDriverBase::signal_data_request(&msg);

    if(msg.frame_size() && msg.frame(0).size())
    {
        std::string dest = (modem_id2name_.left.count(msg.dest()) ?
                            modem_id2name_.left.find(msg.dest())->second :
                            "unknown");

        std::string src = (modem_id2name_.left.count(msg.src()) ?
                           modem_id2name_.left.find(msg.src())->second :
                           "unknown");
        
        std::stringstream out_ss;
        out_ss << "src_node=" << src
               << ",dest_node=" << dest
               << ",var_name=GOBY_UFIELD_INCOMING"
               << ",string_val="
               << "type=DATA" 
               << ",src=" << msg.src()
               << ",dest=" << msg.dest()
               << ",ack=" << goby::util::as<std::string>(msg.ack_requested())
               << ",data=" << hex_encode(msg.frame(0));

        goby::acomms::protobuf::ModemRaw out_raw;
        out_raw.set_raw(out_ss.str());
        ModemDriverBase::signal_raw_outgoing(out_raw);

        const std::string& out_moos_var =
            driver_cfg_.GetExtension(protobuf::Config::ufield_outgoing_moos_var);
        
        glog.is(DEBUG1) &&
            glog << group(glog_out_group())  << out_moos_var << ": " <<  out_ss.str() << std::endl;

        
        moos_client_.Notify(out_moos_var, out_ss.str());
    }
} 

void goby::moos::UFldDriver::do_work()
{
    MOOSMSG_LIST msgs;
    if(moos_client_.Fetch(msgs))
    {
        for(MOOSMSG_LIST::iterator it = msgs.begin(),
                end = msgs.end(); it != end; ++it)
        {
            const std::string& in_moos_var = driver_cfg_.GetExtension(protobuf::Config::ufield_incoming_moos_var);
            if(it->GetKey() == in_moos_var)
            {
                const std::string& value = it->GetString();
                
                glog.is(DEBUG1) &&
                    glog << group(glog_in_group())  << in_moos_var << ": " << value  << std::endl;
                
                int src, dest;
                goby::moos::val_from_string(src, value, "src_node");
                goby::moos::val_from_string(dest, value, "dest_node");

                std::string hex;
                goby::moos::val_from_string(hex, value, "string_val");
                
                goby::acomms::protobuf::ModemTransmission msg;
                msg.set_src(src);
                msg.set_dest(dest);
                msg.set_time(goby::common::goby_time<uint64>());

                msg.set_type(goby::acomms::protobuf::ModemTransmission::DATA);
                msg.add_frame(hex_decode(hex));
                ModemDriverBase::signal_receive(msg);
            }
        }
    }
} 

