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

#include <boost/algorithm/string.hpp>

#include <google/protobuf/message.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/dynamic_message.h>
#include <google/protobuf/text_format.h>

#include <Wt/Dbo/Dbo>
#include <Wt/Dbo/backend/Sqlite3>


namespace Wt
{
    namespace Dbo
    {
        /// \brief allows us to "persist" a Google Protocol Buffers message in an SQL
        /// database handled by Wt::Dbo.
        ///
        /// We "trick" Wt::Dbo into thinking we have a
        /// compile time generated object (here, ProtoBufWrapper<i>) that is actually
        /// runtime assigned and examined google::protobuf::Message. We can do this
        /// because google::protobuf::Messages support runtime reflection
        template <typename T, typename A>
            void protobuf_message_persist(T& obj, A& action, const std::string& prefix = "")
        {            
            const google::protobuf::Descriptor* desc = obj.GetDescriptor();
            const google::protobuf::Reflection* refl = obj.GetReflection();       
     
            for(int i = 0, n = desc->field_count(); i < n; ++i)
            {
                const google::protobuf::FieldDescriptor* field_desc = desc->field(i);
                const std::string field_name = prefix + field_desc->name();

                // use TextFormat to handle repeated fields
                if(field_desc->is_repeated())
                {
                    google::protobuf::TextFormat::Printer printer;
                    printer.SetSingleLineMode(true);

                    std::string aggregate;
                    std::string item;
                    
                    // print the protobuf object out to a string `aggregate` (newline delimited)
                    for(int j = 0, o = refl->FieldSize(obj, field_desc); j < o; ++j)
                    {
                        printer.PrintFieldValueToString(obj, field_desc, j, &item);
                        if(j) aggregate += "\n";
                        aggregate += item;
                    }
                    
                    Wt::Dbo::field(action, aggregate, field_name);

                    // read the (modified) string `aggregate` back into the protobuf object
                    refl->ClearField(&obj, field_desc);
                    typedef boost::split_iterator<std::string::iterator> string_split_iterator;
                    for(string_split_iterator it =
                            make_split_iterator(aggregate,
                                                boost::first_finder("\n", boost::is_iequal()));
                        it!=string_split_iterator();
                        ++it)
                    {
                        if(field_desc->cpp_type() ==
                           google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE)
                        {
                            google::protobuf::TextFormat::ParseFromString(
                                boost::copy_range<std::string>(*it),
                                refl->AddMessage(&obj, field_desc));
                        }
                        else
                        {
                            google::protobuf::TextFormat::ParseFieldValueFromString(
                                boost::copy_range<std::string>(*it),
                                field_desc,
                                &obj);
                        }
                    }
                }
                else // use native fields
                {
                    
                    switch(field_desc->cpp_type())
                    {
                        case google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE:
                        {
                            protobuf_message_persist(*refl->MutableMessage(&obj, field_desc),
                                                     action,
                                                     std::string(field_name + "_"));
                        }
                        break;    
                    
                        // int
                        case google::protobuf::FieldDescriptor::CPPTYPE_INT32:
                        {
                            int tmp = refl->GetInt32(obj, field_desc);
                            Wt::Dbo::field(action, tmp, field_name);
                            refl->SetInt32(&obj, field_desc, tmp);
                        }
                        break;
                    
                        // long long (should be long, but not currently implemented by Wt::Dbo)
                        case google::protobuf::FieldDescriptor::CPPTYPE_INT64:
                        {
                            long long tmp = refl->GetInt64(obj, field_desc);
                            Wt::Dbo::field(action, tmp, field_name);
                            refl->SetInt64(&obj, field_desc, tmp);
                        }
                        break;

                        case google::protobuf::FieldDescriptor::CPPTYPE_UINT32:
                        {
                            long long tmp = refl->GetUInt32(obj, field_desc);
                            Wt::Dbo::field(action, tmp, field_name);
                            refl->SetUInt32(&obj, field_desc, tmp);
                        }
                        break;

                        // long long
                        case google::protobuf::FieldDescriptor::CPPTYPE_UINT64:
                        {
                            long long tmp = refl->GetUInt64(obj, field_desc);
                            Wt::Dbo::field(action, tmp, field_name);
                            refl->SetUInt64(&obj, field_desc, tmp);
                        }
                        break;
                        
                        // short    
                        case google::protobuf::FieldDescriptor::CPPTYPE_BOOL:
                        {
                            short tmp = refl->GetBool(obj, field_desc);
                            Wt::Dbo::field(action, tmp, field_name);
                            refl->SetBool(&obj, field_desc, tmp);
                        }
                        break;
                    
                        case google::protobuf::FieldDescriptor::CPPTYPE_STRING:
                        {
                            std::string tmp = refl->GetString(obj, field_desc);
                            Wt::Dbo::field(action, tmp, field_name);
                            refl->SetString(&obj, field_desc, tmp);
                        }
                        break;                    

                        case google::protobuf::FieldDescriptor::CPPTYPE_FLOAT:
                        {
                            float tmp = refl->GetFloat(obj, field_desc);
                            Wt::Dbo::field(action, tmp, field_name);
                            refl->SetFloat(&obj, field_desc, tmp);
                        }                        
                        break;
                        
                        case google::protobuf::FieldDescriptor::CPPTYPE_DOUBLE:
                        {
                            // need boost::optional to handle NaN (as NULL)
                            boost::optional<double> tmp(refl->GetDouble(obj, field_desc));
                            Wt::Dbo::field(action, tmp, field_name);
                            refl->SetDouble(&obj, field_desc, tmp.get());
                        }
                        break;

                        case google::protobuf::FieldDescriptor::CPPTYPE_ENUM:
                        {
                            const google::protobuf::EnumValueDescriptor* enum_value =
                                refl->GetEnum(obj, field_desc);
                            std::string tmp = enum_value->name();
                            Wt::Dbo::field(action, tmp, field_name);
                            refl->SetEnum(&obj, field_desc, enum_value->type()->FindValueByName(tmp));
                            break;
                        }
                    }
                }
            }
        }
    

        // Tells Wt::Dbo use this overload of `persist` if the object
        // is derived from google::protobuf::Message
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
