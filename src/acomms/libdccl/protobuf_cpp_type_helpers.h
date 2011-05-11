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
#include <google/protobuf/message.h>


namespace goby
{
    namespace acomms
    {
        class FromProtoTypeBase
        {
          public:
            virtual std::string as_str() { return "TYPE_UNKNOWN"; }   
        };        
        
        template<google::protobuf::FieldDescriptor::Type> class FromProtoType { };
        template<> class FromProtoType<google::protobuf::FieldDescriptor::TYPE_DOUBLE>
            : public FromProtoTypeBase
        {
          public:
            std::string as_str() { return "TYPE_DOUBLE"; }
        };
        template<> class FromProtoType<google::protobuf::FieldDescriptor::TYPE_FLOAT>
            : public FromProtoTypeBase
        {
          public:
            std::string as_str() { return "TYPE_FLOAT"; }
        };
        template<> class FromProtoType<google::protobuf::FieldDescriptor::TYPE_INT64>
            : public FromProtoTypeBase
        {
          public:
            std::string as_str() { return "TYPE_INT64"; }
        };
        template<> class FromProtoType<google::protobuf::FieldDescriptor::TYPE_UINT64>
            : public FromProtoTypeBase
        {
          public:
            std::string as_str() { return "TYPE_UINT64"; }
        };
        template<> class FromProtoType<google::protobuf::FieldDescriptor::TYPE_INT32>
            : public FromProtoTypeBase
        {
          public:
            std::string as_str() { return "TYPE_INT32"; }
        };
        template<> class FromProtoType<google::protobuf::FieldDescriptor::TYPE_FIXED64>
            : public FromProtoTypeBase
        {
          public:
            std::string as_str() { return "TYPE_FIXED64"; }
        };
        template<> class FromProtoType<google::protobuf::FieldDescriptor::TYPE_FIXED32>
            : public FromProtoTypeBase
        {
          public:
            std::string as_str() { return "TYPE_FIXED32"; }
        };
        template<> class FromProtoType<google::protobuf::FieldDescriptor::TYPE_BOOL>
            : public FromProtoTypeBase
        {
          public:
            std::string as_str() { return "TYPE_BOOL"; }
        };
        template<> class FromProtoType<google::protobuf::FieldDescriptor::TYPE_STRING>
            : public FromProtoTypeBase
        {
          public:
            std::string as_str() { return "TYPE_STRING"; }
        };
        template<> class FromProtoType<google::protobuf::FieldDescriptor::TYPE_GROUP>
            : public FromProtoTypeBase
        {
          public:
            std::string as_str() { return "TYPE_GROUP"; }
        };
        template<> class FromProtoType<google::protobuf::FieldDescriptor::TYPE_MESSAGE>
            : public FromProtoTypeBase
        {
          public:
            std::string as_str() { return "TYPE_MESSAGE"; }
        };
        template<> class FromProtoType<google::protobuf::FieldDescriptor::TYPE_BYTES>
            : public FromProtoTypeBase
        {
          public:
            std::string as_str() { return "TYPE_BYTES"; }
        };
        template<> class FromProtoType<google::protobuf::FieldDescriptor::TYPE_UINT32>
            : public FromProtoTypeBase
        {
          public:
            std::string as_str() { return "TYPE_UINT32"; }
        };
        template<> class FromProtoType<google::protobuf::FieldDescriptor::TYPE_ENUM>
            : public FromProtoTypeBase
        {
          public:
            std::string as_str() { return "TYPE_ENUM"; }
        };
        template<> class FromProtoType<google::protobuf::FieldDescriptor::TYPE_SFIXED32>
            : public FromProtoTypeBase
        {
          public:
            std::string as_str() { return "TYPE_SFIXED32"; }
        };
        template<> class FromProtoType<google::protobuf::FieldDescriptor::TYPE_SFIXED64>
            : public FromProtoTypeBase
        {
          public:
            std::string as_str() { return "TYPE_SFIXED64"; }
        };
        template<> class FromProtoType<google::protobuf::FieldDescriptor::TYPE_SINT32>
            : public FromProtoTypeBase
        {
          public:
            std::string as_str() { return "TYPE_SINT32"; }
        };
        template<> class FromProtoType<google::protobuf::FieldDescriptor::TYPE_SINT64>
            : public FromProtoTypeBase
        {
          public:
            std::string as_str() { return "TYPE_SINT64"; }
        };

        
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

            boost::any get_value(const google::protobuf::Message& msg)
            {
                return _get_value(0, msg);
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

            void set_value(google::protobuf::Message* msg,
                           boost::any value)
            {
                if(value.empty())
                    return;
                else
                    _set_value(0, msg, value);
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
          private:
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
          private:
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
          private:
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
          private:
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
          private:
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
          private:
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
          private:
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
          private:
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
        
        template<> class FromProtoCppType<google::protobuf::FieldDescriptor::CPPTYPE_ENUM>
            : public FromProtoCppTypeBase
        {
          public:
            typedef const google::protobuf::EnumValueDescriptor* const_type;
            typedef google::protobuf::EnumValueDescriptor* mutable_type;
            
            std::string as_str() { return "CPPTYPE_ENUM"; }
          private:
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


        template<> class FromProtoCppType<google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE>
            : public FromProtoCppTypeBase
        {
          public:
            typedef const google::protobuf::Message* const_type;
            typedef boost::shared_ptr<google::protobuf::Message> mutable_type;
            std::string as_str() { return "CPPTYPE_MESSAGE"; }

          protected:
            virtual boost::any _get_value(
                const google::protobuf::FieldDescriptor* field,
                const google::protobuf::Message& msg)
            {
                if(field)
                    return &(msg.GetReflection()->GetMessage(msg, field));
                else
                    return &msg;
            }

            virtual boost::any _get_repeated_value(
                const google::protobuf::FieldDescriptor* field,
                const google::protobuf::Message& msg,
                int index)
            { return &(msg.GetReflection()->GetRepeatedMessage(msg, field, index)); }
            
                
            virtual void _set_value(const google::protobuf::FieldDescriptor* field,
                                    google::protobuf::Message* msg,
                                    boost::any value)
            {
                try
                {
                    const_type p = boost::any_cast<const_type>(value);

                    if(field)
                        msg->GetReflection()->MutableMessage(msg, field)->MergeFrom(*p);
                    else
                        msg->MergeFrom(*p);
                    
                }
                catch(boost::bad_any_cast& e)
                {
                    mutable_type p = boost::any_cast<mutable_type>(value);
                    if(field)
                        msg->GetReflection()->MutableMessage(msg, field)->MergeFrom(*p);
                    else
                        msg->MergeFrom(*p);
                }
                
            }
            virtual void _add_value(const google::protobuf::FieldDescriptor* field,
                                    google::protobuf::Message* msg,
                                    boost::any value)
            {
                try
                {
                    const_type p = boost::any_cast<const_type>(value);
                    msg->GetReflection()->AddMessage(msg, field)->MergeFrom(*p);
                }
                catch(boost::bad_any_cast& e)
                {
                    mutable_type p = boost::any_cast<mutable_type>(value);
                    msg->GetReflection()->AddMessage(msg, field)->MergeFrom(*p);
                }
            }
            
        };
        

        template<typename CustomMessage>
            class FromProtoCustomMessage : public FromProtoCppType<google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE>
        {
          public:
            typedef const CustomMessage& const_type;
            typedef CustomMessage mutable_type;
            
          private:
            typedef FromProtoCppType<google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE> Parent;
            
            virtual boost::any _get_value(
                const google::protobuf::FieldDescriptor* field,
                const google::protobuf::Message& msg)
            {
                Parent::const_type p = boost::any_cast<Parent::const_type>(Parent::_get_value(field, msg));
                return dynamic_cast<const_type>(*p);
            }
            virtual boost::any _get_repeated_value(
                const google::protobuf::FieldDescriptor* field,
                const google::protobuf::Message& msg,
                int index)
            {
                Parent::const_type p = boost::any_cast<Parent::const_type>(Parent::_get_repeated_value(field, msg, index));
                return dynamic_cast<const_type>(*p);
            }
            virtual void _set_value(const google::protobuf::FieldDescriptor* field,
                                    google::protobuf::Message* msg,
                                    boost::any value)
            {
                const_type v = boost::any_cast<const_type>(value);
                Parent::const_type p = &v;
                Parent::_set_value(field, msg, p);
            }
            virtual void _add_value(const google::protobuf::FieldDescriptor* field,
                                    google::protobuf::Message* msg,
                                    boost::any value)
            {
                const_type v = boost::any_cast<const_type>(value);
                Parent::const_type p = &v;
                Parent::_add_value(field, msg, p);
                
            }
            
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
           class ToProtoCppType<const google::protobuf::EnumValueDescriptor*>
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
