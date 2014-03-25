// Copyright 2009-2014 Toby Schneider (https://launchpad.net/~tes)
//                     GobySoft, LLC (2013-)
//                     Massachusetts Institute of Technology (2007-2014)
//                     Goby Developers Team (https://launchpad.net/~goby-dev)
// 
//
// This file is part of the Goby Underwater Autonomy Project Libraries
// ("The Goby Libraries").
//
// The Goby Libraries are free software: you can redistribute them and/or modify
// them under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 2.1 of the License, or
// (at your option) any later version.
//
// The Goby Libraries are distributed in the hope that they will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with Goby.  If not, see <http://www.gnu.org/licenses/>.



#ifndef PROTOBUFCPPTYPEHELPERS20110323H
#define PROTOBUFCPPTYPEHELPERS20110323H

#include <boost/any.hpp>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>


namespace goby
{
    namespace acomms
    {
        /// \brief Provides various representations of a google::protobuf::FieldDescriptor::Type enumeration. Implementations are provided for all enumerations.
        class FromProtoTypeBase
        {
          public:
            virtual ~FromProtoTypeBase() { }
            /// string representation of the google::protobuf::FieldDescriptor::Type.
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

        
        /// \brief Provides various representations of a google::protobuf::FieldDescriptor::CppType enumeration, and ways to access the google::protobuf::Reflection object for a given type.
        class FromProtoCppTypeBase
        {
          public:
            virtual ~FromProtoCppTypeBase() { }
            
            /// string representation
            virtual std::string as_str() { return "CPPTYPE_UNKNOWN"; }   

            /// \brief Get a given field's value from the provided message.
            ///
            /// \param field Field to get value for.
            /// \param msg Message to get value from.
            /// \return boost::any containing the value. The type is usually the type returned by google::protobuf::Reflection::Get<i>Type</i> where <i>Type</i> is the corresponding google::protobuf::FieldDescriptor::CppType. (See http://code.google.com/apis/protocolbuffers/docs/reference/cpp/google.protobuf.message.html#Reflection).
            boost::any get_value(const google::protobuf::FieldDescriptor* field,
                                 const google::protobuf::Message& msg)
            {
                const google::protobuf::Reflection* refl = msg.GetReflection();
                if(!refl->HasField(msg, field))
                    return boost::any();
                else
                    return _get_value(field, msg);
            }

            /// \brief Get the value of the entire base message (only works for CPPTYPE_MESSAGE)
            boost::any get_value(const google::protobuf::Message& msg)
            {
                return _get_value(0, msg);
            }
            
            /// \brief Get the value of a repeated field at a given index.
            ///
            /// \param field Field to get value for.
            /// \param msg Message to get value from.
            /// \param index Index of the repeated array to get value for.
            /// \return boost::any containing the value. The type is usually the type returned by google::protobuf::Reflection::Get<i>Type</i> where <i>Type</i> is the corresponding google::protobuf::FieldDescriptor::CppType. (See http://code.google.com/apis/protocolbuffers/docs/reference/cpp/google.protobuf.message.html#Reflection).
            boost::any get_repeated_value(
                const google::protobuf::FieldDescriptor* field,
                const google::protobuf::Message& msg,
                int index)
            { return _get_repeated_value(field, msg, index); }
            
            
            /// \brief Set a given field's value in the provided message.
            ///
            /// \param field Field to set value for.
            /// \param msg Message to set value in.
            /// \param value boost::any containing the value to set. The type is usually the type required by google::protobuf::Reflection::Set<i>Type</i> where <i>Type</i> is the corresponding google::protobuf::FieldDescriptor::CppType. (See http://code.google.com/apis/protocolbuffers/docs/reference/cpp/google.protobuf.message.html#Reflection).            
            void set_value(const google::protobuf::FieldDescriptor* field,
                           google::protobuf::Message* msg,
                           boost::any value)
            {
                if(value.empty())
                    return;
                else
                    _set_value(field, msg, value);
            }

            /// \brief Set the value of the entire base message (only works for CPPTYPE_MESSAGE)
            void set_value(google::protobuf::Message* msg,
                           boost::any value)
            {
                if(value.empty())
                    return;
                else
                    _set_value(0, msg, value);
            }

            
            /// \brief Add a new entry for a repeated field to the back.
            /// \param msg Message to set value in.
            /// \return boost::any containing the value. The type is usually the type returned by google::protobuf::Reflection::Add<i>Type</i> where <i>Type</i> is the corresponding google::protobuf::FieldDescriptor::CppType. (See http://code.google.com/apis/protocolbuffers/docs/reference/cpp/google.protobuf.message.html#Reflection).
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
            typedef double type;
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
            { msg->GetReflection()->SetDouble(msg, field, boost::any_cast<type>(value)); }
            void _add_value(const google::protobuf::FieldDescriptor* field,
                            google::protobuf::Message* msg,
                            boost::any value)
            { msg->GetReflection()->AddDouble(msg, field, boost::any_cast<type>(value)); }
        };

        template<> class FromProtoCppType<google::protobuf::FieldDescriptor::CPPTYPE_FLOAT>
            : public FromProtoCppTypeBase
        {
          public:
            typedef float type;
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
            { msg->GetReflection()->SetFloat(msg, field, boost::any_cast<type>(value)); }
            void _add_value(const google::protobuf::FieldDescriptor* field,
                            google::protobuf::Message* msg,
                            boost::any value)
            { msg->GetReflection()->AddFloat(msg, field, boost::any_cast<type>(value)); }
        };
        template<> class FromProtoCppType<google::protobuf::FieldDescriptor::CPPTYPE_INT32>
            : public FromProtoCppTypeBase
        {
          public:
            typedef google::protobuf::int32 type;
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
            { msg->GetReflection()->SetInt32(msg, field, boost::any_cast<type>(value)); }
            void _add_value(const google::protobuf::FieldDescriptor* field,
                            google::protobuf::Message* msg,
                            boost::any value)
            { msg->GetReflection()->AddInt32(msg, field, boost::any_cast<type>(value)); }
        };
        template<> class FromProtoCppType<google::protobuf::FieldDescriptor::CPPTYPE_INT64>
            : public FromProtoCppTypeBase
        {
          public:
            typedef google::protobuf::int64 type;
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
            { msg->GetReflection()->SetInt64(msg, field, boost::any_cast<type>(value)); }
            void _add_value(const google::protobuf::FieldDescriptor* field,
                            google::protobuf::Message* msg,
                            boost::any value)
            { msg->GetReflection()->AddInt64(msg, field, boost::any_cast<type>(value)); }
        };
        template<> class FromProtoCppType<google::protobuf::FieldDescriptor::CPPTYPE_UINT32>
            : public FromProtoCppTypeBase
        {
          public:
            typedef google::protobuf::uint32 type;
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
            { msg->GetReflection()->SetUInt32(msg, field, boost::any_cast<type>(value)); }
            void _add_value(const google::protobuf::FieldDescriptor* field,
                            google::protobuf::Message* msg,
                            boost::any value)
            { msg->GetReflection()->AddUInt32(msg, field, boost::any_cast<type>(value)); }
        };
        template<> class FromProtoCppType<google::protobuf::FieldDescriptor::CPPTYPE_UINT64>
            : public FromProtoCppTypeBase
        {
          public:
            typedef google::protobuf::uint64 type;

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
            { msg->GetReflection()->SetUInt64(msg, field, boost::any_cast<type>(value)); }
            void _add_value(const google::protobuf::FieldDescriptor* field,
                            google::protobuf::Message* msg,
                            boost::any value)
            { msg->GetReflection()->AddUInt64(msg, field, boost::any_cast<type>(value)); }
        };
        template<> class FromProtoCppType<google::protobuf::FieldDescriptor::CPPTYPE_BOOL>
            : public FromProtoCppTypeBase
        {
          public:
            typedef bool type;
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
            { msg->GetReflection()->SetBool(msg, field, boost::any_cast<type>(value)); }
            void _add_value(const google::protobuf::FieldDescriptor* field,
                            google::protobuf::Message* msg,
                            boost::any value)
            { msg->GetReflection()->AddBool(msg, field, boost::any_cast<type>(value)); }
        };
        template<> class FromProtoCppType<google::protobuf::FieldDescriptor::CPPTYPE_STRING>
            : public FromProtoCppTypeBase
        {
          public:
            typedef std::string type;
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
            { msg->GetReflection()->SetString(msg, field, boost::any_cast<type>(value)); }
            void _add_value(const google::protobuf::FieldDescriptor* field,
                            google::protobuf::Message* msg,
                            boost::any value)
            { msg->GetReflection()->AddString(msg, field, boost::any_cast<type>(value)); }
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

        
        /// Implements FromProtoCppTypeBase for CPPTYPE_MESSAGE using the dynamic google::protobuf::Message as the underlying class.
        template<> class FromProtoCppType<google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE>
            : public FromProtoCppTypeBase
        {
          public:
            typedef const google::protobuf::Message* const_type;
            typedef google::protobuf::Message* mutable_type;
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
        
        /// Implements FromProtoCppTypeBase for CPPTYPE_MESSAGE using a specific statically generated Protobuf class.
        template<typename CustomMessage>
            class FromProtoCustomMessage : public FromProtoCppType<google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE>
        {
          public:
            typedef CustomMessage type;
            
          private:
            typedef FromProtoCppType<google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE> Parent;
            
            virtual boost::any _get_value(
                const google::protobuf::FieldDescriptor* field,
                const google::protobuf::Message& msg)
            {
                Parent::const_type p = boost::any_cast<Parent::const_type>(Parent::_get_value(field, msg));
                type r;
                r.CopyFrom(*p);
                return r;
//                return dynamic_cast<const_type>(*p);
            }
            virtual boost::any _get_repeated_value(
                const google::protobuf::FieldDescriptor* field,
                const google::protobuf::Message& msg,
                int index)
            {
                Parent::const_type p = boost::any_cast<Parent::const_type>(Parent::_get_repeated_value(field, msg, index));
                type r;
                r.CopyFrom(*p);
                return r;
//                return dynamic_cast<const_type>(*p);
            }
            virtual void _set_value(const google::protobuf::FieldDescriptor* field,
                                    google::protobuf::Message* msg,
                                    boost::any value)
            {
                type v = boost::any_cast<type>(value);
                Parent::const_type p = &v;
                Parent::_set_value(field, msg, p);
            }
            virtual void _add_value(const google::protobuf::FieldDescriptor* field,
                                    google::protobuf::Message* msg,
                                    boost::any value)
            {
                type v = boost::any_cast<type>(value);
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
