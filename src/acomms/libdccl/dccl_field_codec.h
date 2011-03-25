// copyright 2009-2011 t. schneider tes@mit.edu
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

#ifndef DCCLFIELDCODEC20110322H
#define DCCLFIELDCODEC20110322H

#include <map>
#include <string>

#include <boost/any.hpp>
#include <boost/dynamic_bitset.hpp>
#include <google/protobuf/message.h>
#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/descriptor.h>

#include "dccl_constants.h"

namespace goby
{
    namespace acomms
    {
        class DCCLFieldCodec
        {
          public:
          DCCLFieldCodec() { }
            
            // non-repeated
            void encode(Bitset* bits,
                        const boost::any& field_value,
                        const google::protobuf::FieldDescriptor* field);
            
            void decode(Bitset* bits,
                        boost::any* field_value,
                        const google::protobuf::FieldDescriptor* field);

            // repeated
            void encode(Bitset* bits,
                        const std::vector<boost::any>& field_values,
                        const google::protobuf::FieldDescriptor* field);
            
            void decode(Bitset* bits,
                        std::vector<boost::any>* field_values,
                        const google::protobuf::FieldDescriptor* field);

            // size in bits
            unsigned size(const google::protobuf::FieldDescriptor* field);

            void validate(const google::protobuf::FieldDescriptor* field)
            { _validate(field); }
            

            static void set_message(const google::protobuf::Message* msg) { msg_ = msg; }
            static const google::protobuf::Message* message() { return msg_; }            

            static void set_in_header(bool in_header) { in_header_ = in_header; }
            static bool in_header() { return in_header_; } 

            
          protected:
            virtual void _validate(const google::protobuf::FieldDescriptor* field)
            { }

            virtual Bitset _encode(const boost::any& field_value,
                                   const google::protobuf::FieldDescriptor* field) = 0;
            
            virtual boost::any _decode(const Bitset& bits,
                                       const google::protobuf::FieldDescriptor* field) = 0;
                

            // size in bits
            virtual unsigned _size(const google::protobuf::FieldDescriptor* field) = 0;
            
            
            virtual goby::acomms::Bitset
                repeated_encode(const std::vector<boost::any>& field_values,
                                const google::protobuf::FieldDescriptor* field);

            virtual std::vector<boost::any>
                repeated_decode(Bitset* repeated_bits,
                                const google::protobuf::FieldDescriptor* field);
            
            virtual unsigned repeated_size(const google::protobuf::FieldDescriptor* field);
            
            
          private:
            void encode_common(Bitset* new_bits, Bitset* bits, unsigned size);
            void decode_common(Bitset* these_bits, Bitset* bits, unsigned size);


          private:
            static const google::protobuf::Message* msg_;
            static bool in_header_;
            
        };
        
    }
}


#endif
