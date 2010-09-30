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

bool goby::core::ConfigReader::read_cfg(int argc,
                                        char* argv[],
                                        google::protobuf::Message* message,
                                        std::string* application_name,
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
        ("help,h", "writes this help message");

    std::string od_both_desc = "Typically given in " + *application_name + " configuration file, but may be specified on the command line";
    po::options_description od_both(od_both_desc.c_str());

    od_all.add(od_cli_only);
    if(message)
    {
        get_protobuf_program_options(od_both, *message);
        od_all.add(od_both);
    }
    
    try
    {
        po::positional_options_description p;
        p.add("cfg_path", 1);

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
                    throw(std::runtime_error(std::string("could not open '" + cfg_path + "' for reading")));        
                
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
            message->CheckInitialized();
        }
        
    }
    catch(std::exception& e)
    {
        std::cerr << "Problem parsing command-line configuration: "
                  << e.what() << "\n"
                  << od_all << "\n";
        return false;
    }
    
    return true;
}


void  goby::core::ConfigReader::set_protobuf_program_option(const boost::program_options::variables_map& var_map, google::protobuf::Message& message, const std::string& full_name, const boost::program_options::variable_value& value)
{
    const google::protobuf::Descriptor* desc = message.GetDescriptor();
    const google::protobuf::Reflection* refl = message.GetReflection();
    
    const google::protobuf::FieldDescriptor* field_desc = desc->FindFieldByName(full_name.substr(0, full_name.find('.')));
    if(!field_desc) return ;
    
    if(field_desc->is_repeated())
    {                    
        // not supported at the moment
    }
    else
    {   
        switch(field_desc->cpp_type())
        {
            case google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE:
                set_protobuf_program_option(var_map,
                                            *refl->MutableMessage(&message, field_desc),
                                            full_name.substr(full_name.find('.')+1),
                                            value);
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

void  goby::core::ConfigReader::get_protobuf_program_options(boost::program_options::options_description& po_desc, const google::protobuf::Message& message, const std::string& prefix /*= ""*/)
{
    const google::protobuf::Descriptor* desc = message.GetDescriptor();
    const google::protobuf::Reflection* refl = message.GetReflection();
    
    for(int i = 0, n = desc->field_count(); i < n; ++i)
    {
        const google::protobuf::FieldDescriptor* field_desc = desc->field(i);
        const std::string field_name = prefix + field_desc->name();
        
        std::string cli_name = field_name;

        std::string human_desc = field_desc->options().GetExtension(GobyExtend::description);
        
        if(field_desc->is_repeated())
        {                    
            // not supported at the moment
        }
        else
        {   
            switch(field_desc->cpp_type())
            {
                case google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE:
                {
                    get_protobuf_program_options(po_desc,
                                                 refl->GetMessage(message, field_desc),
                                                 std::string(field_name + "."));
                }
                
                break;    
                    
                case google::protobuf::FieldDescriptor::CPPTYPE_INT32:                    
                {
                    boost::program_options::typed_value<boost::int_least32_t>* v =
                        boost::program_options::value<boost::int_least32_t>();
                    if(field_desc->has_default_value())
                        v->default_value(field_desc->default_value_int32());
                    
                    po_desc.add_options()
                        (cli_name.c_str(), v, human_desc.c_str());
                }
                break;
                    
                case google::protobuf::FieldDescriptor::CPPTYPE_INT64:
                {
                    boost::program_options::typed_value<boost::int_least64_t>* v =
                        boost::program_options::value<boost::int_least64_t>();
                    if(field_desc->has_default_value())
                        v->default_value(field_desc->default_value_int64());
                    
                    po_desc.add_options()
                        (cli_name.c_str(), v, human_desc.c_str());
                }
                break;

                case google::protobuf::FieldDescriptor::CPPTYPE_UINT32:
                {
                    boost::program_options::typed_value<boost::uint_least32_t>* v =
                        boost::program_options::value<boost::uint_least32_t>();
                    if(field_desc->has_default_value())
                        v->default_value(field_desc->default_value_uint32());
                    
                    po_desc.add_options()
                        (cli_name.c_str(), v, human_desc.c_str());
                }
                break;
                                
                case google::protobuf::FieldDescriptor::CPPTYPE_UINT64:
                {
                    boost::program_options::typed_value<boost::uint_least64_t>* v =
                        boost::program_options::value<boost::uint_least64_t>();
                    if(field_desc->has_default_value())
                        v->default_value(field_desc->default_value_uint64());
                    
                    
                    po_desc.add_options()
                        (cli_name.c_str(), v, human_desc.c_str());
                }
                break;
                        
                case google::protobuf::FieldDescriptor::CPPTYPE_BOOL:
                {
                    boost::program_options::typed_value<bool>* v = boost::program_options::value<bool>();
                    if(field_desc->has_default_value())
                        v->default_value(field_desc->default_value_bool(),
                                         field_desc->default_value_bool() ? "true" : "false");
                    
                    po_desc.add_options()
                        (cli_name.c_str(), v, human_desc.c_str());
                }
                break;
                    
                case google::protobuf::FieldDescriptor::CPPTYPE_STRING:
                {
                    boost::program_options::typed_value<std::string>* v = boost::program_options::value<std::string>();
                    if(field_desc->has_default_value())
                        v->default_value(field_desc->default_value_string());
                    
                    po_desc.add_options()
                        (cli_name.c_str(), v, human_desc.c_str());
                }
                break;                    
                
                case google::protobuf::FieldDescriptor::CPPTYPE_FLOAT:
                {
                    boost::program_options::typed_value<float>* v = boost::program_options::value<float>();
                    if(field_desc->has_default_value())
                        v->default_value(field_desc->default_value_float());
                    
                    po_desc.add_options()
                        (cli_name.c_str(), v, human_desc.c_str());
                }
                break;
                        
                case google::protobuf::FieldDescriptor::CPPTYPE_DOUBLE:
                {
                    boost::program_options::typed_value<double>* v = boost::program_options::value<double>();
                    if(field_desc->has_default_value())
                        v->default_value(field_desc->default_value_double());
                    
                    po_desc.add_options()
                        (cli_name.c_str(), v, human_desc.c_str());
                }
                break;
                
                case google::protobuf::FieldDescriptor::CPPTYPE_ENUM:
                {
                    boost::program_options::typed_value<std::string>* v = boost::program_options::value<std::string>();
                    if(field_desc->has_default_value())
                        v->default_value(field_desc->default_value_enum()->name());
                    
                    po_desc.add_options()
                        (cli_name.c_str(), v, human_desc.c_str());
                }
                break;                    
                
            }
        }
    }
}
