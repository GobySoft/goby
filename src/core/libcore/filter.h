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
        /// \name Filter Helpers
        //@{
        /// \brief Helper function for determining if a google::protobuf::Message pass through
        /// the given Filter object 
        ///
        /// \param msg Google Protocol Buffers message to check
        /// \param filter Filter to check message against
        /// \return true if `msg` passes through `filter`
        bool clears_filter(const google::protobuf::Message& msg, const goby::core::proto::Filter& filter);

        /// \brief gives the boolean value for t1 and t2 for the given comparison operation
        ///
        /// \param t1 first value to compare
        /// \param t2 second value to compare
        /// \return boolean value depending on the Filter::Operation. For Filter::EQUAL, returns true if t1==t2 or false if t1 != t2.
        template<typename T>
            bool filter_comp(const T& t1,
                             const T& t2,
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

        //@}
    }
}


#endif
