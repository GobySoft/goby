// Copyright 2009-2012 Toby Schneider (https://launchpad.net/~tes)
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


#include "goby/pb/dbo.h"

#include <iostream>
#include <cmath>
#include <boost/interprocess/ipc/message_queue.hpp>
#include <boost/foreach.hpp>

#include <boost/type_traits/is_convertible.hpp>
#include <boost/utility/enable_if.hpp>


#include <google/protobuf/message.h>
#include <google/protobuf/dynamic_message.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/descriptor.pb.h>

#include "goby/common/logger.h"

#include <ctime>
#include <map>

const unsigned MAX_BUFFER_SIZE = 1 << 20;

int main()
{
    char buffer [MAX_BUFFER_SIZE];
    
    goby::core::DBOManager* dbo_manager = goby::core::DBOManager::get_instance();
    dbo_manager->connect("test2.db");
    
    try
    {
        boost::interprocess::message_queue tq (boost::interprocess::open_only, "type_queue");
        boost::interprocess::message_queue mq (boost::interprocess::open_only, "message_queue");
        
        unsigned int priority;
        std::size_t recvd_size;
        
        while(tq.try_receive(&buffer, MAX_BUFFER_SIZE, recvd_size, priority))
        {            
            google::protobuf::FileDescriptorProto proto_in;
            proto_in.ParseFromArray(&buffer,recvd_size);
            dbo_manager->add_file(proto_in);
        }
        
        while (mq.try_receive(&buffer, MAX_BUFFER_SIZE, recvd_size, priority))
        {
            boost::shared_ptr<google::protobuf::Message> dbl_msg = dbo_manager->new_msg_from_name("GobyDouble");
            dbl_msg->ParseFromArray(&buffer,recvd_size);
            
            std::cout << "receiver: " << dbl_msg->ShortDebugString() << std::endl;
            dbo_manager->add_message(dbl_msg);
        }
    }
    catch(boost::interprocess::interprocess_exception &ex)
    {
        boost::interprocess::message_queue::remove("message_queue");
        std::cout << ex.what() << std::endl;
        return 1;
    }

    boost::interprocess::message_queue::remove("message_queue");
    return 0;
}

