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

#include "test.pb.h"
#include "goby/acomms/queue.h"
#include "goby/acomms/connect.h"
#include "goby/util/binary.h"
#include "goby/util/logger.h"
#include "goby/acomms/libdccl/dccl_field_codec_default.h"
// tests basic DCCL queuing with non-BROADCAST destination

using goby::acomms::operator<<;

int receive_count = 0;
GobyMessage msg_in1;

void handle_receive(const google::protobuf::Message &msg);

int main(int argc, char* argv[])
{    
    goby::glog.add_stream(goby::util::Logger::DEBUG3, &std::cerr);
    goby::glog.set_name(argv[0]);
    
    goby::acomms::QueueManager* q_manager = goby::acomms::QueueManager::get();
    goby::acomms::protobuf::QueueManagerConfig cfg;
    const int MY_MODEM_ID = 1;
    const int UNICORN_MODEM_ID = 3;
    cfg.set_modem_id(MY_MODEM_ID);
    q_manager->set_cfg(cfg);
    
    goby::acomms::DCCLModemIdConverterCodec::add("unicorn", UNICORN_MODEM_ID);
    goby::acomms::DCCLModemIdConverterCodec::add("topside", MY_MODEM_ID);
    
    goby::acomms::connect(&q_manager->signal_receive, &handle_receive);

    msg_in1.set_telegram("hello!");
    msg_in1.mutable_header()->set_time(
        goby::util::as<std::string>(boost::posix_time::second_clock::universal_time()));
    msg_in1.mutable_header()->set_source_platform("topside");
    msg_in1.mutable_header()->set_dest_platform("unicorn");
    msg_in1.mutable_header()->set_dest_type(Header::PUBLISH_OTHER);
    

    std::cout << "Pushed: " << msg_in1 << std::endl;
    q_manager->push_message(msg_in1);


    goby::acomms::protobuf::ModemDataRequest request_msg;
    request_msg.set_max_bytes(256);
    request_msg.mutable_base()->set_dest(UNICORN_MODEM_ID);
    
    goby::acomms::protobuf::ModemDataTransmission data_msg;
    q_manager->handle_modem_data_request(request_msg, &data_msg);
    

    std::cout << "requesting data, got: " << data_msg << std::endl;
    std::cout << "\tdata as hex: " << goby::util::hex_encode(data_msg.data()) << std::endl;

    assert(data_msg.data() == goby::acomms::DCCLCodec::get()->encode(msg_in1));
    assert(data_msg.base().src() == MY_MODEM_ID);
    assert(data_msg.base().dest() == UNICORN_MODEM_ID);
    assert(data_msg.ack_requested() == true);
    

    // feed back the modem layer - this will be rejected
    q_manager->handle_modem_receive(data_msg);
    assert(receive_count == 0);

    
    // pretend we're now Unicorn
    cfg.set_modem_id(UNICORN_MODEM_ID);
    q_manager->set_cfg(cfg);
    
    // feed back the modem layer
    q_manager->handle_modem_receive(data_msg);
    
    assert(receive_count == 1);
    
    std::cout << "all tests passed" << std::endl;    
}

void handle_receive(const google::protobuf::Message &msg)
{
    std::cout << "Received: " << msg << std::endl;
    
    assert(msg_in1.SerializeAsString() == msg.SerializeAsString());
    
    ++receive_count;
}
