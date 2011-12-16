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

#include <boost/format.hpp>

#include <google/protobuf/io/printer.h>
#include <google/protobuf/io/tokenizer.h>

#include "goby/util/logger/flex_ostream.h"
#include "goby/util/as.h"
#include "goby/util/binary.h"
#include "goby/moos/moos_string.h"

#include "goby/moos/transitional/message_algorithms.h"
#include "goby/moos/transitional/message_val.h"

#include "goby/moos/protobuf/translator.pb.h"

namespace goby
{
    namespace moos
    {
        template<goby::moos::protobuf::TranslatorEntry::ParserSerializerTechnique technique>
            class MOOSTranslation
        {};
        

        template<>
            class MOOSTranslation <protobuf::TranslatorEntry::TECHNIQUE_PROTOBUF_TEXT_FORMAT>
        {
          public:
            static void serialize(std::string* out, const google::protobuf::Message& in)
            {
                google::protobuf::TextFormat::Printer printer;
                printer.SetSingleLineMode(true);
                printer.PrintToString(in, out);
            }
            
            static void parse(const std::string& in, google::protobuf::Message* out)
            {
                google::protobuf::TextFormat::Parser parser;
                FlexOStreamErrorCollector error_collector(in);
                parser.RecordErrorsTo(&error_collector);
                parser.ParseFromString(in, out);
            }
        };
        

        template<>
            class MOOSTranslation <protobuf::TranslatorEntry::TECHNIQUE_PROTOBUF_NATIVE_ENCODED>
        {
          public:
            static void serialize(std::string* out, const google::protobuf::Message& in)
            {
                in.SerializeToString(out);
            }
            
            static void parse(const std::string& in, google::protobuf::Message* out)
            {
                out->ParseFromString(in);
            }
        };

        template<>
            class MOOSTranslation <protobuf::TranslatorEntry::TECHNIQUE_COMMA_SEPARATED_KEY_EQUALS_VALUE_PAIRS>
        {
          public:
            static void serialize(std::string* out, const google::protobuf::Message& in)
            {
                std::stringstream out_ss;
                
                const google::protobuf::Descriptor* desc = in.GetDescriptor();
                const google::protobuf::Reflection* refl = in.GetReflection();
    
                for(int i = 0, n = desc->field_count(); i < n; ++i)
                {
                    const google::protobuf::FieldDescriptor* field_desc = desc->field(i);
                    if(i) out_ss << ",";
                    
                    const std::string& field_name = field_desc->name();

                    switch(field_desc->cpp_type())
                    {
                        default:
                            out_ss << to_moos_comma_equals_string_field(in, field_desc);
                            break;

                        case google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE:
                            if(field_desc->is_repeated())
                            {
                                for(int k = 0, o = field_desc->message_type()->field_count(); k < o; ++ k)
                                {
                                    if(k) out_ss << ",";
                                    out_ss << field_name << "_" << field_desc->message_type()->field(k)->name() << "={";
                                    for(int j = 0, m = refl->FieldSize(in, field_desc); j < m; ++j)
                                    {
                                        if(j) out_ss << ",";
                                        const google::protobuf::Message& embedded_msg = refl->GetRepeatedMessage(in, field_desc, j);
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
                                    const google::protobuf::Message& embedded_msg = refl->GetMessage(in, field_desc);
                                    out_ss << to_moos_comma_equals_string_field(embedded_msg, embedded_msg.GetDescriptor()->field(k), false);
                                }
                            }
                
                            break;
                    }
                }    
                *out = out_ss.str();

            }
            
            static void parse(const std::string& in, google::protobuf::Message* out)
            {
                const google::protobuf::Descriptor* desc = out->GetDescriptor();
                const google::protobuf::Reflection* refl = out->GetReflection();
                
                
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
                                from_moos_comma_equals_string_field(out, field_desc, vals);
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
                                                (refl->FieldSize(*out, field_desc) < j+1) ?
                                                refl->AddMessage(out, field_desc) :
                                                refl->MutableRepeatedMessage(out, field_desc, j);     
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

                                        google::protobuf::Message* embedded_msg = refl->MutableMessage(out, field_desc);     
                                        from_moos_comma_equals_string_field(embedded_msg, embedded_msg->GetDescriptor()->field(k), vals);
                                    }
                                }
                            }
                            break;
                    }
                }    
            }

          private:
            static std::string to_moos_comma_equals_string_field(const google::protobuf::Message& proto_msg, const google::protobuf::FieldDescriptor* field_desc, bool write_key = true)
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
            static void from_moos_comma_equals_string_field(google::protobuf::Message* proto_msg, const google::protobuf::FieldDescriptor* field_desc, const std::vector<std::string>& values, int value_key = 0)
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


        };

        template<>
            class MOOSTranslation <protobuf::TranslatorEntry::TECHNIQUE_FORMAT>
        {
          public:
            static void serialize(std::string* out, const google::protobuf::Message& in,
                                  const google::protobuf::RepeatedPtrField<protobuf::TranslatorEntry::PublishSerializer::Algorithm>& algorithms,
                                  const std::string& format,
                                  const std::string& repeated_delimiter)
            {

                const google::protobuf::Descriptor* desc = in.GetDescriptor();
                const google::protobuf::Reflection* refl = in.GetReflection();


                
                int max_field_number = 1;
                for(int i = 1, n = desc->field_count(); i < n; ++i)
                {
                    const google::protobuf::FieldDescriptor* field_desc = desc->field(i);
                    if(field_desc->number() > max_field_number)
                        max_field_number = field_desc->number();
                }
                

                // run algorithms
                typedef google::protobuf::RepeatedPtrField<protobuf::TranslatorEntry::PublishSerializer::Algorithm>::const_iterator const_iterator;

                std::map<int, std::string> modified_values;
                
                for(const_iterator it = algorithms.begin(), n = algorithms.end();
                    it != n; ++it)
                {
                    const google::protobuf::FieldDescriptor* primary_field_desc =
                        desc->FindFieldByNumber(it->primary_field());

                    if(!primary_field_desc || primary_field_desc->is_repeated())
                        continue;
                    
                    std::string primary_val;
                    google::protobuf::TextFormat::PrintFieldValueToString(in, primary_field_desc, -1, &primary_val);
                    
                    goby::transitional::DCCLMessageVal val(primary_val);
                    std::vector<goby::transitional::DCCLMessageVal> ref;
                    for(int i = 0, m = it->reference_field_size(); i<m; ++i)
                    {
                        const google::protobuf::FieldDescriptor* field_desc =
                            desc->FindFieldByNumber(it->reference_field(i));

                        if(field_desc && !field_desc->is_repeated())
                        {
                            std::string ref_value;
                            google::protobuf::TextFormat::PrintFieldValueToString(in, field_desc, -1, &ref_value);    
                            ref.push_back(ref_value);
                        }
                        else
                        {
                            throw(std::runtime_error("Reference field given is invalid or repeated (must be optional or required): " + goby::util::as<std::string>(it->reference_field(i))));
                        }
                        
                        
                        transitional::DCCLAlgorithmPerformer::getInstance()->
                            run_algorithm(it->name(), val, ref);
                        
                        val = std::string(val);
                    }
                    modified_values.insert(std::make_pair(it->output_virtual_field(), val));
                    if(it->output_virtual_field() > max_field_number)
                        max_field_number = it->output_virtual_field();
                }

                boost::format out_format(format);
                out_format.exceptions( boost::io::all_error_bits ^ ( boost::io::too_many_args_bit | boost::io::too_few_args_bit )); 

                for(int i = 1; i <= max_field_number; ++i)
                {
                    const google::protobuf::FieldDescriptor* field_desc = desc->FindFieldByNumber(i);
                    std::map<int, std::string>::const_iterator mod_it = modified_values.find(i);
                    if(field_desc)
                    {                    
                        if(field_desc->is_repeated())
                        {
                            std::stringstream out_repeated;
                            for(int j = 0, m = refl->FieldSize(in, field_desc); j < m; ++j)
                            {                    
                                switch(field_desc->cpp_type())
                                {
                                    if(j) out_repeated << repeated_delimiter;
                                    case google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE:
                                        out_repeated << goby::util::hex_encode(refl->GetRepeatedMessage(in, field_desc, j).SerializeAsString());
                                        break;    
                        
                                    case google::protobuf::FieldDescriptor::CPPTYPE_INT32:
                                        out_repeated << refl->GetRepeatedInt32(in, field_desc, j);
                                        break;
                            
                                    case google::protobuf::FieldDescriptor::CPPTYPE_INT64:
                                        out_repeated << refl->GetRepeatedInt64(in, field_desc, j);
                                        break;
                            
                                    case google::protobuf::FieldDescriptor::CPPTYPE_UINT32:
                                        out_repeated << refl->GetRepeatedUInt32(in, field_desc, j);
                                        break;
                            
                                    case google::protobuf::FieldDescriptor::CPPTYPE_UINT64:
                                        out_repeated << refl->GetRepeatedUInt64(in, field_desc, j);
                                        break;
                            
                                    case google::protobuf::FieldDescriptor::CPPTYPE_BOOL:
                                        out_repeated << std::boolalpha << refl->GetRepeatedBool(in, field_desc, j);
                                        break;
                            
                                    case google::protobuf::FieldDescriptor::CPPTYPE_STRING:
                                        if(field_desc->type() ==  google::protobuf::FieldDescriptor::TYPE_STRING)
                                            out_repeated << refl->GetRepeatedString(in, field_desc, j);
                                        else if(field_desc->type() ==  google::protobuf::FieldDescriptor::TYPE_BYTES)
                                            out_repeated << goby::util::hex_encode(refl->GetRepeatedString(in, field_desc, j));    
                                        break;                    
                            
                                    case google::protobuf::FieldDescriptor::CPPTYPE_FLOAT:
                                        out_repeated << refl->GetRepeatedFloat(in, field_desc, j);
                                        break;
                            
                                    case google::protobuf::FieldDescriptor::CPPTYPE_DOUBLE:
                                        out_repeated << std::setprecision(15) << refl->GetRepeatedDouble(in, field_desc, j);
                                        break;
                            
                                    case google::protobuf::FieldDescriptor::CPPTYPE_ENUM:
                                        out_repeated << refl->GetRepeatedEnum(in, field_desc, j)->name();
                                        break;
                        
                                }
                            }
                            out_format % out_repeated.str();
                        }
                        else
                        {
                            switch(field_desc->cpp_type())
                            {
                                case google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE:
                                    out_format % goby::util::hex_encode(refl->GetMessage(in, field_desc).SerializeAsString());
                                    break;
                        
                                case google::protobuf::FieldDescriptor::CPPTYPE_INT32:
                                    out_format % refl->GetInt32(in, field_desc);
                                    break;
                        
                                case google::protobuf::FieldDescriptor::CPPTYPE_INT64:
                                    out_format % refl->GetInt64(in, field_desc);                        
                                    break;

                                case google::protobuf::FieldDescriptor::CPPTYPE_UINT32:
                                    out_format % refl->GetUInt32(in, field_desc);
                                    break;

                                case google::protobuf::FieldDescriptor::CPPTYPE_UINT64:
                                    out_format % refl->GetUInt64(in, field_desc);
                                    break;
                        
                                case google::protobuf::FieldDescriptor::CPPTYPE_BOOL:
                                    out_format % refl->GetBool(in, field_desc);
                                    break;
                    
                                case google::protobuf::FieldDescriptor::CPPTYPE_STRING:
                                    if(field_desc->type() ==  google::protobuf::FieldDescriptor::TYPE_STRING)
                                        out_format % refl->GetString(in, field_desc);
                                    else if(field_desc->type() ==  google::protobuf::FieldDescriptor::TYPE_BYTES)
                                        out_format % goby::util::hex_encode(refl->GetString(in, field_desc));
                                    break;                    
                
                                case google::protobuf::FieldDescriptor::CPPTYPE_FLOAT:
                                    out_format % refl->GetFloat(in, field_desc);
                                    break;
                
                                case google::protobuf::FieldDescriptor::CPPTYPE_DOUBLE:
                                    out_format % refl->GetDouble(in, field_desc);
                                    break;
                
                                case google::protobuf::FieldDescriptor::CPPTYPE_ENUM:
                                    out_format % refl->GetEnum(in, field_desc)->name();
                                    break;
                            }
                        }
                    }
                    else if(mod_it != modified_values.end())
                    {
                        out_format % mod_it->second;
                    }
                    else
                    {
                        out_format % "unknown";
                    }
                }
                
                *out = out_format.str();
                    
            }
            

            
            static void parse(const std::string& in, google::protobuf::Message* out,
                              const google::protobuf::RepeatedPtrField<protobuf::TranslatorEntry::CreateParser::Algorithm>& algorithms,
                              std::string format)
            {
                
                const google::protobuf::Descriptor* desc = out->GetDescriptor();
                const google::protobuf::Reflection* refl = out->GetReflection();
                boost::to_lower(format);
                std::string str = in;
                std::string lower_str = boost::to_lower_copy(in);
                
//    goby::glog.is(DEBUG1) && goby::glog << "Format: " << format << std::endl;
//   goby::glog.is(DEBUG1) && goby::glog << "String: " << str << std::endl;
//    goby::glog.is(DEBUG1) && goby::glog << "Lower String: " << lower_str << std::endl;
            
                std::string::const_iterator i = format.begin();

                while (i != format.end())
                {
                    if (*i == '%')
                    {
                        ++i; // now *i is the conversion specifier
                        std::string specifier;
                        while(*i != '%')
                            specifier += *i++;                    

                        ++i; // now *i is the next separator
                        std::string extract = str.substr(0, lower_str.find(*i));
                        
                        int field_index = boost::lexical_cast<int>(specifier);                        
                        
                        try
                        {
                            const google::protobuf::FieldDescriptor* field_desc = desc->FindFieldByNumber(field_index);
                            
                            if(!field_desc)
                                throw(std::runtime_error("Bad field: " + specifier + " not in message " + desc->full_name()));

                            // run algorithms
                            typedef google::protobuf::RepeatedPtrField<protobuf::TranslatorEntry::CreateParser::Algorithm>::const_iterator const_iterator;
                        
                            for(const_iterator it = algorithms.begin(), n = algorithms.end();
                                it != n; ++it)
                            {
                                goby::transitional::DCCLMessageVal extract_val(extract);
                                
                                if(it->primary_field() == field_index)
                                    transitional::DCCLAlgorithmPerformer::getInstance()->
                                        run_algorithm(it->name(),
                                                      extract_val,
                                                      std::vector<goby::transitional::DCCLMessageVal>());
                                
                                extract = std::string(extract_val);
                            }

                            
                            switch(field_desc->cpp_type())
                            {
                                case google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE:
                                    field_desc->is_repeated()
                                        ? refl->AddMessage(out, field_desc)->ParseFromString(goby::util::hex_decode(extract))
                                        : refl->MutableMessage(out, field_desc)->ParseFromString(goby::util::hex_decode(extract));
                                    break;
                        
                                case google::protobuf::FieldDescriptor::CPPTYPE_INT32:
                                    field_desc->is_repeated()
                                        ? refl->AddInt32(out, field_desc, goby::util::as<google::protobuf::int32>(extract))
                                        : refl->SetInt32(out, field_desc, goby::util::as<google::protobuf::int32>(extract));
                                    break;
                        
                                case google::protobuf::FieldDescriptor::CPPTYPE_INT64:
                                    field_desc->is_repeated()
                                        ? refl->AddInt64(out, field_desc, goby::util::as<google::protobuf::int64>(extract))
                                        : refl->SetInt64(out, field_desc, goby::util::as<google::protobuf::int64>(extract));                        
                                    break;

                                case google::protobuf::FieldDescriptor::CPPTYPE_UINT32:
                                    field_desc->is_repeated()
                                        ? refl->AddUInt32(out, field_desc, goby::util::as<google::protobuf::uint32>(extract))
                                        : refl->SetUInt32(out, field_desc, goby::util::as<google::protobuf::uint32>(extract));
                                    break;

                                case google::protobuf::FieldDescriptor::CPPTYPE_UINT64:
                                    field_desc->is_repeated()
                                        ? refl->AddUInt64(out, field_desc, goby::util::as<google::protobuf::uint64>(extract))
                                        : refl->SetUInt64(out, field_desc, goby::util::as<google::protobuf::uint64>(extract));
                                    break;
                        
                                case google::protobuf::FieldDescriptor::CPPTYPE_BOOL:
                                    field_desc->is_repeated()                            
                                        ? refl->AddBool(out, field_desc, goby::util::as<bool>(extract))
                                        : refl->SetBool(out, field_desc, goby::util::as<bool>(extract));
                                    break;
                    
                                case google::protobuf::FieldDescriptor::CPPTYPE_STRING:
                                    field_desc->is_repeated()     
                                        ? refl->AddString(out, field_desc, extract)
                                        : refl->SetString(out, field_desc, extract);
                                    break;                    
                
                                case google::protobuf::FieldDescriptor::CPPTYPE_FLOAT:
                                    field_desc->is_repeated()     
                                        ? refl->AddFloat(out, field_desc, goby::util::as<float>(extract))
                                        : refl->SetFloat(out, field_desc, goby::util::as<float>(extract));
                                    break;
                
                                case google::protobuf::FieldDescriptor::CPPTYPE_DOUBLE:
                                    field_desc->is_repeated()
                                        ? refl->AddDouble(out, field_desc, goby::util::as<double>(extract))
                                        : refl->SetDouble(out, field_desc, goby::util::as<double>(extract));
                                    break;
                
                                case google::protobuf::FieldDescriptor::CPPTYPE_ENUM:
                                {
                                    const google::protobuf::EnumValueDescriptor* enum_desc =
                                        refl->GetEnum(*out, field_desc)->type()->FindValueByName(extract);

                                    // try upper case
                                    if(!enum_desc)
                                        enum_desc = refl->GetEnum(*out, field_desc)->type()->FindValueByName(boost::to_upper_copy(extract));
                                    // try lower case
                                    if(!enum_desc)
                                        enum_desc = refl->GetEnum(*out, field_desc)->type()->FindValueByName(boost::to_lower_copy(extract));
                                    if(enum_desc)
                                    {
                                        field_desc->is_repeated()
                                            ? refl->AddEnum(out, field_desc, enum_desc)
                                            : refl->SetEnum(out, field_desc, enum_desc);
                                    }
                        
                                }
                                break;
                            }

                        }
                        catch(boost::bad_lexical_cast&)
                        {
                            throw(std::runtime_error("Bad specifier: " + specifier + ", must be an integer. For message: " +  desc->full_name()));
                        }

                        // goby::glog.is(DEBUG1) && goby::glog << "field: [" << field_index << "], extract: [" << extract << "]" << std::endl;
                    }
                    else
                    {
                        // if it's not a %, eat!
                        std::string::size_type pos_to_remove = lower_str.find(*i)+1;
                        lower_str.erase(0, pos_to_remove);
                        str.erase(0, pos_to_remove);
                        ++i;
                    }
                }
            }
        };
    }
}



/// \file moos_protobuf_helpers.h Helpers for MOOS applications for serializing and parsed Google Protocol buffers messages

/// \brief Converts the Google Protocol Buffers message `msg` into a suitable (human readable) string `out` for sending via MOOS
///
/// \param out pointer to std::string to store serialized result
/// \param msg Google Protocol buffers message to serialize
inline void serialize_for_moos(std::string* out, const google::protobuf::Message& msg)
{
    goby::moos::MOOSTranslation<goby::moos::protobuf::TranslatorEntry::TECHNIQUE_PROTOBUF_TEXT_FORMAT>::serialize(out, msg);
}


/// \brief Parses the string `in` to Google Protocol Buffers message `msg`. All errors are written to the goby::util::glogger().
///
/// \param in std::string to parse
/// \param msg Google Protocol buffers message to store result
inline void parse_for_moos(const std::string& in, google::protobuf::Message* msg)
{
    goby::moos::MOOSTranslation<goby::moos::protobuf::TranslatorEntry::TECHNIQUE_PROTOBUF_TEXT_FORMAT>::parse(in, msg);    
}


   

#endif
