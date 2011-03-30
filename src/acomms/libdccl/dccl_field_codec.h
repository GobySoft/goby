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
#include <boost/signals.hpp>
#include <google/protobuf/message.h>
#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/descriptor.h>

#include "dccl_constants.h"
#include "dccl_exception.h"
#include "goby/util/string.h"

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

            // max_size in bits
            unsigned max_size(const google::protobuf::Message* msg,
                          const google::protobuf::FieldDescriptor* field);

            // min_size in bits
            unsigned min_size(const google::protobuf::Message* msg,
                              const google::protobuf::FieldDescriptor* field);

            
            // write information about this field
            void info(const google::protobuf::Message* msg,
                      const google::protobuf::FieldDescriptor* field,
                      std::ostream* os);
            
            void validate(const google::protobuf::Message* msg,
                          const google::protobuf::FieldDescriptor* field);
            
            
            static void set_in_header(bool in_header) { in_header_ = in_header;}
            static bool in_header() { return in_header_; }            

          protected:
            static const google::protobuf::Message* root_message()
            { return MessageHandler::msg_const_.at(0); }

            static google::protobuf::Message* mutable_root_message()
            { return MessageHandler::msg_.at(0); }
            
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
            {
                return !MessageHandler::msg_const_.empty() ? MessageHandler::msg_const_.back() : 0;
            }
            
            static google::protobuf::Message* mutable_this_message()
            {
                return !MessageHandler::msg_.empty() ? MessageHandler::msg_.back() : 0;
            }
            
            const google::protobuf::FieldDescriptor* field()
            { return field_; }
            
            template<typename Extension>
                typename Extension::TypeTraits::ConstType get(const Extension& e)
            {
                return field()->options().GetExtension(e);
            }

            
            template<typename Extension>
                bool has(const Extension& e)
            {
                return field()->options().HasExtension(e);
            }
            
            template<typename Extension>
                void require(const Extension& e, const std::string& name)
            {
                if(!has(e))
                    throw(DCCLException("Field " + field()->name() + " missing option extension called `" + name + "`."));
            }            
            
            void require(bool b, const std::string& description)
            {
                if(!b)
                    throw(DCCLException("Field " + field()->name() + " failed validation: " + description));
            }
            
                
            virtual void _validate()
            { }

            virtual std::string _info();
            
            
            // field == 0 for root message!
            virtual Bitset _encode(const boost::any& field_value) = 0;
            
            // field == 0 for root message!
            virtual boost::any _decode(Bitset* bits) = 0;
                

            // max_size in bits
            // field == 0 for root message!
            virtual unsigned _max_size() = 0;
            virtual unsigned _min_size()
            {
                if(!_variable_size())
                    return _max_size();
                else
                    throw(DCCLException("Field " + field()->name() + " is variable sized but did not overload _min_size"));
            }
            
            
            virtual goby::acomms::Bitset
                _encode_repeated(const std::vector<boost::any>& field_values);

            virtual std::vector<boost::any>
                _decode_repeated(Bitset* repeated_bits);
            
            virtual unsigned _max_size_repeated();
            virtual unsigned _min_size_repeated();

            
            virtual bool _variable_size() { return false; }

            

            static boost::signal<void (unsigned size)> get_more_bits;
            void _get_bits(Bitset* these_bits, Bitset* bits, unsigned size);
            
          private:
            void _prepend_bits(const Bitset& new_bits, Bitset* bits);

            //RAII handler for the current Message recursion stack
            class MessageHandler
            {
              public:
                MessageHandler(const google::protobuf::FieldDescriptor* field);
                
                ~MessageHandler()
                {
                    for(int i = 0; i < messages_pushed_; ++i)
                        __pop();
                }
                bool first() 
                {
                    return msg_const_.empty();
                }

                int count() 
                {
                    return msg_const_.size();
                }

                void push(const google::protobuf::Message* msg);

                void push(google::protobuf::Message* msg)
                {
                    msg_.push_back(msg);                
                    const google::protobuf::Message* const_msg = msg;
                    push(const_msg);
                }

                
                friend class DCCLFieldCodec;
              private:

                void __pop();
                
                
                static std::vector<const google::protobuf::Message*> msg_const_;
                static std::vector<google::protobuf::Message*> msg_;
                int messages_pushed_;
            };
            
          private:
            static bool in_header_;
            const google::protobuf::FieldDescriptor* field_;
        };
        
    }
}


#endif
