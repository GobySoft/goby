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

#include "dccl_type_helper.h"

#include <boost/assign.hpp>

goby::acomms::TypeHelper::TypeMap goby::acomms::TypeHelper::type_map_;
goby::acomms::TypeHelper::CppTypeMap goby::acomms::TypeHelper::cpptype_map_;
goby::acomms::TypeHelper::CustomMessageMap goby::acomms::TypeHelper::custom_message_map_;

//
// TypeHelper
//

void goby::acomms::TypeHelper::initialize()
{
    using namespace google::protobuf;    
    using boost::shared_ptr;
    boost::assign::insert (type_map_)
        (static_cast<FieldDescriptor::Type>(0),
         shared_ptr<FromProtoTypeBase>(new FromProtoTypeBase))
        (FieldDescriptor::TYPE_DOUBLE,
         shared_ptr<FromProtoTypeBase>(new FromProtoType<FieldDescriptor::TYPE_DOUBLE>))
        (FieldDescriptor::TYPE_FLOAT,
         shared_ptr<FromProtoTypeBase>(new FromProtoType<FieldDescriptor::TYPE_FLOAT>))
        (FieldDescriptor::TYPE_UINT64,
         shared_ptr<FromProtoTypeBase>(new FromProtoType<FieldDescriptor::TYPE_UINT64>))
        (FieldDescriptor::TYPE_UINT32,
         shared_ptr<FromProtoTypeBase>(new FromProtoType<FieldDescriptor::TYPE_UINT32>))
        (FieldDescriptor::TYPE_FIXED64,
         shared_ptr<FromProtoTypeBase>(new FromProtoType<FieldDescriptor::TYPE_FIXED64>))
        (FieldDescriptor::TYPE_FIXED32,
         shared_ptr<FromProtoTypeBase>(new FromProtoType<FieldDescriptor::TYPE_FIXED32>))
        (FieldDescriptor::TYPE_INT32,
         shared_ptr<FromProtoTypeBase>(new FromProtoType<FieldDescriptor::TYPE_INT32>))
        (FieldDescriptor::TYPE_INT64,
         shared_ptr<FromProtoTypeBase>(new FromProtoType<FieldDescriptor::TYPE_INT64>))
        (FieldDescriptor::TYPE_SFIXED32,
         shared_ptr<FromProtoTypeBase>(new FromProtoType<FieldDescriptor::TYPE_SFIXED32>))
        (FieldDescriptor::TYPE_SFIXED64,
         shared_ptr<FromProtoTypeBase>(new FromProtoType<FieldDescriptor::TYPE_SFIXED64>))
        (FieldDescriptor::TYPE_SINT32,
         shared_ptr<FromProtoTypeBase>(new FromProtoType<FieldDescriptor::TYPE_SINT32>))
        (FieldDescriptor::TYPE_SINT64,
         shared_ptr<FromProtoTypeBase>(new FromProtoType<FieldDescriptor::TYPE_SINT64>))
        (FieldDescriptor::TYPE_BOOL,
         shared_ptr<FromProtoTypeBase>(new FromProtoType<FieldDescriptor::TYPE_BOOL>))
        (FieldDescriptor::TYPE_STRING,
         shared_ptr<FromProtoTypeBase>(new FromProtoType<FieldDescriptor::TYPE_STRING>))
        (FieldDescriptor::TYPE_BYTES,
         shared_ptr<FromProtoTypeBase>(new FromProtoType<FieldDescriptor::TYPE_BYTES>))
        (FieldDescriptor::TYPE_MESSAGE,
         shared_ptr<FromProtoTypeBase>(new FromProtoType<FieldDescriptor::TYPE_MESSAGE>))
        (FieldDescriptor::TYPE_GROUP,
         shared_ptr<FromProtoTypeBase>(new FromProtoType<FieldDescriptor::TYPE_GROUP>))
        (FieldDescriptor::TYPE_ENUM,
         shared_ptr<FromProtoTypeBase>(new FromProtoType<FieldDescriptor::TYPE_ENUM>));


    boost::assign::insert (cpptype_map_)
        (static_cast<FieldDescriptor::CppType>(0),
         shared_ptr<FromProtoCppTypeBase>(new FromProtoCppTypeBase))
        (FieldDescriptor::CPPTYPE_DOUBLE,
         shared_ptr<FromProtoCppTypeBase>(new FromProtoCppType<FieldDescriptor::CPPTYPE_DOUBLE>))
        (FieldDescriptor::CPPTYPE_FLOAT,
         shared_ptr<FromProtoCppTypeBase>(new FromProtoCppType<FieldDescriptor::CPPTYPE_FLOAT>))
        (FieldDescriptor::CPPTYPE_UINT64,
         shared_ptr<FromProtoCppTypeBase>(new FromProtoCppType<FieldDescriptor::CPPTYPE_UINT64>))
        (FieldDescriptor::CPPTYPE_UINT32,
         shared_ptr<FromProtoCppTypeBase>(new FromProtoCppType<FieldDescriptor::CPPTYPE_UINT32>))
        (FieldDescriptor::CPPTYPE_INT32,
         shared_ptr<FromProtoCppTypeBase>(new FromProtoCppType<FieldDescriptor::CPPTYPE_INT32>))
        (FieldDescriptor::CPPTYPE_INT64,
         shared_ptr<FromProtoCppTypeBase>(new FromProtoCppType<FieldDescriptor::CPPTYPE_INT64>))
        (FieldDescriptor::CPPTYPE_BOOL,
         shared_ptr<FromProtoCppTypeBase>(new FromProtoCppType<FieldDescriptor::CPPTYPE_BOOL>))
        (FieldDescriptor::CPPTYPE_STRING,
         shared_ptr<FromProtoCppTypeBase>(new FromProtoCppType<FieldDescriptor::CPPTYPE_STRING>))
        (FieldDescriptor::CPPTYPE_MESSAGE,
         shared_ptr<FromProtoCppTypeBase>(new FromProtoCppType<FieldDescriptor::CPPTYPE_MESSAGE>))
        (FieldDescriptor::CPPTYPE_ENUM,
         shared_ptr<FromProtoCppTypeBase>(new FromProtoCppType<FieldDescriptor::CPPTYPE_ENUM>));

}

boost::shared_ptr<goby::acomms::FromProtoCppTypeBase> goby::acomms::TypeHelper::find(google::protobuf::FieldDescriptor::CppType cpptype, const std::string& type_name /*= ""*/)
{
    if(!type_name.empty())
    {
        CustomMessageMap::iterator it = custom_message_map_.find(type_name);
        if(it != custom_message_map_.end())
            return it->second;
    }
    
    CppTypeMap::iterator it = cpptype_map_.find(cpptype);
    if(it != cpptype_map_.end())
        return it->second;
    else
        return boost::shared_ptr<FromProtoCppTypeBase>();
}

// const boost::shared_ptr<goby::acomms::FromProtoCppTypeBase> goby::acomms::TypeHelper::find(google::protobuf::FieldDescriptor::CppType cpptype, const std::string& type_name /* = ""*/) const
// {
//     if(!type_name.empty())
//     {
//         CustomMessageMap::const_iterator it = custom_message_map_.find(type_name);
//         if(it != custom_message_map_.end())
//             return it->second;
//     }
    
//     CppTypeMap::const_iterator it = cpptype_map_.find(cpptype);
//     if(it != cpptype_map_.end())
//         return it->second;
//     else
//         return boost::shared_ptr<FromProtoCppTypeBase>();
// }



boost::shared_ptr<goby::acomms::FromProtoTypeBase> goby::acomms::TypeHelper::find(google::protobuf::FieldDescriptor::Type type)
{
    TypeMap::iterator it = type_map_.find(type);
    if(it != type_map_.end())
        return it->second;
    else
        return boost::shared_ptr<FromProtoTypeBase>();
}

// const boost::shared_ptr<goby::acomms::FromProtoTypeBase> goby::acomms::TypeHelper::find(google::protobuf::FieldDescriptor::Type type) const
// {
//     TypeMap::const_iterator it = type_map_.find(type);
//     if(it != type_map_.end())
//         return it->second;
//     else
//         return boost::shared_ptr<FromProtoTypeBase>();
// }


