
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
