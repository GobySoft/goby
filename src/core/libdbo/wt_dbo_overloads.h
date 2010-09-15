// copyright 2010 t. schneider tes@mit.edu
// 
// this file is part of goby_dbo
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

#ifndef WTDBOOVERLOADS20100901H
#define WTDBOOVERLOADS20100901H

#include <iostream>

#include <google/protobuf/message.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/dynamic_message.h>

#include <Wt/Dbo/Dbo>
#include <Wt/Dbo/backend/Sqlite3>

namespace Wt
{
    namespace Dbo
    {
        template <typename T, typename A>
        void protobuf_message_persist(T& obj, A& action)
        {
            const google::protobuf::Descriptor* desc = obj.GetDescriptor();
            const google::protobuf::Reflection* refl = obj.GetReflection();
            
            for(int i = 0, n = desc->field_count(); i < n; ++i)
            {
                const google::protobuf::FieldDescriptor* field_desc = desc->field(i);

                
                if(field_desc->is_repeated())
                    throw(std::runtime_error(std::string("repeated messages are not currently supported:" +  field_desc->DebugString())));
                
                switch(field_desc->cpp_type())
                {
                    case google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE:
                        throw(std::runtime_error(std::string("nested messages are not currently supported:" +  field_desc->DebugString())));
                        break;    
                        
                    // int
                    case google::protobuf::FieldDescriptor::CPPTYPE_INT32:
                    {
                        int tmp = refl->GetInt32(obj, field_desc);
                        Wt::Dbo::field(action, tmp, field_desc->name());
                        refl->SetInt32(&obj, field_desc, tmp);
                    }
                    break;
                        
                    // long long (should be long, but not currently implemented by Wt::Dbo)
                    case google::protobuf::FieldDescriptor::CPPTYPE_INT64:
                    {
                        long long tmp = refl->GetInt64(obj, field_desc);
                        Wt::Dbo::field(action, tmp, field_desc->name());
                        refl->SetInt64(&obj, field_desc, tmp);
                    }
                    break;

                    case google::protobuf::FieldDescriptor::CPPTYPE_UINT32:
                    {
                        long long tmp = refl->GetUInt32(obj, field_desc);
                        Wt::Dbo::field(action, tmp, field_desc->name());
                        refl->SetUInt32(&obj, field_desc, tmp);
                    }
                    break;

                    // long long
                    case google::protobuf::FieldDescriptor::CPPTYPE_UINT64:
                    {
                        long long tmp = refl->GetUInt64(obj, field_desc);
                        Wt::Dbo::field(action, tmp, field_desc->name());
                        refl->SetInt64(&obj, field_desc, tmp);
                    }
                    break;
                        
                        // short    
                    case google::protobuf::FieldDescriptor::CPPTYPE_BOOL:
                    {
                        short tmp = refl->GetBool(obj, field_desc);
                        Wt::Dbo::field(action, tmp, field_desc->name());
                        refl->SetBool(&obj, field_desc, tmp);
                    }
                    break;
                    
                    case google::protobuf::FieldDescriptor::CPPTYPE_STRING:
                    {
                        std::string tmp = refl->GetString(obj, field_desc);
                        Wt::Dbo::field(action, tmp, field_desc->name());
                        refl->SetString(&obj, field_desc, tmp);
                    }                        
                    break;                    

                    case google::protobuf::FieldDescriptor::CPPTYPE_FLOAT:
                    {
                        float tmp = refl->GetFloat(obj, field_desc);
                        Wt::Dbo::field(action, tmp, field_desc->name());
                        refl->SetFloat(&obj, field_desc, tmp);
                    }                        
                    break;
                        
                    case google::protobuf::FieldDescriptor::CPPTYPE_DOUBLE:
                    {
                        double tmp = refl->GetDouble(obj, field_desc);
                        Wt::Dbo::field(action, tmp, field_desc->name());
                        refl->SetDouble(&obj, field_desc, tmp);
                    }
                    break;

                    case google::protobuf::FieldDescriptor::CPPTYPE_ENUM:
                    default:
                        throw(std::runtime_error(std::string("no mapping from Google Protocol Buffers field type: " + field_desc->DebugString() + " to SQL Wt::Dbo type")));
                        break;

                }
            }
        }
    

        // use this overload of `persist` if the object is derived from google::protobuf::Message
        template <typename C>
        struct persist<C, typename boost::enable_if<boost::is_base_of<google::protobuf::Message, C> >::type>
        {
            template<typename A>
            static void apply(C& obj, A& action)
                { protobuf_message_persist(obj, action); }
        };
        
    }
}



#endif
