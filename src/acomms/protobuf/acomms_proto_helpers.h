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

#include "goby/acomms/protobuf/modem_message.pb.h"
#include "goby/util/time.h"

namespace goby
{
    namespace acomms
    {
        inline void set_time(protobuf::ModemMsgBase* msg_base, const boost::posix_time::ptime& t = goby::util::goby_time())
        {
            msg_base->set_iso_time(boost::posix_time::to_iso_string(t));
            msg_base->set_unix_time(goby::util::ptime2unix_double(t));
            msg_base->set_time_source(protobuf::ModemMsgBase::GOBY_TIME);
        }
        inline void get_time(const protobuf::ModemMsgBase& msg_base, boost::posix_time::ptime* t)
        {
            *t = boost::posix_time::from_iso_string(msg_base.iso_time());
        }
    }
}


#endif
