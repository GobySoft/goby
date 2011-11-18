// copyright 2011 t. schneider tes@mit.edu
// 
// this file is part of goby-acomms, a collection of libraries for acoustic underwater networking
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

#ifndef MOOSPROTOBUFHELPERS20110216H
#define MOOSPROTOBUFHELPERS20110216H

#include "google/protobuf/io/printer.h"
#include <google/protobuf/io/tokenizer.h>
#include "goby/util/logger/flex_ostream.h"
#include "goby/util/as.h"
#include "goby/util/binary.h"
#include "goby/moos/moos_string.h"


/// \file moos_protobuf_helpers.h Helpers for MOOS applications for serializing and parsed Google Protocol buffers messages

/// \brief Converts the Google Protocol Buffers message `msg` into a suitable (human readable) string `out` for sending via MOOS
///
/// \param out pointer to std::string to store serialized result
/// \param msg Google Protocol buffers message to serialize
inline void serialize_for_moos(std::string* out, const google::protobuf::Message& msg)
{
    google::protobuf::TextFormat::Printer printer;
    printer.SetSingleLineMode(true);
    printer.PrintToString(msg, out);
}


/// \brief Parses the string `in` to Google Protocol Buffers message `msg`. All errors are written to the goby::util::glogger().
///
/// \param in std::string to parse
/// \param msg Google Protocol buffers message to store result
inline void parse_for_moos(const std::string& in, google::protobuf::Message* msg)
{
    google::protobuf::TextFormat::Parser parser;
    FlexOStreamErrorCollector error_collector(in);
    parser.RecordErrorsTo(&error_collector);
    parser.ParseFromString(in, msg);
}

inline void from_moos_comma_equals_string_field(google::protobuf::Message* proto_msg, const google::protobuf::FieldDescriptor* field_desc, const std::vector<std::string>& values, int value_key = 0)
{
    if(values.size() == 0)
        return;
    
    
    const google::protobuf::Reflection* refl = proto_msg->GetReflection();
    if(field_desc->is_repeated())
    {
        for(int j = 0, m = values.size(); j < m; ++j)
        {
            const std::string& v = values[j];

            switch(field_desc->cpp_type())
            {
                case google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE:
                    refl->AddMessage(proto_msg, field_desc)->ParseFromString(goby::util::hex_decode(v));
                    break;    
                        
                case google::protobuf::FieldDescriptor::CPPTYPE_INT32:
                    refl->AddInt32(proto_msg, field_desc, goby::util::as<google::protobuf::int32>(v));
                    break;
                            
                case google::protobuf::FieldDescriptor::CPPTYPE_INT64:
                    refl->AddInt64(proto_msg, field_desc, goby::util::as<google::protobuf::int64>(v));
                    break;
                            
                case google::protobuf::FieldDescriptor::CPPTYPE_UINT32:
                    refl->AddUInt32(proto_msg, field_desc, goby::util::as<google::protobuf::uint32>(v));
                    break;
                            
                case google::protobuf::FieldDescriptor::CPPTYPE_UINT64:
                    refl->AddUInt64(proto_msg, field_desc, goby::util::as<google::protobuf::uint64>(v));
                    break;
                            
                case google::protobuf::FieldDescriptor::CPPTYPE_BOOL:
                    refl->AddBool(proto_msg, field_desc, goby::util::as<bool>(v));
                    break;
                            
                case google::protobuf::FieldDescriptor::CPPTYPE_STRING:
                    if(field_desc->type() ==  google::protobuf::FieldDescriptor::TYPE_STRING)
                        refl->AddString(proto_msg, field_desc, v);
                    else if(field_desc->type() ==  google::protobuf::FieldDescriptor::TYPE_BYTES)
                        refl->AddString(proto_msg, field_desc, goby::util::hex_decode(v));
                    break;                    
                            
                case google::protobuf::FieldDescriptor::CPPTYPE_FLOAT:
                    refl->AddFloat(proto_msg, field_desc, goby::util::as<float>(v));
                    break;
                            
                case google::protobuf::FieldDescriptor::CPPTYPE_DOUBLE:
                    refl->AddDouble(proto_msg, field_desc, goby::util::as<double>(v));
                    break;
                            
                case google::protobuf::FieldDescriptor::CPPTYPE_ENUM:
                {
                    const google::protobuf::EnumValueDescriptor* enum_desc =
                        field_desc->enum_type()->FindValueByName(v);
                    if(enum_desc)
                        refl->AddEnum(proto_msg, field_desc, enum_desc);
                }
                break;
                
            }                
        }
    }
    else
    {
        const std::string& v = values[value_key];
        switch(field_desc->cpp_type())
        {
            case google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE:
                refl->MutableMessage(proto_msg, field_desc)->ParseFromString(goby::util::hex_decode(v));
                break;
                        
            case google::protobuf::FieldDescriptor::CPPTYPE_INT32:
                refl->SetInt32(proto_msg, field_desc, goby::util::as<google::protobuf::int32>(v));
                break;
                        
            case google::protobuf::FieldDescriptor::CPPTYPE_INT64:
                refl->SetInt64(proto_msg, field_desc, goby::util::as<google::protobuf::int64>(v));                        
                break;

            case google::protobuf::FieldDescriptor::CPPTYPE_UINT32:
                refl->SetUInt32(proto_msg, field_desc, goby::util::as<google::protobuf::uint32>(v));
                break;

            case google::protobuf::FieldDescriptor::CPPTYPE_UINT64:
                refl->SetUInt64(proto_msg, field_desc, goby::util::as<google::protobuf::uint64>(v));
                break;
                        
            case google::protobuf::FieldDescriptor::CPPTYPE_BOOL:
                refl->SetBool(proto_msg, field_desc, goby::util::as<bool>(v));
                break;
                    
            case google::protobuf::FieldDescriptor::CPPTYPE_STRING:
                if(field_desc->type() ==  google::protobuf::FieldDescriptor::TYPE_STRING)
                    refl->SetString(proto_msg, field_desc, v);
                else if(field_desc->type() ==  google::protobuf::FieldDescriptor::TYPE_BYTES)
                    refl->SetString(proto_msg, field_desc, goby::util::hex_decode(v));
                break;                    
                
            case google::protobuf::FieldDescriptor::CPPTYPE_FLOAT:
                refl->SetFloat(proto_msg, field_desc, goby::util::as<float>(v));
                break;
                
            case google::protobuf::FieldDescriptor::CPPTYPE_DOUBLE:
                refl->SetDouble(proto_msg, field_desc, goby::util::as<double>(v));
                break;
                
            case google::protobuf::FieldDescriptor::CPPTYPE_ENUM:
            {
                const google::protobuf::EnumValueDescriptor* enum_desc =
                    field_desc->enum_type()->FindValueByName(v);
                if(enum_desc)
                    refl->SetEnum(proto_msg, field_desc, enum_desc);
            }
            break;
        }
    }
}

inline void from_moos_comma_equals_string(google::protobuf::Message* proto_msg, const std::string& in)
{
    const google::protobuf::Descriptor* desc = proto_msg->GetDescriptor();
    const google::protobuf::Reflection* refl = proto_msg->GetReflection();

    
    for(int i = 0, n = desc->field_count(); i < n; ++i)
    {
        const google::protobuf::FieldDescriptor* field_desc = desc->field(i);

        switch(field_desc->cpp_type())
        {
            default:
            {
                std::string val;
                if(goby::moos::val_from_string(val, in, field_desc->name()))
                {
                    std::vector<std::string> vals;
                    boost::split(vals, val, boost::is_any_of(","));
                    from_moos_comma_equals_string_field(proto_msg, field_desc, vals);
                }
            }
            break;

            case google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE:
                if(field_desc->is_repeated())
                {
                    for(int k = 0, o = field_desc->message_type()->field_count(); k < o; ++k)
                    {
                        std::string val;
                        if(goby::moos::val_from_string(val, in, field_desc->name() + "_" + field_desc->message_type()->field(k)->name()))
                        {
                            std::vector<std::string> vals;
                            boost::split(vals, val, boost::is_any_of(","));
                            
                            for(int j = 0, m = vals.size(); j < m; ++j)
                            {

                                google::protobuf::Message* embedded_msg =
                                    (refl->FieldSize(*proto_msg, field_desc) < j+1) ?
                                    refl->AddMessage(proto_msg, field_desc) :
                                    refl->MutableRepeatedMessage(proto_msg, field_desc, j);     
                                from_moos_comma_equals_string_field(embedded_msg, embedded_msg->GetDescriptor()->field(k), vals, j);
                            }
                        }
                    }   
                }
                else
                {
                    for(int k = 0, o = field_desc->message_type()->field_count(); k < o; ++ k)
                    {
                        std::string val;
                        if(goby::moos::val_from_string(val, in, field_desc->name() + "_" + field_desc->message_type()->field(k)->name()))
                        {
                            std::vector<std::string> vals;
                            boost::split(vals, val, boost::is_any_of(","));

                            google::protobuf::Message* embedded_msg = refl->MutableMessage(proto_msg, field_desc);     
                            from_moos_comma_equals_string_field(embedded_msg, embedded_msg->GetDescriptor()->field(k), vals);
                        }
                    }
                }
                break;
        }
    }    
}

inline std::string to_moos_comma_equals_string_field(const google::protobuf::Message& proto_msg, const google::protobuf::FieldDescriptor* field_desc, bool write_key = true)
{
    const google::protobuf::Reflection* refl = proto_msg.GetReflection();

    std::stringstream out;
    const std::string& field_name = field_desc->name();
        
    if(field_desc->is_repeated())
    {
        if(write_key)
            out << field_name << "={";

        for(int j = 0, m = refl->FieldSize(proto_msg, field_desc); j < m; ++j)
        {
            if(j) out << ",";
            
            switch(field_desc->cpp_type())
            {
                case google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE:
                    out << goby::util::hex_encode(refl->GetRepeatedMessage(proto_msg, field_desc, j).SerializeAsString());
                        break;    
                        
                case google::protobuf::FieldDescriptor::CPPTYPE_INT32:
                    out << refl->GetRepeatedInt32(proto_msg, field_desc, j);
                    break;
                            
                case google::protobuf::FieldDescriptor::CPPTYPE_INT64:
                    out << refl->GetRepeatedInt64(proto_msg, field_desc, j);
                    break;
                            
                case google::protobuf::FieldDescriptor::CPPTYPE_UINT32:
                    out << refl->GetRepeatedUInt32(proto_msg, field_desc, j);
                    break;
                            
                case google::protobuf::FieldDescriptor::CPPTYPE_UINT64:
                    out << refl->GetRepeatedUInt64(proto_msg, field_desc, j);
                    break;
                            
                case google::protobuf::FieldDescriptor::CPPTYPE_BOOL:
                    out << std::boolalpha << refl->GetRepeatedBool(proto_msg, field_desc, j);
                    break;
                            
                case google::protobuf::FieldDescriptor::CPPTYPE_STRING:
                    if(field_desc->type() ==  google::protobuf::FieldDescriptor::TYPE_STRING)
                        out << refl->GetRepeatedString(proto_msg, field_desc, j);
                    else if(field_desc->type() ==  google::protobuf::FieldDescriptor::TYPE_BYTES)
                        out << goby::util::hex_encode(refl->GetRepeatedString(proto_msg, field_desc, j));    
                    break;                    
                            
                case google::protobuf::FieldDescriptor::CPPTYPE_FLOAT:
                    out << refl->GetRepeatedFloat(proto_msg, field_desc, j);
                    break;
                            
                case google::protobuf::FieldDescriptor::CPPTYPE_DOUBLE:
                    out << std::setprecision(15) << refl->GetRepeatedDouble(proto_msg, field_desc, j);
                    break;
                            
                case google::protobuf::FieldDescriptor::CPPTYPE_ENUM:
                    out << refl->GetRepeatedEnum(proto_msg, field_desc, j)->name();
                    break;
                            
            }
                
        }
        if(write_key)
            out << "}";
    }
    else
    {
        if(write_key)
            out << field_name << "=";
        switch(field_desc->cpp_type())
        {
            case google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE:
                out << goby::util::hex_encode(refl->GetMessage(proto_msg, field_desc).SerializeAsString());
                break;
                        
            case google::protobuf::FieldDescriptor::CPPTYPE_INT32:
                out << refl->GetInt32(proto_msg, field_desc);
                break;
                        
            case google::protobuf::FieldDescriptor::CPPTYPE_INT64:
                out << refl->GetInt64(proto_msg, field_desc);                        
                break;

            case google::protobuf::FieldDescriptor::CPPTYPE_UINT32:
                out << refl->GetUInt32(proto_msg, field_desc);
                break;

            case google::protobuf::FieldDescriptor::CPPTYPE_UINT64:
                out << refl->GetUInt64(proto_msg, field_desc);
                break;
                        
            case google::protobuf::FieldDescriptor::CPPTYPE_BOOL:
                out << std::boolalpha << refl->GetBool(proto_msg, field_desc);
                break;
                    
            case google::protobuf::FieldDescriptor::CPPTYPE_STRING:
                if(field_desc->type() ==  google::protobuf::FieldDescriptor::TYPE_STRING)
                    out << refl->GetString(proto_msg, field_desc);
                else if(field_desc->type() ==  google::protobuf::FieldDescriptor::TYPE_BYTES)
                    out << goby::util::hex_encode(refl->GetString(proto_msg, field_desc));
                break;                    
                
            case google::protobuf::FieldDescriptor::CPPTYPE_FLOAT:
                out << refl->GetFloat(proto_msg, field_desc);
                break;
                
            case google::protobuf::FieldDescriptor::CPPTYPE_DOUBLE:
                out << std::setprecision(15) << refl->GetDouble(proto_msg, field_desc);
                break;
                
            case google::protobuf::FieldDescriptor::CPPTYPE_ENUM:
                out << refl->GetEnum(proto_msg, field_desc)->name();
                break;
        }
    }
    return out.str();
}

inline void to_moos_comma_equals_string(const google::protobuf::Message& proto_msg, std::string* out)
{
    std::stringstream out_ss;
    const google::protobuf::Descriptor* desc = proto_msg.GetDescriptor();
    const google::protobuf::Reflection* refl = proto_msg.GetReflection();
    
    for(int i = 0, n = desc->field_count(); i < n; ++i)
    {
        const google::protobuf::FieldDescriptor* field_desc = desc->field(i);
        if(i) out_ss << ",";

        const std::string& field_name = field_desc->name();

        switch(field_desc->cpp_type())
        {
            default:
                out_ss << to_moos_comma_equals_string_field(proto_msg, field_desc);
                break;

            case google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE:
                if(field_desc->is_repeated())
                {
                    for(int k = 0, o = field_desc->message_type()->field_count(); k < o; ++ k)
                    {
                        if(k) out_ss << ",";
                        out_ss << field_name << "_" << field_desc->message_type()->field(k)->name() << "={";
                        for(int j = 0, m = refl->FieldSize(proto_msg, field_desc); j < m; ++j)
                        {
                            if(j) out_ss << ",";
                            const google::protobuf::Message& embedded_msg = refl->GetRepeatedMessage(proto_msg, field_desc, j);
                            out_ss << to_moos_comma_equals_string_field(embedded_msg, embedded_msg.GetDescriptor()->field(k), false);
                        }
                        out_ss << "}";
                    }
                }
                
                else
                {
                    for(int k = 0, o = field_desc->message_type()->field_count(); k < o; ++ k)
                    {
                        if(k) out_ss << ",";
                        out_ss << field_name << "_" << field_desc->message_type()->field(k)->name() << "=";
                        const google::protobuf::Message& embedded_msg = refl->GetMessage(proto_msg, field_desc);
                        out_ss << to_moos_comma_equals_string_field(embedded_msg, embedded_msg.GetDescriptor()->field(k), false);
                    }
                }
                
                break;
        }
    }    
    *out = out_ss.str();
}



#endif
