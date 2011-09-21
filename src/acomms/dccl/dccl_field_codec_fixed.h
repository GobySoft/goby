// copyright 2011 t. schneider tes@mit.edu
// 
// this file is part of the Dynamic Compact Control Language (DCCL),
// the goby-acomms codec. goby-acomms is a collection of libraries 
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

#ifndef DCCLFIELDCODEC20110510H
#define DCCLFIELDCODEC20110510H

#include "dccl_field_codec_typed.h"

namespace goby
{
    namespace acomms
    {
        /// \brief Base class for static-typed field encoders/decoders that use a fixed number of bits on the wire regardless of the value of the field. Use DCCLTypedFieldCodec if your encoder is variable length. See DCCLTypedFieldCodec for an explanation of the template parameters (FieldType and WireType).
        ///
        /// \ingroup dccl_api
        /// Implements DCCLTypedFieldCodec::size(const FieldType& field_value), DCCLTypedFieldCodec::max_size and DCCLTypedFieldCodec::min_size, and provides a virtual zero-argument function for size()
        template<typename WireType, typename FieldType = WireType>
            class DCCLTypedFixedFieldCodec : public DCCLTypedFieldCodec<WireType, FieldType>
        {
          protected:
          /// \brief The size of the encoded message in bits. Use DCCLTypedFieldCodec if the size depends on the data.
          virtual unsigned size() = 0;
          
          private:
          unsigned size(const FieldType& field_value)
          { return size(); }
          
          unsigned max_size()
          { return size(); }
          
          unsigned min_size()
          { return size(); }          
        };
    }
}


#endif
