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

#include "goby/protobuf/modem_message.pb.h"
#include "goby/protobuf/queue.pb.h"
#include "goby/util/time.h"

namespace goby
{
    namespace acomms
    {        
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
