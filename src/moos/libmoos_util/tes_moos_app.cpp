// t. schneider tes@mit.edu 07.26.10
// ocean engineering graudate student - mit / whoi joint program
// massachusetts institute of technology (mit)
// laboratory for autonomous marine sensing systems (lamss)      
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


#include "tes_moos_app.h"

#include <boost/foreach.hpp>
#include <boost/bind.hpp>

#include "goby/util/string.h"

using goby::glog;
using goby::util::as;

std::string TesMoosApp::mission_file_;
std::string TesMoosApp::application_name_;

int TesMoosApp::argc_ = 0;
char** TesMoosApp::argv_ = 0;

bool TesMoosApp::Iterate()
{
    if(!configuration_read_) return true;

    // clear out MOOSApp cout for ncurses "scope" mode
    // MOOS has stopped talking by first Iterate()
    if(!cout_cleared_)
    {
        glog.refresh();
        cout_cleared_ = true;
    }

    while(connected_ && !msg_buffer_.empty())
    {
        glog << "writing from buffer: " << msg_buffer_.front().GetKey() << ": " << msg_buffer_.front().GetAsString() << std::endl;
        m_Comms.Post(msg_buffer_.front());
        msg_buffer_.pop_front();
    }
    
    
    loop();
    return true;
}    

bool TesMoosApp::OnNewMail(MOOSMSG_LIST &NewMail)
{
    BOOST_FOREACH(const CMOOSMsg& msg, NewMail)
    {
        // update dynamic moos variables - do this inside the loop so the newest is
        // also the one referenced in the call to inbox()
        dynamic_vars().update_moos_vars(msg);   

        if(msg.GetTime() < start_time_ && ignore_stale_)
        {
            glog.is(warn) &&
                glog << "ignoring normal mail from " << msg.GetKey()
                          << " from before we started (dynamics still updated)"
                          << std::endl;
        }
        else if(mail_handlers_.count(msg.GetKey()))
            mail_handlers_[msg.GetKey()](msg);
        else
        {
            glog.is(die) &&
                glog << "received mail that we have no handler for!" << std::endl;
        }
        
    }
    
    return true;    
}

bool TesMoosApp::OnConnectToServer()
{
    std::cout << m_MissionReader.GetAppName() << ", connected to server." << std::endl;
    connected_ = true;
    try_subscribing();
    
    BOOST_FOREACH(const TesMoosAppConfig::Initializer& ini,
                  common_cfg_.initializer())
    {   
        if(ini.has_global_cfg_var())
        {
            std::string result;
            if(m_MissionReader.GetValue(ini.global_cfg_var(), result))
            {
                if(ini.type() == TesMoosAppConfig::Initializer::INI_DOUBLE)
                    publish(ini.moos_var(), as<double>(result));
                else if(ini.type() == TesMoosAppConfig::Initializer::INI_STRING)
                    publish(ini.moos_var(), result);
            }
        }
        else
        {
            if(ini.type() == TesMoosAppConfig::Initializer::INI_DOUBLE)
                publish(ini.moos_var(), ini.dval());
            else if(ini.type() == TesMoosAppConfig::Initializer::INI_STRING)
                publish(ini.moos_var(), ini.sval());            
        }        
    }
    
    return true;
}

bool TesMoosApp::OnStartUp()
{
    std::cout << m_MissionReader.GetAppName () << ", starting ..." << std::endl;
    CMOOSApp::SetCommsFreq(common_cfg_.comm_tick());
    CMOOSApp::SetAppFreq(common_cfg_.app_tick());
    started_up_ = true;
    try_subscribing();
    return true;
}


void TesMoosApp::subscribe(const std::string& var,  InboxFunc handler, int blackout /* = 0 */ )
{
    glog.is(verbose) &&
        glog << "subscribing for MOOS variable: " << var << " @ " << blackout << std::endl;
    
    pending_subscriptions_.push_back(std::make_pair(var, blackout));
    try_subscribing();
    mail_handlers_[var] = handler;
}

void TesMoosApp::try_subscribing()
{
    if (connected_ && started_up_)
        do_subscriptions();
}

void TesMoosApp::do_subscriptions()
{
    while(!pending_subscriptions_.empty())
    {
        // variable name, blackout
        m_Comms.Register(pending_subscriptions_.front().first,
                         pending_subscriptions_.front().second);
        glog.is(verbose) &&
            glog << "subscribed for: " << pending_subscriptions_.front().first << std::endl;
        pending_subscriptions_.pop_front();
    }
}


void TesMoosApp::fetch_moos_globals(google::protobuf::Message* msg,
                                    CMOOSFileReader& moos_file_reader)
{
    const google::protobuf::Descriptor* desc = msg->GetDescriptor();
    const google::protobuf::Reflection* refl = msg->GetReflection();

    for(int i = 0, n = desc->field_count(); i < n; ++i)
    {
        const google::protobuf::FieldDescriptor* field_desc = desc->field(i);
        if(field_desc->is_repeated())
            continue;
        
        std::string moos_global = field_desc->options().GetExtension(goby::field).moos_global();
        
        switch(field_desc->cpp_type())
        {
            case google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE:
                fetch_moos_globals(refl->MutableMessage(msg, field_desc), moos_file_reader);
                break;    
                    
            case google::protobuf::FieldDescriptor::CPPTYPE_INT32:
            {
                int result;
                if(moos_file_reader.GetValue(moos_global, result))
                    refl->SetInt32(msg, field_desc, result);
                break;
            }
            
            case google::protobuf::FieldDescriptor::CPPTYPE_INT64:
            {
                int result;
                if(moos_file_reader.GetValue(moos_global, result))
                    refl->SetInt64(msg, field_desc, result);
                break;
            }

            case google::protobuf::FieldDescriptor::CPPTYPE_UINT32:
            {
                unsigned result;
                if(moos_file_reader.GetValue(moos_global, result))
                    refl->SetUInt32(msg, field_desc, result);
                break;
            }

            case google::protobuf::FieldDescriptor::CPPTYPE_UINT64:
            {
                unsigned result;
                if(moos_file_reader.GetValue(moos_global, result))
                    refl->SetUInt64(msg, field_desc, result);
                break;
            }
                        
            case google::protobuf::FieldDescriptor::CPPTYPE_BOOL:
            {
                bool result;
                if(moos_file_reader.GetValue(moos_global, result))
                    refl->SetBool(msg, field_desc, result);
                break;
            }
                    
            case google::protobuf::FieldDescriptor::CPPTYPE_STRING:
            {
                std::string result;
                if(moos_file_reader.GetValue(moos_global, result))
                    refl->SetString(msg, field_desc, result);
                break;
            }
                
            case google::protobuf::FieldDescriptor::CPPTYPE_FLOAT:
            {
                float result;
                if(moos_file_reader.GetValue(moos_global, result))
                    refl->SetFloat(msg, field_desc, result);
                break;
            }

            case google::protobuf::FieldDescriptor::CPPTYPE_DOUBLE:
            {
                double result;
                if(moos_file_reader.GetValue(moos_global, result))
                    refl->SetDouble(msg, field_desc, result);
                break;
            }
                
            case google::protobuf::FieldDescriptor::CPPTYPE_ENUM:
            {
                std::string result;
                if(moos_file_reader.GetValue(moos_global, result))
                {                
                    const google::protobuf::EnumValueDescriptor* enum_desc =
                        refl->GetEnum(*msg, field_desc)->type()->FindValueByName(result);
                    if(!enum_desc)
                        throw(std::runtime_error(std::string("invalid enumeration " +
                                                             result + " for field " +
                                                             field_desc->name())));
                    
                    refl->SetEnum(msg, field_desc, enum_desc);
                }
                break;
            }
        }
    }
}

void TesMoosApp::read_configuration(google::protobuf::Message* cfg)
{

    boost::filesystem::path launch_path(argv_[0]);
    application_name_ = launch_path.filename();

    //
    // READ CONFIGURATION
    //
    
    boost::program_options::options_description od_all;    
    boost::program_options::variables_map var_map;
    try
    {        
    
        boost::program_options::options_description od_cli_only("Given on command line only");    
        od_cli_only.add_options()
            ("help,h", "writes this help message")
            ("moos_file,c", boost::program_options::value<std::string>(&mission_file_), "path to .moos file")
            ("moos_name,a", boost::program_options::value<std::string>(&application_name_), "name to register with MOOS")
            ("example_config,e", "writes an example .moos ProcessConfig block")
            ("version,V", "writes the current version");
        
    
        boost::program_options::options_description od_both("Typically given in the .moos file, but may be specified on the command line");
    
        goby::core::ConfigReader::get_protobuf_program_options(od_both, cfg->GetDescriptor());
        od_all.add(od_both);
        od_all.add(od_cli_only);

        boost::program_options::positional_options_description p;
        p.add("moos_file", 1);
        p.add("moos_name", 2);
        
        boost::program_options::store(boost::program_options::command_line_parser(argc_, argv_).
                                      options(od_all).positional(p).run(), var_map);


        boost::program_options::notify(var_map);
        
        if (var_map.count("help"))
        {
            goby::core::ConfigException e("");
            e.set_error(false);
            throw(e);
        }
        else if(var_map.count("example_config"))
        {
            std::cout << "ProcessConfig = " << application_name_ << "\n{";
            goby::core::ConfigReader::get_example_cfg_file(cfg, &std::cout, "  ");
            std::cout << "}" << std::endl;
            exit(EXIT_SUCCESS);            
        }
        else if(var_map.count("version"))
        {
            std::cout << "This is Version " << goby::VERSION_STRING
                      << " of the Goby Underwater Autonomy Project released on "
                      << goby::VERSION_DATE
                      << ".\nSee https://launchpad.net/goby to search for updates." << std::endl;
            exit(EXIT_SUCCESS);            
        }
        
        glog.set_name(application_name_);
        glog.add_stream("verbose", &std::cout);
    
    
        std::string protobuf_text;
        std::ifstream fin;
        fin.open(mission_file_.c_str());
        if(fin.is_open())
        {
            std::string line;
            bool in_process_config = false;
            while(!getline(fin, line).eof())
            {
                std::string no_blanks_line = boost::algorithm::erase_all_copy(line, " ");
                if(boost::algorithm::iequals(no_blanks_line, "PROCESSCONFIG=" +  application_name_))
                {
                    in_process_config = true;
                }
                else if(in_process_config &&
                        !boost::algorithm::ifind_first(line, "PROCESSCONFIG").empty())
                {
                    break;
                }

                if(in_process_config)
                    protobuf_text += line + "\n";


            }

            if(!in_process_config)
            {
                glog.is(die) &&
                    glog << "no ProcessConfig block for " << application_name_ << std::endl;
            }
            
            // trim off "ProcessConfig = __ {"
            protobuf_text.erase(0, protobuf_text.find_first_of('{')+1);
            
            // trim off last "}" and anything that follows
            protobuf_text.erase(protobuf_text.find_last_of('}'));
            
            // convert "//" to "#" for comments
            boost::algorithm::replace_all(protobuf_text, "//", "#");
            
            
                
            google::protobuf::TextFormat::Parser parser;
            FlexOStreamErrorCollector error_collector(protobuf_text);
            parser.RecordErrorsTo(&error_collector);
            parser.AllowPartialMessage(true);
            parser.ParseFromString(protobuf_text, cfg);
            if(error_collector.has_errors())
            {
                glog.is(die) && 
                    glog << "fatal configuration errors (see above)" << std::endl;    
            }            
        }
        else
        {
            glog.is(warn) &&
                glog << "failed to open " << mission_file_ << std::endl;
        }
    
        fin.close();
    
        CMOOSFileReader moos_file_reader;
        moos_file_reader.SetFile(mission_file_);
        fetch_moos_globals(cfg, moos_file_reader);



// add / overwrite any options that are specified in the cfg file with those given on the command line
        typedef std::pair<std::string, boost::program_options::variable_value> P;
        BOOST_FOREACH(const P&p, var_map)
        {
            // let protobuf deal with the defaults
            if(!p.second.defaulted())
                goby::core::ConfigReader::set_protobuf_program_option(var_map, *cfg, p.first, p.second);
        }

        
        // now the proto message must have all required fields
        if(!cfg->IsInitialized())
        {
            std::vector< std::string > errors;
            cfg->FindInitializationErrors(&errors);
                
            std::stringstream err_msg;
            err_msg << "Configuration is missing required parameters: \n";
            BOOST_FOREACH(const std::string& s, errors)
                err_msg << goby::util::esc_red << s << "\n" << goby::util::esc_nocolor;
                
            err_msg << "Make sure you specified a proper .moos file";
            throw(goby::core::ConfigException(err_msg.str()));
        }
        
    }
    catch(goby::core::ConfigException& e)
    {
        // output all the available command line options
        std::cerr << od_all << "\n";
        if(e.error())
            std::cerr << "Problem parsing command-line configuration: \n"
                      << e.what() << "\n";
        
        throw;
    }




    
    



}



void TesMoosApp::process_configuration()
{
    //
    // PROCESS CONFIGURATION
    //
    switch(common_cfg_.verbosity())
    {
        case TesMoosAppConfig::VERBOSITY_VERBOSE:
            glog.add_stream(goby::util::Logger::VERBOSE, &std::cout);
            break;
        case TesMoosAppConfig::VERBOSITY_WARN:
            glog.add_stream(goby::util::Logger::WARN, &std::cout);
            break;
        case TesMoosAppConfig::VERBOSITY_DEBUG:
            glog.add_stream(goby::util::Logger::DEBUG1, &std::cout);
            break;
        case TesMoosAppConfig::VERBOSITY_GUI:
            glog.add_stream(goby::util::Logger::GUI, &std::cout);
            break;
        case TesMoosAppConfig::VERBOSITY_QUIET:
            glog.add_stream(goby::util::Logger::QUIET, &std::cout);
            break;
    }    


    if(common_cfg_.log())
    {
        if(!common_cfg_.has_log_path())
        {
            glog.is(warn) &&
                glog << "logging all terminal output to default directory (" << common_cfg_.log_path() << ")." << "set log_path for another path " << std::endl;
        }

        if(!common_cfg_.log_path().empty())
        {
            using namespace boost::posix_time;
            std::string file_name = application_name_ + "_" + common_cfg_.community() + "_" + to_iso_string(second_clock::universal_time()) + ".txt";

            glog.is(verbose) &&
                glog << "logging output to file: " << file_name << std::endl;
            
            fout_.open(std::string(common_cfg_.log_path() + "/" + file_name).c_str());
        
            // if fails, try logging to this directory
            if(!fout_.is_open())
            {
                fout_.open(std::string("./" + file_name).c_str());
                glog.is(warn) &&
                    glog << "logging to current directory because given directory is unwritable!" << std::endl;
            }
            // if still no go, quit
            if(!fout_.is_open())
            {
                
                glog.is(die) && glog << die << "cannot write to current directory, so cannot log." << std::endl;
            }
            
            glog.add_stream(goby::util::Logger::VERBOSE, &fout_);
        }
    }

}
