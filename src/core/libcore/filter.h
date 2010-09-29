// copyright 2010 t. schneider tes@mit.edu
// 
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

#ifndef FILTER20100922H
#define FILTER20100922H

#include <google/protobuf/message.h>
#include <google/protobuf/descriptor.h>

#include "goby/core/proto/interprocess_notification.pb.h"


namespace goby
{
    namespace core
    {
        // true if `msg` passes through `filter`
        bool clears_filter(const google::protobuf::Message& msg, const goby::core::proto::Filter& filter);

        template<typename T1, typename T2>
            bool filter_comp(const T1& t1,
                             const T2& t2,
                             goby::core::proto::Filter::Operation op)
        {
            switch(op)
            {
                case goby::core::proto::Filter::EQUAL: return t1 == t2;
                case goby::core::proto::Filter::NOT_EQUAL: return t1 != t2;    
                default: return false;
            }
        }
        
        inline bool operator==(const goby::core::proto::Filter& f1,
                               const goby::core::proto::Filter& f2)
        {
            return f1.key() == f2.key() &&
            f1.operation() == f2.operation() &&
            f1.value() == f2.value();
        }        

        inline bool operator!=(const goby::core::proto::Filter& f1,
                               const goby::core::proto::Filter& f2)
        { return !(f1==f2); }
        
    }
}


#endif
