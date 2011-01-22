// copyright 2011 t. schneider tes@mit.edu
// 
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
#ifndef AcommsProtoHelpers20110117H
#define AcommsProtoHelpers20110117H

#include <sstream>

#include "goby/acomms/protobuf/modem_message.pb.h"
#include "goby/acomms/protobuf/queue.pb.h"
#include "goby/util/time.h"

namespace goby
{
    namespace acomms
    {
        inline void set_time(protobuf::ModemMsgBase* base_msg, const boost::posix_time::ptime& t = goby::util::goby_time())
        {
            base_msg->set_iso_time(boost::posix_time::to_iso_string(t));
            base_msg->set_unix_time(goby::util::ptime2unix_double(t));
            base_msg->set_time_source(protobuf::ModemMsgBase::GOBY_TIME);
        }

        inline void get_time(const protobuf::ModemMsgBase& base_msg, boost::posix_time::ptime* t)
        {
            *t = boost::posix_time::from_iso_string(base_msg.iso_time());
        }
        
        /* inline std::string snippet(const ModemMsgBase& base_msg) */
        /* { */
        /* } */
        
        
        namespace protobuf
        {
            
            inline bool operator<(const goby::acomms::protobuf::QueueKey& a,
                                  const goby::acomms::protobuf::QueueKey& b)
            { return (a.id() == b.id()) ? a.type() < b.type() : a.id() < b.id(); }

            inline bool operator>(const goby::acomms::protobuf::QueueKey& a,
                                  const goby::acomms::protobuf::QueueKey& b)
            { return (a.id() == b.id()) ? a.type() > b.type() : a.id() > b.id(); }
            inline bool operator>=(const goby::acomms::protobuf::QueueKey& a,
                                   const goby::acomms::protobuf::QueueKey& b)
            { return !(a<b); }
            inline bool operator<=(const goby::acomms::protobuf::QueueKey& a,
                                   const goby::acomms::protobuf::QueueKey& b)
            { return !(a>b); }
            inline bool operator==(const goby::acomms::protobuf::QueueKey& a,
                                   const goby::acomms::protobuf::QueueKey& b)
            { return (a.id() == b.id()) && (a.type() == b.type()); }
        }
    }
}

        




#endif
