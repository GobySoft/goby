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

#include "configuration_reader.h"

#include "goby/util/liblogger/term_color.h"

using namespace goby::tcolor;

bool goby::core::ConfigReader::read_cfg(int argc,
                                        char* argv[],
                                        google::protobuf::Message* message,
                                        std::string* application_name,
                                        std::string* self_name,
                                        boost::program_options::variables_map* var_map)
{
    boost::filesystem::path launch_path(argv[0]);
    *application_name = launch_path.filename();
    
    namespace po = boost::program_options;
    std::string cfg_path;
    
    po::options_description od_all("Allowed options");

    po::options_description od_cli_only("Given on command line only");

    std::string cfg_path_desc = "path to " + *application_name + " configuration file (typically " + *application_name + ".cfg)";
    od_cli_only.add_options()
        ("cfg_path,c", po::value<std::string>(&cfg_path), cfg_path_desc.c_str())
        ("help,h", "writes this help message")
        ("self.name,n", po::value<std::string>(self_name), "name of this platform (same as gobyd configuration value `self.name`)");
    
    std::string od_both_desc = "Typically given in " + *application_name + " configuration file, but may be specified on the command line";
    po::options_description od_both(od_both_desc.c_str());

    od_all.add(od_cli_only);
    if(message)
    {
        get_protobuf_program_options(od_both, message->GetDescriptor());
        od_all.add(od_both);
    }
    
    try
    {
        po::positional_options_description p;
        p.add("cfg_path", 1);
        p.add("self.name", 2);
        
        po::store(po::command_line_parser(argc, argv).
                  options(od_all).positional(p).run(), *var_map);
        
        if (var_map->count("help")) {
            std::cout << od_all << "\n";
            return false;
        }
        
        po::notify(*var_map);

        if(message)
        {
            if(!cfg_path.empty())
            {
                
                // try to read file
                std::ifstream fin;
                fin.open(cfg_path.c_str(), std::ifstream::in);
                if(!fin.is_open())
                    throw(std::runtime_error(std::string("could not open '" + cfg_path + "' for reading. check value of --cfg_path")));        
                
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
                    err_msg << esc_red << s << "\n" << esc_nocolor;
                
                err_msg << "Make sure you specified a proper `cfg_path` to the configuration file.";
                throw(std::runtime_error(err_msg.str()));
            }
        }
        
    }
    catch(std::exception& e)
    {
        std::cerr << od_all << "\n" 
                  << "Problem parsing command-line configuration: \n"
                  << e.what() << "\n";

        return false;
    }
    
    return true;
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
                BOOST_FOREACH(std::string v,
                              value.as<std::vector<std::string> >())
                {
                    google::protobuf::TextFormat::MergeFromString(
                        v, refl->AddMessage(&message, field_desc));
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
                        throw(std::runtime_error(std::string("invalid enumeration " + v + " for field " + full_name)));
                    
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
                google::protobuf::TextFormat::MergeFromString(
                    value.as<std::string>(),
                    refl->MutableMessage(&message, field_desc));
                break;    
                    
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
                    throw(std::runtime_error(std::string("invalid enumeration " + value.as<std::string>() + " for field "
                                                         + full_name)));
                
                refl->SetEnum(&message, field_desc, enum_desc);
                break;                    
                
        }
    }
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
        human_desc_ss << esc_green << field_desc->options().GetExtension(proto::description) << esc_nocolor;

        append_label(human_desc_ss, field_desc);
        
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
                                                 std::stringstream& human_desc_ss,
                                                 const std::string& indent /*= ""*/)
{
    for(int i = 0, n = desc->field_count(); i < n; ++i)
    {
        const google::protobuf::FieldDescriptor* field_desc = desc->field(i);

        if(field_desc->cpp_type() == google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE)
        {
            
            human_desc_ss << "\n" << indent << field_desc->name() << " {  \t" <<
                esc_blue << field_desc->options().GetExtension(proto::description) << esc_nocolor;
            append_label(human_desc_ss, field_desc);

            build_description(field_desc->message_type(), human_desc_ss, indent + "  ");
            human_desc_ss << "\n" << indent << "}";


        }
        else
        {
            human_desc_ss << "\n" << indent;

            std::string example;
            if(field_desc->has_default_value())
                example = default_as_string(field_desc);
            else
                example = field_desc->options().GetExtension(proto::example); 

            if(field_desc->cpp_type() == google::protobuf::FieldDescriptor::CPPTYPE_STRING)
                example = "\"" + example + "\""; 

            
            human_desc_ss << field_desc->name() << ": "
                          << example;
            
            human_desc_ss << "  \t" << esc_blue << field_desc->options().GetExtension(proto::description) << esc_nocolor;
            append_label(human_desc_ss, field_desc);

            if(field_desc->has_default_value())
                human_desc_ss << " (default)";
        
        }
    }
}


void goby::core::ConfigReader::append_label(std::stringstream& human_desc_ss,
                                            const google::protobuf::FieldDescriptor* field_desc)
{
    switch(field_desc->label())
    {
        case google::protobuf::FieldDescriptor::LABEL_REQUIRED:
            human_desc_ss << esc_red << " (req)" << esc_nocolor;
            break;
            
        case google::protobuf::FieldDescriptor::LABEL_OPTIONAL:
            human_desc_ss << esc_lt_magenta << " (opt)" << esc_nocolor;
            break;
            
        case google::protobuf::FieldDescriptor::LABEL_REPEATED:
            human_desc_ss << esc_lt_cyan << " (repeat)"  << esc_nocolor;
            break;
    }
}


std::string goby::core::ConfigReader::default_as_string(const google::protobuf::FieldDescriptor* field_desc)
{
    switch(field_desc->cpp_type())
    {
        case google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE:
            return "";
        
        case google::protobuf::FieldDescriptor::CPPTYPE_INT32:                    
            return goby::util::as<std::string>(field_desc->default_value_int32());
                    
        case google::protobuf::FieldDescriptor::CPPTYPE_INT64:
            return goby::util::as<std::string>(field_desc->default_value_int64());

        case google::protobuf::FieldDescriptor::CPPTYPE_UINT32:
            return goby::util::as<std::string>(field_desc->default_value_uint32());
                                
        case google::protobuf::FieldDescriptor::CPPTYPE_UINT64:
            return goby::util::as<std::string>(field_desc->default_value_uint64());
                        
        case google::protobuf::FieldDescriptor::CPPTYPE_BOOL:
            return goby::util::as<std::string>(field_desc->default_value_bool());
                    
        case google::protobuf::FieldDescriptor::CPPTYPE_STRING:
            return field_desc->default_value_string();
            
        case google::protobuf::FieldDescriptor::CPPTYPE_FLOAT:
            return goby::util::as<std::string>(field_desc->default_value_float());
                        
        case google::protobuf::FieldDescriptor::CPPTYPE_DOUBLE:
            return goby::util::as<std::string>(field_desc->default_value_double());
                
        case google::protobuf::FieldDescriptor::CPPTYPE_ENUM:
            return field_desc->default_value_enum()->name();
            
                                      
    }
}
