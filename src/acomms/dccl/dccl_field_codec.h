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
#include "goby/protobuf/acomms_dccl.pb.h"
#include "goby/protobuf/acomms_option_extensions.pb.h"
#include "goby/util/as.h"
#include "dccl_type_helper.h"
#include "dccl_field_codec_helpers.h"

namespace goby
{
    namespace acomms
    {        
        class DCCLCodec;

        /// \brief Provides a base class for defining DCCL field encoders / decoders.
        class DCCLFieldCodecBase
        {
          public:
            typedef goby::acomms::Bitset Bitset;

            enum MessagePart { HEAD, BODY };



            
            /// \name Constructor, Destructor
            //@{
            
            DCCLFieldCodecBase();
            virtual ~DCCLFieldCodecBase() { }
            //@}
            
            /// \name Information Methods
            //@{
            /// \brief the name of the codec used to identifier it in the .proto custom option extension
            std::string name() const { return name_; }
            /// \brief the type exposed to the user in the original and decoded Protobuf messages
            ///
            /// \return the Google Protobuf message type. See http://code.google.com/apis/protocolbuffers/docs/reference/cpp/google.protobuf.descriptor.html#FieldDescriptor.Type.details
            google::protobuf::FieldDescriptor::Type field_type() const  { return field_type_; }
            /// \brief the C++ type used "on the wire". This is the type visible <i>after</i> pre_encode and <i>before</i> post_decode functions are called.
            ///
            /// The wire type allows codecs to make type changes (e.g. from string to integer) before reusing another codec that knows how to encode that wire type (e.g. DCCLDefaultArithmeticFieldCodec)
            /// \return the C++ type used to encode and decode. See http://code.google.com/apis/protocolbuffers/docs/reference/cpp/google.protobuf.descriptor.html#FieldDescriptor.CppType.details
            google::protobuf::FieldDescriptor::CppType wire_type() const  { return wire_type_; }


            /// \brief Returns the FieldDescriptor (field schema  meta-data) for this field
            ///
            /// \return FieldDescriptor for the current field or 0 if this codec is encoding the base message.
            const google::protobuf::FieldDescriptor* this_field()
            { return !MessageHandler::field_.empty() ? MessageHandler::field_.back() : 0; }
            
            /// \brief Returns the Descriptor (message schema meta-data) for the immediate parent Message
            ///
            /// for:
            /// \code
            /// message Foo
            /// {
            ///    int32 bar = 1;
            ///    FooBar baz = 2;
            /// }
            /// \endcode
            /// returns Descriptor for Foo if this_field() == 0
            /// returns Descriptor for Foo if this_field() == FieldDescriptor for bar
            /// returns Descriptor for FooBar if this_field() == FieldDescriptor for baz
            static const google::protobuf::Descriptor* this_descriptor()
            { return !MessageHandler::desc_.empty() ? MessageHandler::desc_.back() : 0; }

            /// \brief the part of the message currently being encoded (head or body).
            static MessagePart part() { return part_; }
            
            //@}

            /// \name Base message functions
            ///
            /// These are called typically by DCCLCodec to start processing a new message. In this example "Foo" is a base message:
            /// \code
            /// message Foo
            /// {
            ///    int32 bar = 1;
            ///    FooBar baz = 2;
            /// }
            /// \endcode
            //@{

            /// \brief Encode this part (body or head) of the base message
            ///
            /// \param bits pointer to a Bitset where all bits will be pushed on to the most significant end
            /// \param field_value DCCL Message to encode
            /// \param part Part of the message to encode
            void base_encode(Bitset* bits,
                             const google::protobuf::Message& field_value,
                             MessagePart part);

            /// \brief Run hooks on this part of the base message
            ///
            /// See DCCLCodec::run_hooks() for details on the hook functionality.
            /// \param msg DCCL Message to run hooks on
            /// \param part part of the Message to run hooks on
            void base_run_hooks(const google::protobuf::Message& msg, MessagePart part);

            /// \brief Calculate the size (in bits) of the message when it is encoded
             ///
            /// \param bit_size Pointer to unsigned integer to store the result.
            void base_size(unsigned* bit_size, const google::protobuf::Message& msg, MessagePart part);
            void base_decode(Bitset* bits,
                             google::protobuf::Message* field_value,
                             MessagePart part);
            void base_max_size(unsigned* bit_size, const google::protobuf::Descriptor* desc, MessagePart part);
            void base_min_size(unsigned* bit_size, const google::protobuf::Descriptor* desc, MessagePart part);
            void base_validate(const google::protobuf::Descriptor* desc, MessagePart part);
            void base_info(std::ostream* os, const google::protobuf::Descriptor* desc, MessagePart part);
            //@}
            
            /// \name Field functions (primitive types and embedded messages)
            // 
            /// These are called typically by DCCLDefaultMessageCodec to start processing a new field. In this example "bar" and "baz" are fields:
            /// \code
            /// message Foo
            /// {
            ///    int32 bar = 1;
            ///    FooBar baz = 2;
            /// }
            //@{
            void base_pre_encode(boost::any* wire_value, const boost::any& field_value)
            { any_pre_encode(wire_value, field_value); }
            void base_pre_encode_repeated(std::vector<boost::any>* wire_values,
                                          const std::vector<boost::any>& field_values)
            { any_pre_encode_repeated(wire_values, field_values); }
            
            // traverse const
            void base_encode(Bitset* bits,
                             const boost::any& field_value,
                             const google::protobuf::FieldDescriptor* field);
            void base_encode_repeated(Bitset* bits,
                                      const std::vector<boost::any>& field_values,
                                      const google::protobuf::FieldDescriptor* field);
            void base_run_hooks(bool* b,
                                const boost::any& field_value,
                                const google::protobuf::FieldDescriptor* field);
            void base_size(unsigned* bit_size, const boost::any& field_value,
                           const google::protobuf::FieldDescriptor* field);
            void base_size_repeated(unsigned* bit_size, const std::vector<boost::any>& field_values,
                                    const google::protobuf::FieldDescriptor* field);

            // traverse mutable
            void base_decode(Bitset* bits,
                             boost::any* field_value,
                             const google::protobuf::FieldDescriptor* field);            
            void base_decode_repeated(Bitset* bits,
                                      std::vector<boost::any>* field_values,
                                      const google::protobuf::FieldDescriptor* field);
            void base_post_decode(const boost::any& wire_value, boost::any* field_value)
            { any_post_decode(wire_value, field_value); }
            void base_post_decode_repeated(const std::vector<boost::any>& wire_values,
                                           std::vector<boost::any>* field_values)
            { any_post_decode_repeated(wire_values, field_values); }
            
            
            // traverse schema (Descriptor)
            void base_max_size(unsigned* bit_size, const google::protobuf::FieldDescriptor* field);
            void base_min_size(unsigned* bit_size, const google::protobuf::FieldDescriptor* field);
            void base_validate(bool* b, const google::protobuf::FieldDescriptor* field);
            void base_info(std::ostream* os, const google::protobuf::FieldDescriptor* field);
            //@}
            
            
            /// \name Hook API methods (Advanced)
            //@{
            static void register_wire_value_hook(
                int extension_number, boost::function<void (const boost::any& field_value,
                                                            const boost::any& wire_value,
                                                            const boost::any& extension_value)> callback)
            { wire_value_hooks_[extension_number].connect(callback); }

            //@}

          protected:
            

            DCCLFieldOptions dccl_field_options()
            {
                if(this_field())
                    return this_field()->options().GetExtension(goby::field).dccl();
                else
                    throw(DCCLException("Cannot call dccl_field on base message (has no *field* option extension"));                
                
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
            
            // 
            // VIRTUAL
            //

            // contain boost::any
            // bits is *just* the bits from the current operation
            virtual void any_encode(Bitset* bits, const boost::any& wire_value) = 0;
            virtual void any_decode(Bitset* bits, boost::any* wire_value) = 0;

            virtual void any_pre_encode(boost::any* wire_value,
                                        const boost::any& field_value) 
            { *wire_value = field_value; }
            virtual void any_post_decode(const boost::any& wire_value,
                                               boost::any* field_value)
            { *field_value = wire_value; }
            
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
            void any_encode_repeated(Bitset* bits, const std::vector<boost::any>& field_values);
            void any_decode_repeated(Bitset* repeated_bits, std::vector<boost::any>* field_values);

            void any_pre_encode_repeated(std::vector<boost::any>* wire_values,
                                    const std::vector<boost::any>& field_values);
            
            void any_post_decode_repeated(const std::vector<boost::any>& wire_values,
                                     std::vector<boost::any>* field_values);
            
            unsigned any_size_repeated(const std::vector<boost::any>& field_values);
            

            friend class BitsHandler;
            
            static boost::signal<void (unsigned size)> get_more_bits;

            
            friend class DCCLFieldCodecManager;
          private:

            // codec information
            void set_name(const std::string& name)
            { name_ = name; }
            void set_field_type(google::protobuf::FieldDescriptor::Type type)
            { field_type_ = type; }
            void set_wire_type(google::protobuf::FieldDescriptor::CppType type)
            { wire_type_ = type; }


            
            void __encode_prepend_bits(const Bitset& new_bits, Bitset* bits)
            {    
                for(int i = 0, n = new_bits.size(); i < n; ++i)
                    bits->push_back(new_bits[i]);
            }
            
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
            class DCCLFieldCodecSelector : public DCCLFieldCodecBase
            { };

        
        // if not the same WireType and FieldType, add these extra methods to
        // handle them 
        template <typename WireType, typename FieldType>
            class DCCLFieldCodecSelector<WireType, FieldType,
            typename boost::disable_if<boost::is_same<WireType, FieldType> >::type>
            : public DCCLFieldCodecBase
            {
              protected:
                virtual WireType pre_encode(const FieldType& field_value) = 0;
                virtual FieldType post_decode(const WireType& wire_value) = 0;

              private:
                virtual void any_pre_encode(boost::any* wire_value,
                                            const boost::any& field_value) 
                {
                    try
                    {
                        if(!field_value.empty())
                            *wire_value = pre_encode(boost::any_cast<FieldType>(field_value));
                    }
                    catch(boost::bad_any_cast&)
                    {
                        throw(type_error("pre_encode", typeid(FieldType), field_value.type()));
                    }
                    catch(DCCLNullValueException&)
                    {
                        *wire_value = boost::any();
                    }
                }
          
                virtual void any_post_decode(const boost::any& wire_value,
                                             boost::any* field_value)
                {
                    try
                    {
                        if(!wire_value.empty())
                            *field_value = post_decode(boost::any_cast<WireType>(wire_value));
                    }
                    catch(boost::bad_any_cast&)
                    {
                        throw(type_error("post_decode", typeid(WireType), wire_value.type()));
                    }
                    catch(DCCLNullValueException&)
                    {
                        *field_value = boost::any();
                    }
                }
            };

        template<typename WireType, typename FieldType = WireType>
            class DCCLTypedFieldCodecBase : public DCCLFieldCodecSelector<WireType, FieldType>
        {
          public:
          typedef WireType wire_type;
          typedef FieldType field_type;

          public:
          virtual Bitset encode() = 0;
          virtual Bitset encode(const WireType& wire_value) = 0;
          virtual WireType decode(const Bitset& bits) = 0;
          virtual unsigned size() = 0;
          virtual unsigned size(const FieldType& wire_value) = 0;
          
          private:
          unsigned any_size(const boost::any& field_value)
          {
              try
              { return field_value.empty() ? size() : size(boost::any_cast<FieldType>(field_value)); }
              catch(boost::bad_any_cast&)
              { throw(type_error("size", typeid(FieldType), field_value.type())); }
          }          
          
          void any_encode(Bitset* bits, const boost::any& wire_value)
          {
              try
              { *bits = wire_value.empty() ? encode() : encode(boost::any_cast<WireType>(wire_value)); }
              catch(boost::bad_any_cast&)
              { throw(type_error("encode", typeid(WireType), wire_value.type())); }
          }
          
        };
        
        // for all types except embedded messages
        template<typename WireType, typename FieldType = WireType, class Enable = void>
            class DCCLTypedFieldCodec : public DCCLTypedFieldCodecBase<WireType, FieldType>
            {
              public:
                virtual WireType decode(const Bitset& bits) = 0;

              private:
                void any_decode(Bitset* bits, boost::any* wire_value)
                {
                    try
                    { *wire_value = decode(*bits); }
                    catch(DCCLNullValueException&)
                    { *wire_value = boost::any(); }
                }
          
            };


        // special for embedded messages to work properly with passing pointers (instead of copying)
        template <typename WireType>
            class DCCLTypedFieldCodec<WireType, WireType, typename boost::enable_if<boost::is_base_of<google::protobuf::Message, WireType> >::type> : public DCCLTypedFieldCodecBase<WireType, WireType>
        {          
              public:
                virtual WireType decode(const Bitset& bits) = 0;

              private:
            void any_decode(Bitset* bits, boost::any* wire_value)
            {
                try
                {
                    google::protobuf::Message* msg = boost::any_cast<google::protobuf::Message* >(*wire_value);  
                    msg->CopyFrom(decode(*bits));
                }
                catch(DCCLNullValueException&)
                {
                    if(DCCLFieldCodecBase::this_field())
                        *wire_value = boost::any();
                }
            }            
        }; 
          
    }
}


#endif
