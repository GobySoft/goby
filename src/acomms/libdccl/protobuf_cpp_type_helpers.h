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

#ifndef PROTOBUFCPPTYPEHELPERS20110323H
#define PROTOBUFCPPTYPEHELPERS20110323H

#include <boost/any.hpp>
#include <google/protobuf/descriptor.h>


namespace goby
{
    namespace acomms
    {
        class FromProtoCppTypeBase
        {
          public:
            virtual std::string as_str() { return "CPPTYPE_UNKNOWN"; }

   

            boost::any get_value(const google::protobuf::FieldDescriptor* field,
                                 const google::protobuf::Message& msg)
            {
                const google::protobuf::Reflection* refl = msg.GetReflection();
                if(!refl->HasField(msg, field))
                    return boost::any();
                else
                    return _get_value(field, msg);
            }

            
            boost::any get_repeated_value(
                const google::protobuf::FieldDescriptor* field,
                const google::protobuf::Message& msg,
                int index)
            { return _get_repeated_value(field, msg, index); }
            

            void set_value(const google::protobuf::FieldDescriptor* field,
                           google::protobuf::Message* msg,
                           boost::any value)
            {
                if(value.empty())
                    return;
                else
                    _set_value(field, msg, value);
            }

            void add_value(const google::protobuf::FieldDescriptor* field,
                           google::protobuf::Message* msg,
                           boost::any value)
            {
                if(value.empty())
                    return;
                else
                    _add_value(field, msg, value);
            }
            
            virtual void _set_value(const google::protobuf::FieldDescriptor* field,
                                            google::protobuf::Message* msg,
                                            boost::any value)
            { return; }

            virtual void _add_value(const google::protobuf::FieldDescriptor* field,
                                            google::protobuf::Message* msg,
                                            boost::any value)
            { return; }

            virtual boost::any _get_repeated_value(
                const google::protobuf::FieldDescriptor* field,
                const google::protobuf::Message& msg,
                int index)
            { return boost::any(); }

            virtual boost::any _get_value(
                const google::protobuf::FieldDescriptor* field,
                const google::protobuf::Message& msg)
            { return boost::any(); }
        };        
        
        template<google::protobuf::FieldDescriptor::CppType> class FromProtoCppType { };
        template<> class FromProtoCppType<google::protobuf::FieldDescriptor::CPPTYPE_DOUBLE>
            : public FromProtoCppTypeBase
        {
          public:
            typedef double const_type;
            typedef const_type mutable_type;
            std::string as_str() { return "CPPTYPE_DOUBLE"; }
            boost::any _get_value(
                const google::protobuf::FieldDescriptor* field,
                const google::protobuf::Message& msg)
            { return msg.GetReflection()->GetDouble(msg, field); }
            boost::any _get_repeated_value(
                const google::protobuf::FieldDescriptor* field,
                const google::protobuf::Message& msg,
                int index)
            { return msg.GetReflection()->GetRepeatedDouble(msg, field, index); }
            void _set_value(const google::protobuf::FieldDescriptor* field,
                                    google::protobuf::Message* msg,
                                    boost::any value)
            { msg->GetReflection()->SetDouble(msg, field, boost::any_cast<const_type>(value)); }
            void _add_value(const google::protobuf::FieldDescriptor* field,
                                    google::protobuf::Message* msg,
                                    boost::any value)
            { msg->GetReflection()->AddDouble(msg, field, boost::any_cast<const_type>(value)); }
        };

        template<> class FromProtoCppType<google::protobuf::FieldDescriptor::CPPTYPE_FLOAT>
            : public FromProtoCppTypeBase
        {
          public:
            typedef float const_type;
            typedef const_type mutable_type;
            std::string as_str() { return "CPPTYPE_FLOAT"; }
            boost::any _get_value(
                const google::protobuf::FieldDescriptor* field,
                const google::protobuf::Message& msg)
            { return msg.GetReflection()->GetFloat(msg, field); }
            boost::any _get_repeated_value(
                const google::protobuf::FieldDescriptor* field,
                const google::protobuf::Message& msg,
                int index)
            { return msg.GetReflection()->GetRepeatedFloat(msg, field, index); }
            void _set_value(const google::protobuf::FieldDescriptor* field,
                                    google::protobuf::Message* msg,
                                    boost::any value)
            { msg->GetReflection()->SetFloat(msg, field, boost::any_cast<const_type>(value)); }
            void _add_value(const google::protobuf::FieldDescriptor* field,
                                    google::protobuf::Message* msg,
                                    boost::any value)
            { msg->GetReflection()->AddFloat(msg, field, boost::any_cast<const_type>(value)); }
        };
        template<> class FromProtoCppType<google::protobuf::FieldDescriptor::CPPTYPE_INT32>
            : public FromProtoCppTypeBase
        {
          public:
            typedef google::protobuf::int32 const_type;
            typedef const_type mutable_type;
            std::string as_str() { return "CPPTYPE_INT32"; }
            boost::any _get_value(
                const google::protobuf::FieldDescriptor* field,
                const google::protobuf::Message& msg)
            { return msg.GetReflection()->GetInt32(msg, field); }
            boost::any _get_repeated_value(
                const google::protobuf::FieldDescriptor* field,
                const google::protobuf::Message& msg,
                int index)
            { return msg.GetReflection()->GetRepeatedInt32(msg, field, index); }
            void _set_value(const google::protobuf::FieldDescriptor* field,
                                    google::protobuf::Message* msg,
                                    boost::any value)
            { msg->GetReflection()->SetInt32(msg, field, boost::any_cast<const_type>(value)); }
            void _add_value(const google::protobuf::FieldDescriptor* field,
                                    google::protobuf::Message* msg,
                                    boost::any value)
            { msg->GetReflection()->AddInt32(msg, field, boost::any_cast<const_type>(value)); }
        };
        template<> class FromProtoCppType<google::protobuf::FieldDescriptor::CPPTYPE_INT64>
            : public FromProtoCppTypeBase
        {
          public:
            typedef google::protobuf::int64 const_type;
            typedef const_type mutable_type;
            std::string as_str() { return "CPPTYPE_INT64"; }
            boost::any _get_value(
                const google::protobuf::FieldDescriptor* field,
                const google::protobuf::Message& msg)
            { return msg.GetReflection()->GetInt64(msg, field); }
            boost::any _get_repeated_value(
                const google::protobuf::FieldDescriptor* field,
                const google::protobuf::Message& msg,
                int index)
            { return msg.GetReflection()->GetRepeatedInt64(msg, field, index); }
            void _set_value(const google::protobuf::FieldDescriptor* field,
                                    google::protobuf::Message* msg,
                                    boost::any value)
            { msg->GetReflection()->SetInt64(msg, field, boost::any_cast<const_type>(value)); }
            void _add_value(const google::protobuf::FieldDescriptor* field,
                                    google::protobuf::Message* msg,
                                    boost::any value)
            { msg->GetReflection()->AddInt64(msg, field, boost::any_cast<const_type>(value)); }
        };
        template<> class FromProtoCppType<google::protobuf::FieldDescriptor::CPPTYPE_UINT32>
            : public FromProtoCppTypeBase
        {
          public:
            typedef google::protobuf::uint32 const_type;
            typedef const_type mutable_type;
            std::string as_str() { return "CPPTYPE_UINT32"; }
            boost::any _get_value(
                const google::protobuf::FieldDescriptor* field,
                const google::protobuf::Message& msg)
            { return msg.GetReflection()->GetUInt32(msg, field); }
            boost::any _get_repeated_value(
                const google::protobuf::FieldDescriptor* field,
                const google::protobuf::Message& msg,
                int index)
            { return msg.GetReflection()->GetRepeatedUInt32(msg, field, index); }
            void _set_value(const google::protobuf::FieldDescriptor* field,
                                    google::protobuf::Message* msg,
                                    boost::any value)
            { msg->GetReflection()->SetUInt32(msg, field, boost::any_cast<const_type>(value)); }
            void _add_value(const google::protobuf::FieldDescriptor* field,
                                    google::protobuf::Message* msg,
                                    boost::any value)
            { msg->GetReflection()->AddUInt32(msg, field, boost::any_cast<const_type>(value)); }
        };
        template<> class FromProtoCppType<google::protobuf::FieldDescriptor::CPPTYPE_UINT64>
            : public FromProtoCppTypeBase
        {
          public:
            typedef google::protobuf::uint64 const_type;
            typedef const_type mutable_type;

            std::string as_str() { return "CPPTYPE_UINT64"; }
            boost::any _get_value(
                const google::protobuf::FieldDescriptor* field,
                const google::protobuf::Message& msg)
            { return msg.GetReflection()->GetUInt64(msg, field); }
            boost::any _get_repeated_value(
                const google::protobuf::FieldDescriptor* field,
                const google::protobuf::Message& msg,
                int index)
            { return msg.GetReflection()->GetRepeatedUInt64(msg, field, index); }
            void _set_value(const google::protobuf::FieldDescriptor* field,
                                    google::protobuf::Message* msg,
                                    boost::any value)
            { msg->GetReflection()->SetUInt64(msg, field, boost::any_cast<const_type>(value)); }
            void _add_value(const google::protobuf::FieldDescriptor* field,
                                    google::protobuf::Message* msg,
                                    boost::any value)
            { msg->GetReflection()->AddUInt64(msg, field, boost::any_cast<const_type>(value)); }
        };
        template<> class FromProtoCppType<google::protobuf::FieldDescriptor::CPPTYPE_BOOL>
            : public FromProtoCppTypeBase
        {
          public:
            typedef bool const_type;
            typedef const_type mutable_type;
            std::string as_str() { return "CPPTYPE_BOOL"; }
            boost::any _get_value(
                const google::protobuf::FieldDescriptor* field,
                const google::protobuf::Message& msg)
            { return msg.GetReflection()->GetBool(msg, field); }
            boost::any _get_repeated_value(
                const google::protobuf::FieldDescriptor* field,
                const google::protobuf::Message& msg,
                int index)
            { return msg.GetReflection()->GetRepeatedBool(msg, field, index); }
            void _set_value(const google::protobuf::FieldDescriptor* field,
                                    google::protobuf::Message* msg,
                                    boost::any value)
            { msg->GetReflection()->SetBool(msg, field, boost::any_cast<const_type>(value)); }
            void _add_value(const google::protobuf::FieldDescriptor* field,
                                    google::protobuf::Message* msg,
                                    boost::any value)
            { msg->GetReflection()->AddBool(msg, field, boost::any_cast<const_type>(value)); }
        };
        template<> class FromProtoCppType<google::protobuf::FieldDescriptor::CPPTYPE_STRING>
            : public FromProtoCppTypeBase
        {
          public:
            typedef std::string const_type;
            typedef const_type mutable_type;
            std::string as_str() { return "CPPTYPE_STRING"; }
            boost::any _get_value(
                const google::protobuf::FieldDescriptor* field,
                const google::protobuf::Message& msg)
            { return msg.GetReflection()->GetString(msg, field); }
            boost::any _get_repeated_value(
                const google::protobuf::FieldDescriptor* field,
                const google::protobuf::Message& msg,
                int index)
            { return msg.GetReflection()->GetRepeatedString(msg, field, index); }
            void _set_value(const google::protobuf::FieldDescriptor* field,
                                    google::protobuf::Message* msg,
                                    boost::any value)
            { msg->GetReflection()->SetString(msg, field, boost::any_cast<const_type>(value)); }
            void _add_value(const google::protobuf::FieldDescriptor* field,
                                    google::protobuf::Message* msg,
                                    boost::any value)
            { msg->GetReflection()->AddString(msg, field, boost::any_cast<const_type>(value)); }
        };
        

        template<> class FromProtoCppType<google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE>
            : public FromProtoCppTypeBase
        {
          public:
            typedef const google::protobuf::Message* const_type;
            typedef google::protobuf::Message* mutable_type;
            std::string as_str() { return "CPPTYPE_MESSAGE"; }
            boost::any _get_value(
                const google::protobuf::FieldDescriptor* field,
                const google::protobuf::Message& msg)
            { return &(msg.GetReflection()->GetMessage(msg, field)); }
            boost::any _get_repeated_value(
                const google::protobuf::FieldDescriptor* field,
                const google::protobuf::Message& msg,
                int index)
            { return &(msg.GetReflection()->GetRepeatedMessage(msg, field, index)); }
            void _set_value(const google::protobuf::FieldDescriptor* field,
                            google::protobuf::Message* msg,
                            boost::any value)
            {
                /* const google::protobuf::Message* msg_value = boost::any_cast<const_type>(value); */
                /* if(!msg_value) */
                /*     msg_value = boost::any_cast<mutable_type>(value); */
                
                /* msg->GetReflection()->MutableMessage(msg, field)->CopyFrom(*msg_value); */
            }
            void _add_value(const google::protobuf::FieldDescriptor* field,
                                    google::protobuf::Message* msg,
                                    boost::any value)
            {
                /* const google::protobuf::Message* msg_value = boost::any_cast<const_type>(value); */
                /* if(!msg_value) */
                /*     msg_value = boost::any_cast<mutable_type>(value); */
                
                /* msg->GetReflection()->AddMessage(msg, field)->CopyFrom(*msg_value); */
            }
            
        };
        template<> class FromProtoCppType<google::protobuf::FieldDescriptor::CPPTYPE_ENUM>
            : public FromProtoCppTypeBase
        {
          public:
            typedef const google::protobuf::EnumValueDescriptor* const_type;
            typedef google::protobuf::EnumValueDescriptor* mutable_type;
            
            std::string as_str() { return "CPPTYPE_ENUM"; }
            boost::any _get_value(
                const google::protobuf::FieldDescriptor* field,
                const google::protobuf::Message& msg)
            { return msg.GetReflection()->GetEnum(msg, field); }
            boost::any _get_repeated_value(
                const google::protobuf::FieldDescriptor* field,
                const google::protobuf::Message& msg,
                int index)
            { return msg.GetReflection()->GetRepeatedEnum(msg, field, index); }
            void _set_value(const google::protobuf::FieldDescriptor* field,
                            google::protobuf::Message* msg,
                            boost::any value)
            { msg->GetReflection()->SetEnum(msg, field, boost::any_cast<const_type>(value)); }
            void _add_value(const google::protobuf::FieldDescriptor* field,
                                    google::protobuf::Message* msg,
                                    boost::any value)
            { msg->GetReflection()->AddEnum(msg, field, boost::any_cast<const_type>(value)); }
            
        };
        
        
        template<typename T>
            class ToProtoCppType
        { };
        template<>
            class ToProtoCppType<double>
        {
          public:
            static google::protobuf::FieldDescriptor::CppType as_enum()
            { return google::protobuf::FieldDescriptor::CPPTYPE_DOUBLE; }
        };
       template<>
            class ToProtoCppType<float>
        {
          public:
            static google::protobuf::FieldDescriptor::CppType as_enum()
            { return google::protobuf::FieldDescriptor::CPPTYPE_FLOAT; }
        };
        template<>
            class ToProtoCppType<google::protobuf::int32>
        {
          public:
            static google::protobuf::FieldDescriptor::CppType as_enum()
            { return google::protobuf::FieldDescriptor::CPPTYPE_INT32; }
        };
       template<>
            class ToProtoCppType<google::protobuf::uint32>
        {
          public:
            static google::protobuf::FieldDescriptor::CppType as_enum()
            { return google::protobuf::FieldDescriptor::CPPTYPE_UINT32; }
        };
        template<>
            class ToProtoCppType<google::protobuf::int64>
        {
          public:
            static google::protobuf::FieldDescriptor::CppType as_enum()
            { return google::protobuf::FieldDescriptor::CPPTYPE_INT64; }
        };
       template<>
            class ToProtoCppType<google::protobuf::uint64>
        {
          public:
            static google::protobuf::FieldDescriptor::CppType as_enum()
            { return google::protobuf::FieldDescriptor::CPPTYPE_UINT64; }
        };
       template<>
           class ToProtoCppType<std::string>
        {
          public:
            static google::protobuf::FieldDescriptor::CppType as_enum()
            { return google::protobuf::FieldDescriptor::CPPTYPE_STRING; }
        };
       template<>
           class ToProtoCppType<google::protobuf::EnumDescriptor>
       {
         public:
           static google::protobuf::FieldDescriptor::CppType as_enum()
           { return google::protobuf::FieldDescriptor::CPPTYPE_ENUM; }
       };
       
       template<>
           class ToProtoCppType<bool>
       {
         public:
           static google::protobuf::FieldDescriptor::CppType as_enum()
           { return google::protobuf::FieldDescriptor::CPPTYPE_BOOL; }
       };
       
       template<>
           class ToProtoCppType<google::protobuf::Message>
       {
         public:
           static google::protobuf::FieldDescriptor::CppType as_enum()
           { return google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE; }
       };
    }
}












#endif
