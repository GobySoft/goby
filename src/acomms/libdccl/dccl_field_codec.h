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
#include <boost/bind.hpp>
#include <boost/dynamic_bitset.hpp>
#include <boost/signals.hpp>
#include <boost/ptr_container/ptr_map.hpp>
#include <google/protobuf/message.h>
#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/descriptor.h>

#include "dccl_common.h"
#include "dccl_exception.h"
#include "goby/util/string.h"
#include "dccl_type_helper.h"

namespace goby
{
    namespace acomms
    {        
        class DCCLFieldCodec
        {
          public:
            typedef goby::acomms::Bitset Bitset;
            
            DCCLFieldCodec() { }
            virtual ~DCCLFieldCodec() { }
            
            enum MessagePart
            {
                HEAD,
                BODY
            };

            virtual boost::any pre_encode(const boost::any& field_value)
            { return field_value; }
            virtual std::vector<boost::any> pre_encode(const std::vector<boost::any>& field_values)
            {
                std::vector<boost::any> return_values;
                BOOST_FOREACH(const boost::any& field_value, field_values)
                    return_values.push_back(pre_encode(field_value));
                return return_values;
            }
            
            // traverse const 
            void encode(Bitset* bits,
                        const boost::any& field_value,
                        MessagePart part);
            void encode(Bitset* bits,
                         const boost::any& field_value,
                         const google::protobuf::FieldDescriptor* field);
            void encode_repeated(Bitset* bits,
                                 const std::vector<boost::any>& field_values,
                                 const google::protobuf::FieldDescriptor* field);

            void size(unsigned* bit_size, const google::protobuf::Message& msg, MessagePart part);
            void size(unsigned* bit_size, const boost::any& field_value,
                      const google::protobuf::FieldDescriptor* field);
            void size_repeated(unsigned* bit_size, const std::vector<boost::any>& field_values,
                               const google::protobuf::FieldDescriptor* field);

            // traverse mutable
            void decode(Bitset* bits,
                        boost::any* field_value,
                        MessagePart part);
            void decode(Bitset* bits,
                         boost::any* field_value,
                         const google::protobuf::FieldDescriptor* field);            
            void decode_repeated(Bitset* bits,
                                 std::vector<boost::any>* field_values,
                                 const google::protobuf::FieldDescriptor* field);

            virtual boost::any post_decode(const boost::any& wire_value) { return wire_value; }
            virtual std::vector<boost::any> post_decode(const std::vector<boost::any>& wire_values)
            {
                std::vector<boost::any> return_values;
                BOOST_FOREACH(const boost::any& wire_value, wire_values)
                    return_values.push_back(post_decode(wire_value));
                return return_values;
            }
            
            
            // traverse schema (Descriptor)
            unsigned max_size(const google::protobuf::Descriptor* desc, MessagePart part);
            unsigned max_size(const google::protobuf::FieldDescriptor* field);
            
            unsigned min_size(const google::protobuf::Descriptor* desc, MessagePart part);
            unsigned min_size(const google::protobuf::FieldDescriptor* field);
            
            void validate(const google::protobuf::Descriptor* desc, MessagePart part);
            void validate(const google::protobuf::FieldDescriptor* field);
            
            void info(const google::protobuf::Descriptor* desc, std::ostream* os, MessagePart part);  
            void info(const google::protobuf::FieldDescriptor* field, std::ostream* os);


            // codec information
            void set_name(const std::string& name)
            { name_ = name; }
            void set_field_type(google::protobuf::FieldDescriptor::Type type)
            { field_type_ = type; }
            void set_wire_type(google::protobuf::FieldDescriptor::CppType type)
            { wire_type_ = type; }

            std::string name() const { return name_; }
            google::protobuf::FieldDescriptor::Type field_type() const  { return field_type_; }
            google::protobuf::FieldDescriptor::CppType wire_type() const  { return wire_type_; }


            static void register_wire_value_hook(
                int field_option_extension_id,
                boost::function<void (const boost::any& wire_value, const boost::any& extension_value)>& callback)
            {
                wire_value_hooks_[field_option_extension_id].connect(callback);
            }

          protected:
            // for: 
            // message Foo
            // {
            //    int32 bar = 1;
            //    FooBar baz = 2;
            // }
            // returns Descriptor for Foo if field == 0
            // returns Descriptor for Foo if field == FieldDescriptor for bar
            // returns Descriptor for FooBar if field == FieldDescriptor for baz
            static const google::protobuf::Descriptor* this_descriptor()
            {
                return !MessageHandler::desc_.empty() ? MessageHandler::desc_.back() : 0;
            }

            const google::protobuf::FieldDescriptor* this_field()
            {
                return !MessageHandler::field_.empty() ? MessageHandler::field_.back() : 0;
            }

            static MessagePart part() { return part_; }
            
            template<typename Extension>
                typename Extension::TypeTraits::ConstType get(const Extension& e)
            {
                if(this_field())
                    return this_field()->options().GetExtension(e);
                else
                    return typename Extension::TypeTraits::ConstType();
            }

            
            template<typename Extension>
                bool has(const Extension& e)
            {
                if(this_field())
                    return this_field()->options().HasExtension(e);
                else
                    return false;
            }
            
            template<typename Extension>
                void require(const Extension& e, const std::string& name)
            {
                if(!has(e))
                {
                    if(this_field())
                        throw(DCCLException("Field " + this_field()->name() + " missing option extension called `" + name + "`."));
                }
                
            }            
            
            void require(bool b, const std::string& description)
            {
                if(!b)
                {
                    if(this_field())
                        throw(DCCLException("Field " + this_field()->name() + " failed validation: " + description));
                    else
                        throw(DCCLException("Message " + this_descriptor()->name() + " failed validation: " + description));
                }
                
            }
            
                
            virtual void _validate()
            { }

            virtual std::string _info();

            
            virtual Bitset _encode(const boost::any& field_value) = 0;
            
            virtual boost::any _decode(Bitset* bits) = 0;

            virtual goby::acomms::Bitset
                _encode_repeated(const std::vector<boost::any>& field_values);

            virtual std::vector<boost::any>
                _decode_repeated(Bitset* repeated_bits);            
            
            // max_size in bits
            // field == 0 for root message!
            virtual unsigned _size(const boost::any& field_value) = 0;
            virtual unsigned _size_repeated(const std::vector<boost::any>& field_values);
            
            virtual unsigned _max_size() = 0;
            virtual unsigned _min_size() = 0;            

            virtual unsigned _max_size_repeated();
            virtual unsigned _min_size_repeated();

            
            virtual bool _variable_size() { return true; }


            static boost::signal<void (unsigned size)> get_more_bits;

            class BitsHandler
            {
              public:
                BitsHandler(Bitset* out_pool, Bitset* in_pool)
                    : in_pool_(in_pool),
                    out_pool_(out_pool)
                    {
                        connection_ = get_more_bits.connect(
                            boost::bind(&BitsHandler::transfer_bits, this, _1),
                            boost::signals::at_back);
                    }
                
                ~BitsHandler()
                {
                    connection_.disconnect();
                }
                void transfer_bits(unsigned size);

              private:
                Bitset* in_pool_;
                Bitset* out_pool_;
                boost::signals::connection connection_;
            };
            


            
            
            friend class DCCLCodec;
          private:
            bool __check_field(const google::protobuf::FieldDescriptor* field);
            void __encode_prepend_bits(const Bitset& new_bits, Bitset* bits);
            

            //RAII handler for the current Message recursion stack
            class MessageHandler
            {
              public:
                MessageHandler(const google::protobuf::FieldDescriptor* field = 0);
                
                ~MessageHandler()
                {
                    for(int i = 0; i < descriptors_pushed_; ++i)
                        __pop_desc();
                    
                    for(int i = 0; i < fields_pushed_; ++i)
                        __pop_field();
                }
                bool first() 
                {
                    return desc_.empty();
                }

                int count() 
                {
                    return desc_.size();
                }

                void push(const google::protobuf::Descriptor* desc);
                void push(const google::protobuf::FieldDescriptor* field);

                friend class DCCLFieldCodec;
              private:
                void __pop_desc();
                void __pop_field();
                
                static std::vector<const google::protobuf::Descriptor*> desc_;
                static std::vector<const google::protobuf::FieldDescriptor*> field_;
                int descriptors_pushed_;
                int fields_pushed_;
            };
            
          private:
            static MessagePart part_;
            // maps protobuf extension number for FieldOption onto a hook (signal) to call
            // if such a FieldOption is set, during the call to "size()"
            static boost::ptr_map<int, boost::signal<void (const boost::any& wire_value, const boost::any& extension_value)> >  wire_value_hooks_;
            
            
            std::string name_;
            google::protobuf::FieldDescriptor::Type field_type_;
            google::protobuf::FieldDescriptor::CppType wire_type_;


            

        };

        inline std::ostream& operator<<(std::ostream& os, const DCCLFieldCodec& field_codec )
        {
            using google::protobuf::FieldDescriptor;
            return os << "[FieldCodec '" << field_codec.name() << "']: field type: "
                      << DCCLTypeHelper::find(field_codec.field_type())->as_str()
                      << " (" << DCCLTypeHelper::find(FieldDescriptor::TypeToCppType(field_codec.field_type()))->as_str()
                      << ") | wire type: " << DCCLTypeHelper::find(field_codec.wire_type())->as_str();
        }

        class DCCLFixedFieldCodec : public DCCLFieldCodec
        {
          protected:
            virtual unsigned _size() = 0;
            virtual unsigned _size_repeated();
            
          private:
            unsigned _size(const boost::any& field_value)
            { return _size(); }

            unsigned _max_size()
            { return _size(); }

            unsigned _min_size()
            { return _size(); }
            
            bool _variable_size() { return false; }

            unsigned _size_repeated(const std::vector<boost::any>& field_values)
            { return _size_repeated(); }

            unsigned _max_size_repeated()
            { return _size_repeated(); }

            unsigned _min_size_repeated()
            { return _size_repeated(); }

            
        };
        
    }
}


#endif
