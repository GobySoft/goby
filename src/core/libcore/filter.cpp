// copyright 2010 t. schneider tes@mit.edu
// 
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

#include <boost/cstdint.hpp>

#include "goby/util/string.h"

#include "filter.h"

bool goby::core::clears_filter(const google::protobuf::Message& msg, const goby::core::protobuf::Filter& filter)
{
    using goby::util::as;
    
    // no filter
    if(!filter.IsInitialized()) return true;
    
    // application base already checked for filter problems
    using namespace google::protobuf;
    // check filter for exclusions
    const Descriptor* descriptor = msg.GetDescriptor();
    const Reflection* reflection = msg.GetReflection();

    // field that this filter acts on
    const FieldDescriptor* field_descriptor = descriptor->FindFieldByName(filter.key());
    if(!field_descriptor) return false;

    switch(field_descriptor->cpp_type())
    {
        default:
        case google::protobuf::FieldDescriptor::CPPTYPE_ENUM:
        case google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE:
            return false;
            
        case google::protobuf::FieldDescriptor::CPPTYPE_INT32:
            return filter_comp(as<google::protobuf::int32>(filter.value()),
                               reflection->GetInt32(msg, field_descriptor),
                               filter.operation());
            
        case google::protobuf::FieldDescriptor::CPPTYPE_INT64:
            return filter_comp(as<google::protobuf::int64>(filter.value()),
                               reflection->GetInt64(msg, field_descriptor),
                               filter.operation());

        case google::protobuf::FieldDescriptor::CPPTYPE_UINT32:
            return filter_comp(as<google::protobuf::uint32>(filter.value()),
                               reflection->GetUInt32(msg, field_descriptor),
                               filter.operation());

        case google::protobuf::FieldDescriptor::CPPTYPE_UINT64:
            return filter_comp(as<google::protobuf::uint64>(filter.value()),
                               reflection->GetUInt64(msg, field_descriptor),
                               filter.operation());

        case google::protobuf::FieldDescriptor::CPPTYPE_BOOL:
            return filter_comp(as<bool>(filter.value()),
                               reflection->GetBool(msg, field_descriptor),
                               filter.operation());

        case google::protobuf::FieldDescriptor::CPPTYPE_STRING:
            return filter_comp(filter.value(),
                               reflection->GetString(msg, field_descriptor),
                               filter.operation());

        case google::protobuf::FieldDescriptor::CPPTYPE_FLOAT:
            return filter_comp(as<float>(filter.value()),
                               reflection->GetFloat(msg, field_descriptor),
                               filter.operation());

        case google::protobuf::FieldDescriptor::CPPTYPE_DOUBLE:
            return filter_comp(as<double>(filter.value()),
                               reflection->GetDouble(msg, field_descriptor),
                               filter.operation());

    }
}
