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
#include "goby/protobuf/dccl.pb.h"
#include "goby/protobuf/acomms_proto_helpers.h"
#include "goby/protobuf/dccl_option_extensions.pb.h"
#include "goby/util/string.h"
#include "dccl_type_helper.h"

namespace goby
{
    namespace acomms
    {        
        class DCCLFieldCodecBase
        {
          public:
            typedef goby::acomms::Bitset Bitset;
            
            DCCLFieldCodecBase() { }
            virtual ~DCCLFieldCodecBase() { }
            
            enum MessagePart
            {
                HEAD,
                BODY
            };

            boost::any base_pre_encode(const boost::any& field_value)
            { return any_pre_encode(field_value); }
            std::vector<boost::any> base_pre_encode_repeated(const std::vector<boost::any>& field_values)
            {
                return any_pre_encode_repeated(field_values);
            }
            
            // traverse const 
            void base_encode(Bitset* bits,
                             const boost::any& field_value,
                             MessagePart part);
            void base_encode(Bitset* bits,
                             const boost::any& field_value,
                             const google::protobuf::FieldDescriptor* field);
            void base_encode_repeated(Bitset* bits,
                                      const std::vector<boost::any>& field_values,
                                      const google::protobuf::FieldDescriptor* field);

            void base_run_hooks(const google::protobuf::Message& msg, MessagePart part);
            void base_run_hooks(bool* b,
                                const boost::any& field_value,
                                const google::protobuf::FieldDescriptor* field);

            
            void base_size(unsigned* bit_size, const google::protobuf::Message& msg, MessagePart part);
            void base_size(unsigned* bit_size, const boost::any& field_value,
                           const google::protobuf::FieldDescriptor* field);
            void base_size_repeated(unsigned* bit_size, const std::vector<boost::any>& field_values,
                                    const google::protobuf::FieldDescriptor* field);

            // traverse mutable
            void base_decode(Bitset* bits,
                             boost::any* field_value,
                             MessagePart part);
            void base_decode(Bitset* bits,
                             boost::any* field_value,
                             const google::protobuf::FieldDescriptor* field);            
            void base_decode_repeated(Bitset* bits,
                                      std::vector<boost::any>* field_values,
                                      const google::protobuf::FieldDescriptor* field);

            boost::any base_post_decode(const boost::any& wire_value)
            { return any_post_decode(wire_value); }
            std::vector<boost::any> base_post_decode_repeated(
                const std::vector<boost::any>& wire_values)
            {
                return any_post_decode_repeated(wire_values);
            }
            
            
            // traverse schema (Descriptor)
            void base_max_size(unsigned* bit_size, const google::protobuf::Descriptor* desc, MessagePart part);
            void base_max_size(unsigned* bit_size, const google::protobuf::FieldDescriptor* field);
            
            void base_min_size(unsigned* bit_size, const google::protobuf::Descriptor* desc, MessagePart part);
            void base_min_size(unsigned* bit_size, const google::protobuf::FieldDescriptor* field);
            
            void base_validate(const google::protobuf::Descriptor* desc, MessagePart part);
            void base_validate(bool* b, const google::protobuf::FieldDescriptor* field);
            
            void base_info(std::ostream* os, const google::protobuf::Descriptor* desc, MessagePart part);  
            void base_info(std::ostream* os, const google::protobuf::FieldDescriptor* field);


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
                int key, boost::function<void (const boost::any& field_value,
                                               const boost::any& wire_value,
                                               const boost::any& extension_value)> callback)
            {
                wire_value_hooks_[key].connect(callback);
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

            static MessagePart part() { return part_; }

            const google::protobuf::FieldDescriptor* this_field()
            {
                return !MessageHandler::field_.empty() ? MessageHandler::field_.back() : 0;
            }
            

            DCCLFieldOptions dccl_field_options()
            {
                if(this_field())
                    return this_field()->options().GetExtension(goby::field).dccl();
                else
                    throw(DCCLException("Cannot call dccl_field on base message (has no *field* option extension"));                
                
            }
            
            /* template<typename Extension> */
            /*     typename Extension::TypeTraits::ConstType get(const Extension& e) */
            /* { */
            /*     if(this_field()) */
            /*         return this_field()->options().GetExtension(e); */
            /*     else */
            /*         throw(DCCLException("Cannot call get on base message (has no *field* option extension"));                 */
            /* } */

            
            /* template<typename Extension> */
            /*     bool has(const Extension& e) */
            /* { */
            /*     if(this_field()) */
            /*         return this_field()->options().HasExtension(e); */
            /*     else */
            /*         return false; */
            /* } */
            
            /* template<typename Extension> */
            /*     void require(const Extension& e, const std::string& name) */
            /* { */
            /*     if(!has(e)) */
            /*     { */
            /*         if(this_field()) */
            /*             throw(DCCLException("Field " + this_field()->name() + " missing option extension called `" + name + "`.")); */
            /*     } */
            
            /* }             */
            
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
            
            // 
            // VIRTUAL
            //

            // contain boost::any
            virtual Bitset any_encode(const boost::any& field_value) = 0;
            virtual boost::any any_decode(Bitset* bits) = 0;

            virtual boost::any any_pre_encode(const boost::any& field_value)
            { return field_value; }
            virtual boost::any any_post_decode(const boost::any& wire_value)
            { return wire_value; }

            virtual unsigned any_size(const boost::any& field_value) = 0;
            virtual void any_run_hooks(const boost::any& field_value);

            // no boost::any
            virtual void validate() { }
            virtual std::string info();
            
            virtual unsigned max_size() = 0;
            virtual unsigned min_size() = 0;            

            virtual unsigned max_size_repeated();
            virtual unsigned min_size_repeated();
            
            virtual bool variable_size() { return true; }

            

            // TODO (tes): make these virtual for end user
            goby::acomms::Bitset
                any_encode_repeated(const std::vector<boost::any>& field_values);
            std::vector<boost::any>
                any_decode_repeated(Bitset* repeated_bits);

            std::vector<boost::any> any_pre_encode_repeated(const std::vector<boost::any>& field_values);
            
            std::vector<boost::any> any_post_decode_repeated(
                const std::vector<boost::any>& wire_values);
            
            unsigned any_size_repeated(const std::vector<boost::any>& field_values);
            

            static boost::signal<void (unsigned size)> get_more_bits;

            class BitsHandler
            {
              public:
              BitsHandler(Bitset* out_pool, Bitset* in_pool, bool lsb_first = true)
                  : lsb_first_(lsb_first),
                    in_pool_(in_pool),
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
                bool lsb_first_;
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

                friend class DCCLFieldCodecBase;
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
            static boost::ptr_map<int, boost::signal<void (const boost::any& field_value, const boost::any& wire_value, const boost::any& extension_value)> >  wire_value_hooks_;
            
            
            std::string name_;
            google::protobuf::FieldDescriptor::Type field_type_;
            google::protobuf::FieldDescriptor::CppType wire_type_;

        };

        inline std::ostream& operator<<(std::ostream& os, const DCCLFieldCodecBase& field_codec )
        {
            using google::protobuf::FieldDescriptor;
            return os << "[FieldCodec '" << field_codec.name() << "']: field type: "
                      << DCCLTypeHelper::find(field_codec.field_type())->as_str()
                      << " (" << DCCLTypeHelper::find(FieldDescriptor::TypeToCppType(field_codec.field_type()))->as_str()
                      << ") | wire type: " << DCCLTypeHelper::find(field_codec.wire_type())->as_str();
        }

        inline DCCLException type_error(const std::string& action,
                                        const std::type_info& expected,
                                        const std::type_info& got)
        {
            std::string e = "error " + action + ", expected: ";
            e += expected.name();
            e += ", got ";
            e += got.name();
            return DCCLException(e);
        }
          

        // if WireType == FieldType, we don't have to add anything
        template <typename WireType, typename FieldType, class Enable = void> 
            class DCCLFieldCodecSwitcher : public DCCLFieldCodecBase
            { };

        // if not the same WireType and FieldType, add these extra methods
        template <typename WireType, typename FieldType>
            class DCCLFieldCodecSwitcher<WireType, FieldType,
            typename boost::disable_if<boost::is_same<WireType, FieldType> >::type>
            : public DCCLFieldCodecBase
            {
              protected:
                virtual WireType pre_encode(const FieldType& field_value) = 0;
                virtual FieldType post_decode(const WireType& wire_value) = 0;

              private:
                virtual boost::any any_pre_encode(const boost::any& field_value)
                {
                    try
                    {
                        if(!field_value.empty())
                            return pre_encode(boost::any_cast<FieldType>(field_value));
                        else
                            return field_value;
                    }
                    catch(boost::bad_any_cast&)
                    {
                        throw(type_error("pre_encode", typeid(FieldType), field_value.type()));
                    }
                    catch(DCCLNullValueException&)
                    {
                        return boost::any();
                    }
                }
          
                virtual boost::any any_post_decode(const boost::any& wire_value)
                {
                    try
                    {
                        if(!wire_value.empty())
                            return post_decode(boost::any_cast<WireType>(wire_value));
                        else
                            return wire_value;
                  
                    }
                    catch(boost::bad_any_cast&)
                    {
                        throw(type_error("post_decode", typeid(WireType), wire_value.type()));
                    }
                    catch(DCCLNullValueException&)
                    {
                        return boost::any();
                    }
                }


            };
        
        template<typename WireType, typename FieldType = WireType>
            class DCCLTypedFieldCodec : public DCCLFieldCodecSwitcher<WireType, FieldType>
        {
          public:
          typedef WireType wire_type;
          typedef FieldType field_type;
            

          public:
          virtual Bitset encode() = 0;          
          virtual Bitset encode(const WireType& wire_value) = 0;
          virtual WireType decode(Bitset* bits) = 0;
          virtual unsigned size() = 0;
          virtual unsigned size(const FieldType& wire_value) = 0;
          
          private:
          unsigned any_size(const boost::any& field_value)
          {
              try
              {
                  if(field_value.empty())
                      return size();
                  else
                      return size(boost::any_cast<FieldType>(field_value));
              }
              catch(boost::bad_any_cast&)
              {
                  throw(type_error("size", typeid(FieldType), field_value.type()));
              }
          }
          

          Bitset any_encode(const boost::any& wire_value)
          {
              try
              {
                  if(wire_value.empty())
                      return encode();
                  else
                      return encode(boost::any_cast<WireType>(wire_value));
              }
              catch(boost::bad_any_cast&)
              {
                  throw(type_error("encode", typeid(WireType), wire_value.type()));
              }
          }
          
          boost::any any_decode(Bitset* bits)
          {
              try
              {
                  return decode(bits);
              }
              catch(DCCLNullValueException&)
              {
                  return boost::any();
              }
          }
          


        };
        
    }
}


#endif
