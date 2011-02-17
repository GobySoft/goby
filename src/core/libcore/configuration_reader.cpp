// copyright 2010 t. schneider tes@mit.edu
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

#include "goby/core/proto/option_extensions.pb.h"
#include "goby/core/proto/app_base_config.pb.h"

#include "configuration_reader.h"

#include "goby/util/liblogger/term_color.h"
#include "exception.h"
#include <google/protobuf/dynamic_message.h>

// brings std::ostream& red, etc. into scope
using namespace goby::util::tcolor;

void goby::core::ConfigReader::read_cfg(int argc,
                                        char* argv[],
                                        google::protobuf::Message* message,
                                        std::string* application_name,
                                        boost::program_options::options_description* od_all,
                                        boost::program_options::variables_map* var_map)
{
    if(!argv) return;

    boost::filesystem::path launch_path(argv[0]);
    *application_name = launch_path.filename();
    
    std::string cfg_path;    

    boost::program_options::options_description od_cli_only("Given on command line only");

    std::string cfg_path_desc = "path to " + *application_name + " configuration file (typically " + *application_name + ".cfg)";
    
    std::string app_name_desc = "name to use when connecting to gobyd (default: " + std::string(argv[0]) + ")";
    od_cli_only.add_options()
        ("cfg_path,c", boost::program_options::value<std::string>(&cfg_path), cfg_path_desc.c_str())
        ("help,h", "writes this help message")
        ("platform_name,p", boost::program_options::value<std::string>(), "name of this platform (same as gobyd configuration value `self.name`)")
        ("app_name,a", boost::program_options::value<std::string>(), app_name_desc.c_str())
        ("verbose,v", boost::program_options::value<std::string>()->implicit_value("")->multitoken(), "output useful information to std::cout. -v is verbosity: verbose, -vv is verbosity: debug, -vvv is verbosity: gui");
    
    std::string od_both_desc = "Typically given in " + *application_name + " configuration file, but may be specified on the command line";
    boost::program_options::options_description od_both(od_both_desc.c_str());

    if(message)
    {
        get_protobuf_program_options(od_both, message->GetDescriptor());
        od_all->add(od_both);
    }
    od_all->add(od_cli_only);
    
    boost::program_options::positional_options_description p;
    p.add("cfg_path", 1);
    p.add("platform_name", 2);
    p.add("app_name", 3);
    
        
    boost::program_options::store(boost::program_options::command_line_parser(argc, argv).
                                  options(*od_all).positional(p).run(), *var_map);
    
    if (var_map->count("help"))
    {
        ConfigException e("");
        e.set_error(false);
        throw(e);
    }
    
    boost::program_options::notify(*var_map);
        
    if(message)
    {
        if(!cfg_path.empty())
        {
                
            // try to read file
            std::ifstream fin;
            fin.open(cfg_path.c_str(), std::ifstream::in);
            if(!fin.is_open())
                throw(ConfigException(std::string("could not open '" + cfg_path + "' for reading. check value of --cfg_path")));   
                
            google::protobuf::io::IstreamInputStream is(&fin);
                
            google::protobuf::TextFormat::Parser parser;
            // maybe the command line will fill in the missing pieces
            parser.AllowPartialMessage(true);
            parser.Parse(&is, message);
        }
        
        // add / overwrite any options that are specified in the cfg file with those given on the command line
        typedef std::pair<std::string, boost::program_options::variable_value> P;
        BOOST_FOREACH(const P&p, *var_map)
        {
            // let protobuf deal with the defaults
            if(!p.second.defaulted())
                set_protobuf_program_option(*var_map, *message, p.first, p.second);
        }
        
        // now the proto message must have all required fields
        if(!message->IsInitialized())
        {
            std::vector< std::string > errors;
            message->FindInitializationErrors(&errors);
                
            std::stringstream err_msg;
            err_msg << "Configuration is missing required parameters: \n";
            BOOST_FOREACH(const std::string& s, errors)
                err_msg << util::esc_red << s << "\n" << util::esc_nocolor;
                
            err_msg << "Make sure you specified a proper `cfg_path` to the configuration file.";
            throw(ConfigException(err_msg.str()));
        }
    }        
}


void goby::core::ConfigReader::set_protobuf_program_option(const boost::program_options::variables_map& var_map,
                                                           google::protobuf::Message& message,
                                                           const std::string& full_name,
                                                           const boost::program_options::variable_value& value)
{
    const google::protobuf::Descriptor* desc = message.GetDescriptor();
    const google::protobuf::Reflection* refl = message.GetReflection();
    
    const google::protobuf::FieldDescriptor* field_desc = desc->FindFieldByName(full_name);
    if(!field_desc) return;
    
    if(field_desc->is_repeated())
    {                    
        switch(field_desc->cpp_type())
        {
            case google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE:
                BOOST_FOREACH(std::string v,value.as<std::vector<std::string> >())
                {
                    google::protobuf::TextFormat::Parser parser;
                    parser.AllowPartialMessage(true);   
                    parser.MergeFromString(v, refl->AddMessage(&message, field_desc));
                }
                
                break;    
                    
            case google::protobuf::FieldDescriptor::CPPTYPE_INT32:
                BOOST_FOREACH(google::protobuf::int32 v,
                              value.as<std::vector<google::protobuf::int32> >())
                    refl->AddInt32(&message, field_desc, v);
                break;
                    
            case google::protobuf::FieldDescriptor::CPPTYPE_INT64:
                BOOST_FOREACH(google::protobuf::int64 v,
                              value.as<std::vector<google::protobuf::int64> >())
                    refl->AddInt64(&message, field_desc, v);
                break;

            case google::protobuf::FieldDescriptor::CPPTYPE_UINT32:
                BOOST_FOREACH(google::protobuf::uint32 v,
                              value.as<std::vector<google::protobuf::uint32> >())
                    refl->AddUInt32(&message, field_desc, v);
                break;

            case google::protobuf::FieldDescriptor::CPPTYPE_UINT64:
                BOOST_FOREACH(google::protobuf::uint64 v,
                              value.as<std::vector<google::protobuf::uint64> >())
                    refl->AddUInt64(&message, field_desc, v);
                break;
                        
            case google::protobuf::FieldDescriptor::CPPTYPE_BOOL:
                BOOST_FOREACH(bool v, value.as<std::vector<bool> >())
                    refl->AddBool(&message, field_desc, v);
                break;
                    
            case google::protobuf::FieldDescriptor::CPPTYPE_STRING:
                BOOST_FOREACH(const std::string& v,
                              value.as<std::vector<std::string> >())
                    refl->AddString(&message, field_desc, v);
                break;                    
                
            case google::protobuf::FieldDescriptor::CPPTYPE_FLOAT:
                BOOST_FOREACH(float v, value.as<std::vector<float> >())
                    refl->AddFloat(&message, field_desc, v);
                break;
                
            case google::protobuf::FieldDescriptor::CPPTYPE_DOUBLE:
                BOOST_FOREACH(double v, value.as<std::vector<double> >())
                    refl->AddDouble(&message, field_desc, v);
                break;
                
            case google::protobuf::FieldDescriptor::CPPTYPE_ENUM:
                BOOST_FOREACH(std::string v,
                              value.as<std::vector<std::string> >())
                {
                    const google::protobuf::EnumValueDescriptor* enum_desc =
                        refl->GetEnum(message, field_desc)->type()->FindValueByName(v);
                    if(!enum_desc)
                        throw(ConfigException(std::string("invalid enumeration " + v + " for field " + full_name)));
                    
                    refl->AddEnum(&message, field_desc, enum_desc);
                }
                
                break;                    
                
        }
    }
    else
    {   
        switch(field_desc->cpp_type())
        {
            case google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE:
            {
                google::protobuf::TextFormat::Parser parser;
                parser.AllowPartialMessage(true);
                parser.MergeFromString(value.as<std::string>(),refl->MutableMessage(&message, field_desc));
                break;    
            }
                    
            case google::protobuf::FieldDescriptor::CPPTYPE_INT32:
                refl->SetInt32(&message, field_desc, value.as<boost::int_least32_t>());
                break;
                    
            case google::protobuf::FieldDescriptor::CPPTYPE_INT64:
                refl->SetInt64(&message, field_desc, value.as<boost::int_least64_t>());
                break;

            case google::protobuf::FieldDescriptor::CPPTYPE_UINT32:
                refl->SetUInt32(&message, field_desc, value.as<boost::uint_least32_t>());
                break;

            case google::protobuf::FieldDescriptor::CPPTYPE_UINT64:
                refl->SetUInt64(&message, field_desc, value.as<boost::uint_least64_t>()); 
                break;
                        
            case google::protobuf::FieldDescriptor::CPPTYPE_BOOL:
                refl->SetBool(&message, field_desc, value.as<bool>());
                break;
                    
            case google::protobuf::FieldDescriptor::CPPTYPE_STRING:
                refl->SetString(&message, field_desc, value.as<std::string>());
                break;                    
                
            case google::protobuf::FieldDescriptor::CPPTYPE_FLOAT:
                refl->SetFloat(&message, field_desc, value.as<float>());
                break;
                
            case google::protobuf::FieldDescriptor::CPPTYPE_DOUBLE:
                refl->SetDouble(&message, field_desc, value.as<double>());
                break;
                
            case google::protobuf::FieldDescriptor::CPPTYPE_ENUM:
                const google::protobuf::EnumValueDescriptor* enum_desc =
                    refl->GetEnum(message, field_desc)->type()->FindValueByName(value.as<std::string>());
                if(!enum_desc)
                    throw(ConfigException(std::string("invalid enumeration " + value.as<std::string>() + " for field "
                                                         + full_name)));
                
                refl->SetEnum(&message, field_desc, enum_desc);
                break;                    
                
        }
    }
}

void goby::core::ConfigReader::get_example_cfg_file(google::protobuf::Message* message, std::ostream* stream, const std::string& indent /*= ""*/)
{
    build_description(message->GetDescriptor(), *stream, indent, false);
    *stream << std::endl;
}


void  goby::core::ConfigReader::get_protobuf_program_options(boost::program_options::options_description& po_desc,
                                                             const google::protobuf::Descriptor* desc)
{
    for(int i = 0, n = desc->field_count(); i < n; ++i)
    {
        const google::protobuf::FieldDescriptor* field_desc = desc->field(i);
        const std::string& field_name = field_desc->name();
        
        std::string cli_name = field_name;
        std::stringstream human_desc_ss;
        human_desc_ss << util::esc_lt_blue << field_desc->options().GetExtension(::description);
        append_label(human_desc_ss, field_desc);
        human_desc_ss << util::esc_nocolor;
        
        switch(field_desc->cpp_type())
        {
            case google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE:
            {
                build_description(field_desc->message_type(), human_desc_ss, "  ");
                
                set_single_option(po_desc,
                                  field_desc,
                                  std::string(),
                                  cli_name,
                                  human_desc_ss.str());
            }                
            break;
            
            case google::protobuf::FieldDescriptor::CPPTYPE_INT32:                    
            {
                set_single_option(po_desc,
                                  field_desc,
                                  field_desc->default_value_int32(),
                                  cli_name,
                                  human_desc_ss.str());
            }
            break;
                    
            case google::protobuf::FieldDescriptor::CPPTYPE_INT64:
            {
                set_single_option(po_desc,
                                  field_desc,
                                  field_desc->default_value_int64(),
                                  cli_name,
                                  human_desc_ss.str());
            }
            break;

            case google::protobuf::FieldDescriptor::CPPTYPE_UINT32:
            {
                set_single_option(po_desc,
                                  field_desc,
                                  field_desc->default_value_uint32(),
                                  cli_name,
                                  human_desc_ss.str());
            }
            break;
                                
            case google::protobuf::FieldDescriptor::CPPTYPE_UINT64:
            {
                set_single_option(po_desc,
                                  field_desc,
                                  field_desc->default_value_uint64(),
                                  cli_name,
                                  human_desc_ss.str());
            }
            break;
                        
            case google::protobuf::FieldDescriptor::CPPTYPE_BOOL:
            {
                set_single_option(po_desc,
                                  field_desc,
                                  field_desc->default_value_bool(),
                                  cli_name,
                                  human_desc_ss.str());
            }
            break;
                    
            case google::protobuf::FieldDescriptor::CPPTYPE_STRING:
            {
                set_single_option(po_desc,
                                  field_desc,
                                  field_desc->default_value_string(),
                                  cli_name,
                                  human_desc_ss.str());
            }
            break;                    
                
            case google::protobuf::FieldDescriptor::CPPTYPE_FLOAT:
            {
                set_single_option(po_desc,
                                  field_desc,
                                  field_desc->default_value_float(),
                                  cli_name,
                                  human_desc_ss.str());
            }
            break;
                        
            case google::protobuf::FieldDescriptor::CPPTYPE_DOUBLE:
            {
                set_single_option(po_desc,
                                  field_desc,
                                  field_desc->default_value_double(),
                                  cli_name,
                                  human_desc_ss.str());
            }
            break;
                
            case google::protobuf::FieldDescriptor::CPPTYPE_ENUM:
            {
                set_single_option(po_desc,
                                  field_desc,
                                  field_desc->default_value_enum()->name(),
                                  cli_name,
                                  human_desc_ss.str());
            }
            break;
        }
    }
}


void goby::core::ConfigReader::build_description(const google::protobuf::Descriptor* desc,
                                                 std::ostream& stream,
                                                 const std::string& indent /*= ""*/,
                                                 bool use_color /* = true */ )
{
    google::protobuf::DynamicMessageFactory factory;
    const google::protobuf::Message* default_msg = factory.GetPrototype(desc);

    for(int i = 0, n = desc->field_count(); i < n; ++i)
    {
        const google::protobuf::FieldDescriptor* field_desc = desc->field(i);

        if(field_desc->cpp_type() == google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE)
        {
            stream << "\n" << indent << field_desc->name() << " {  ";

            if(use_color)
                stream << util::esc_green;
            else
                stream << "# ";
            
            stream << field_desc->options().GetExtension(::description);
            append_label(stream, field_desc);

            if(use_color)
                stream << util::esc_nocolor;
            
            build_description(field_desc->message_type(), stream, indent + "  ", use_color);
            stream << "\n" << indent << "}";
        }
        else
        {
            stream << "\n" << indent;

            std::string example;
            if(field_desc->has_default_value())
                google::protobuf::TextFormat::PrintFieldValueToString(*default_msg, field_desc, -1, &example);
            else
            {
                example = field_desc->options().GetExtension(::example);
                if(field_desc->cpp_type() == google::protobuf::FieldDescriptor::CPPTYPE_STRING)
                    example = "\"" + example + "\""; 
            }
            
            stream << field_desc->name() << ": "
                          << example;

            stream << "  ";
            if(use_color)
                stream << util::esc_green;
            else
                stream << "# ";
            
            stream << field_desc->options().GetExtension(::description);

            if(field_desc->cpp_type() == google::protobuf::FieldDescriptor::CPPTYPE_ENUM)
            {
                stream << " (";
                for(int i = 0, n = field_desc->enum_type()->value_count(); i < n; ++i)
                {
                    if(i) stream << ", ";
                    stream << field_desc->enum_type()->value(i)->name();
                }
                
                stream << ")";
            }
            
            append_label(stream, field_desc);
            

            if(field_desc->has_default_value())
                stream << " (default=" << example << ")";

            if(use_color)
                stream << util::esc_nocolor;
        
        }
    }
}


void goby::core::ConfigReader::append_label(std::ostream& human_desc_ss,
                                            const google::protobuf::FieldDescriptor* field_desc)
{
    switch(field_desc->label())
    {
        case google::protobuf::FieldDescriptor::LABEL_REQUIRED:
            human_desc_ss << " (req)";
            break;
            
        case google::protobuf::FieldDescriptor::LABEL_OPTIONAL:
            human_desc_ss << " (opt)";
            break;
            
        case google::protobuf::FieldDescriptor::LABEL_REPEATED:
            human_desc_ss << " (repeat)";
            break;
    }
}

void goby::core::ConfigReader::merge_app_base_cfg(AppBaseConfig* base_cfg,
                        const boost::program_options::variables_map& var_map)
{
    if (var_map.count("verbose"))
    {
        switch(var_map["verbose"].as<std::string>().size())
        {
            case 0:
                base_cfg->set_verbosity(AppBaseConfig::VERBOSE);
                break;
            case 1:
                base_cfg->set_verbosity(AppBaseConfig::DEBUG);
                break;
            default:
            case 2:
                base_cfg->set_verbosity(AppBaseConfig::GUI);
                break;
        }
    }
}
