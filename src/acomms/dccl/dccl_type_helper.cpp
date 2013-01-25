// Copyright 2009-2013 Toby Schneider (https://launchpad.net/~tes)
//                     Massachusetts Institute of Technology (2007-)
//                     Woods Hole Oceanographic Institution (2007-)
//                     Goby Developers Team (https://launchpad.net/~goby-dev)
// 
//
// This file is part of the Goby Underwater Autonomy Project Libraries
// ("The Goby Libraries").
//
// The Goby Libraries are free software: you can redistribute them and/or modify
// them under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// The Goby Libraries are distributed in the hope that they will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with Goby.  If not, see <http://www.gnu.org/licenses/>.


#include "dccl_type_helper.h"

#include <boost/assign.hpp>

goby::acomms::DCCLTypeHelper::TypeMap goby::acomms::DCCLTypeHelper::type_map_;
goby::acomms::DCCLTypeHelper::CppTypeMap goby::acomms::DCCLTypeHelper::cpptype_map_;
goby::acomms::DCCLTypeHelper::CustomMessageMap goby::acomms::DCCLTypeHelper::custom_message_map_;

// used to construct, initialize, and delete a copy of this object
boost::shared_ptr<goby::acomms::DCCLTypeHelper> goby::acomms::DCCLTypeHelper::inst_(new goby::acomms::DCCLTypeHelper);

//
// DCCLTypeHelper
//

void goby::acomms::DCCLTypeHelper::initialize()
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

boost::shared_ptr<goby::acomms::FromProtoCppTypeBase> goby::acomms::DCCLTypeHelper::find(google::protobuf::FieldDescriptor::CppType cpptype, const std::string& type_name /*= ""*/)
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


boost::shared_ptr<goby::acomms::FromProtoTypeBase> goby::acomms::DCCLTypeHelper::find(google::protobuf::FieldDescriptor::Type type)
{
    TypeMap::iterator it = type_map_.find(type);
    if(it != type_map_.end())
        return it->second;
    else
        return boost::shared_ptr<FromProtoTypeBase>();
}

