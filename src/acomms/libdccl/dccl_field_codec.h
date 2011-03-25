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
                        const google::protobuf::FieldDescriptor* field = 0);
            
            void decode(Bitset* bits,
                        boost::any* field_value,
                        const google::protobuf::FieldDescriptor* field= 0);

            // repeated
            void encode(Bitset* bits,
                        const std::vector<boost::any>& field_values,
                        const google::protobuf::FieldDescriptor* field = 0);
            
            void decode(Bitset* bits,
                        std::vector<boost::any>* field_values,
                        const google::protobuf::FieldDescriptor* field = 0);

            // size in bits
            unsigned size(const google::protobuf::Message* msg = 0,
                          const google::protobuf::FieldDescriptor* field = 0);

            void validate(const google::protobuf::Message* msg = 0,
                          const google::protobuf::FieldDescriptor* field = 0);
            

            static void set_in_header(bool in_header) { in_header_ = in_header;}
            static bool in_header() { return in_header_; } 

            
          protected:
            static const google::protobuf::Message* root_message()
            { return root_msg_const_; }

            static google::protobuf::Message* mutable_root_message()
            { return root_msg_; }
            
            // for: 
            // message Foo
            // {
            //    int32 bar = 1;
            //    FooBar baz = 2;
            // }
            // returns Descriptor for Foo if field == 0
            // returns Descriptor for Foo if field == FieldDescriptor for bar
            // returns Descriptor for FooBar if field == FieldDescriptor for baz
            static const google::protobuf::Message* this_message()
            { return this_msg_const_ ? this_msg_const_ : this_msg_; }  
            static google::protobuf::Message* mutable_this_message()
            { return this_msg_; }
            
            const google::protobuf::FieldDescriptor* this_field()
            { return this_field_; }  
            
            virtual void _validate()
            { }

            // field == 0 for root message!
            virtual Bitset _encode(const boost::any& field_value) = 0;
            
            // field == 0 for root message!
            virtual boost::any _decode(Bitset* bits) = 0;
                

            // size in bits
            // field == 0 for root message!
            virtual unsigned _size() = 0;
            
            
            virtual goby::acomms::Bitset
                _encode_repeated(const std::vector<boost::any>& field_values);

            virtual std::vector<boost::any>
                _decode_repeated(Bitset* repeated_bits);
            
            virtual unsigned _size_repeated();
            
            
          private:
            
            class Counter
            {
              public:
                Counter()
                {
                    ++i_;
                }
                ~Counter()
                {
                    --i_;
                }
                bool first() 
                {
                    return (i_ == 1);
                }
                static int i_;
            };
            
            void __encode_common(Bitset* new_bits, Bitset* bits, unsigned size);
            void __decode_common(Bitset* these_bits, Bitset* bits, unsigned size);
            void __try_set_this(const google::protobuf::FieldDescriptor* field);

            void __set_root_message(const google::protobuf::Message* msg)
            {
                root_msg_const_ = msg;
                this_msg_const_ = msg;
                this_field_ = 0;
            }
            
            void __set_root_message(google::protobuf::Message* msg)
            {
                root_msg_ = msg;
                this_msg_ = msg;
                const google::protobuf::Message* const_msg = msg;
                __set_root_message(const_msg);
            }
            
          private:
            static const google::protobuf::Message* root_msg_const_;
            static const google::protobuf::Message* this_msg_const_;

            static google::protobuf::Message* root_msg_;
            static google::protobuf::Message* this_msg_;

            const google::protobuf::FieldDescriptor* this_field_;
            
            static bool in_header_;
            
        };
        
    }
}


#endif
