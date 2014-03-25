// Copyright 2009-2014 Toby Schneider (https://launchpad.net/~tes)
//                     GobySoft, LLC (2013-)
//                     Massachusetts Institute of Technology (2007-2014)
//                     Goby Developers Team (https://launchpad.net/~goby-dev)
// 
//
// This file is part of the Goby Underwater Autonomy Project Libraries
// ("The Goby Libraries").
//
// The Goby Libraries are free software: you can redistribute them and/or modify
// them under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 2.1 of the License, or
// (at your option) any later version.
//
// The Goby Libraries are distributed in the hope that they will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with Goby.  If not, see <http://www.gnu.org/licenses/>.




#include "goby_moos_app.h"

#include <boost/foreach.hpp>
#include <boost/bind.hpp>
#include <boost/filesystem.hpp>

#include "goby/util/as.h"

using goby::glog;
using goby::util::as;
using namespace goby::common::logger;


std::string GobyMOOSApp::mission_file_;
std::string GobyMOOSApp::application_name_;

int GobyMOOSApp::argc_ = 0;
char** GobyMOOSApp::argv_ = 0;

bool GobyMOOSApp::Iterate()
{
    if(!configuration_read_) return true;

    // clear out MOOSApp cout for ncurses "scope" mode
    // MOOS has stopped talking by first Iterate()
    if(!cout_cleared_)
    {
        glog.refresh();
        cout_cleared_ = true;
    }

    while(!msg_buffer_.empty() && (connected_ && started_up_))
    {
        glog << "writing from buffer: " << msg_buffer_.front().GetKey() << ": " << msg_buffer_.front().GetAsString() << std::endl;
        m_Comms.Post(msg_buffer_.front());
        msg_buffer_.pop_front();
    }
    
    
    loop();
    return true;
}    

bool GobyMOOSApp::OnNewMail(MOOSMSG_LIST &NewMail)
{
    for(MOOSMSG_LIST::const_iterator it = NewMail.begin(),
        end = NewMail.end(); it != end; ++it)
    {
        const CMOOSMsg& msg = *it;
        goby::glog.is(DEBUG3) && goby::glog << "Received mail: " << msg.GetKey() << ", time: " << std::setprecision(15) << msg.GetTime() << std::endl;        
        
        // update dynamic moos variables - do this inside the loop so the newest is
        // also the one referenced in the call to inbox()
        dynamic_vars().update_moos_vars(msg);   

        if(msg.GetTime() < start_time_ && ignore_stale_)
        {
            glog.is(WARN) &&
                glog << "ignoring normal mail from " << msg.GetKey()
                          << " from before we started (dynamics still updated)"
                          << std::endl;
        }
        else if(mail_handlers_.count(msg.GetKey()))
            (*mail_handlers_[msg.GetKey()])(msg);
    }
    
    return true;    
}

bool GobyMOOSApp::OnConnectToServer()
{
    std::cout << m_MissionReader.GetAppName() << ", connected to server." << std::endl;
    connected_ = true;
    try_subscribing();


    for(google::protobuf::RepeatedPtrField<GobyMOOSAppConfig::Initializer>::const_iterator it = common_cfg_.initializer().begin(), end = common_cfg_.initializer().end(); it != end; ++it)
    {
        const GobyMOOSAppConfig::Initializer& ini = *it;
        if(ini.has_global_cfg_var())
        {
            std::string result;
            if(m_MissionReader.GetValue(ini.global_cfg_var(), result))
            {
                if(ini.type() == GobyMOOSAppConfig::Initializer::INI_DOUBLE)
                    publish(ini.moos_var(), as<double>(result));
                else if(ini.type() == GobyMOOSAppConfig::Initializer::INI_STRING)
                    publish(ini.moos_var(), ini.trim() ?
                            boost::trim_copy(result) :
                            result);
            }
        }
        else
        {
            if(ini.type() == GobyMOOSAppConfig::Initializer::INI_DOUBLE)
                publish(ini.moos_var(), ini.dval());
            else if(ini.type() == GobyMOOSAppConfig::Initializer::INI_STRING)
                publish(ini.moos_var(), ini.trim() ?
                        boost::trim_copy(ini.sval()) :
                        ini.sval()); 
        }        
    }
    
    return true;
}

bool GobyMOOSApp::OnStartUp()
{
    std::cout << m_MissionReader.GetAppName () << ", starting ..." << std::endl;
    CMOOSApp::SetCommsFreq(common_cfg_.comm_tick());
    CMOOSApp::SetAppFreq(common_cfg_.app_tick());
    started_up_ = true;
    try_subscribing();
    return true;
}


void GobyMOOSApp::subscribe(const std::string& var,  InboxFunc handler, int blackout /* = 0 */ )
{
    glog.is(VERBOSE) &&
        glog << "subscribing for MOOS variable: " << var << " @ " << blackout << std::endl;
    
    pending_subscriptions_.push_back(std::make_pair(var, blackout));
    try_subscribing();

    if(!mail_handlers_[var])
        mail_handlers_[var].reset(new boost::signals2::signal<void (const CMOOSMsg& msg)>);
        
    if(handler)
        mail_handlers_[var]->connect(handler);
}

void GobyMOOSApp::try_subscribing()
{
    if (connected_ && started_up_)
        do_subscriptions();
}

void GobyMOOSApp::do_subscriptions()
{
    while(!pending_subscriptions_.empty())
    {
        // variable name, blackout
        m_Comms.Register(pending_subscriptions_.front().first,
                         pending_subscriptions_.front().second);
        glog.is(VERBOSE) &&
            glog << "subscribed for: " << pending_subscriptions_.front().first << std::endl;
        pending_subscriptions_.pop_front();
    }
}


void GobyMOOSApp::fetch_moos_globals(google::protobuf::Message* msg,
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

void GobyMOOSApp::read_configuration(google::protobuf::Message* cfg)
{

    boost::filesystem::path launch_path(argv_[0]);

#if BOOST_FILESYSTEM_VERSION == 3
    application_name_ = launch_path.filename().string();
#else
    application_name_ = launch_path.filename();
#endif


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
    
        goby::common::ConfigReader::get_protobuf_program_options(od_both, cfg->GetDescriptor());
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
            goby::common::ConfigException e("");
            e.set_error(false);
            throw(e);
        }
        else if(var_map.count("example_config"))
        {
            std::cout << "ProcessConfig = " << application_name_ << "\n{";
            goby::common::ConfigReader::get_example_cfg_file(cfg, &std::cout, "  ");
            std::cout << "}" << std::endl;
            exit(EXIT_SUCCESS);            
        }
        else if(var_map.count("version"))
        {
            std::cout << goby::version_message() << std::endl;
            exit(EXIT_SUCCESS);            
        }
        
        glog.set_name(application_name_);
        glog.add_stream(goby::common::protobuf::GLogConfig::VERBOSE, &std::cout);    
    
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
                glog.is(DIE) &&
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
            if(error_collector.has_errors() || error_collector.has_warnings())
            {
                glog.is(DIE) && 
                    glog << "fatal configuration errors (see above)" << std::endl;    
            }            
        }
        else
        {
            glog.is(WARN) &&
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
                goby::common::ConfigReader::set_protobuf_program_option(var_map, *cfg, p.first, p.second);
        }

        
        // now the proto message must have all required fields
        if(!cfg->IsInitialized())
        {
            std::vector< std::string > errors;
            cfg->FindInitializationErrors(&errors);
                
            std::stringstream err_msg;
            err_msg << "Configuration is missing required parameters: \n";
            BOOST_FOREACH(const std::string& s, errors)
                err_msg << goby::common::esc_red << s << "\n" << goby::common::esc_nocolor;
                
            err_msg << "Make sure you specified a proper .moos file";
            throw(goby::common::ConfigException(err_msg.str()));
        }
        
    }
    catch(goby::common::ConfigException& e)
    {
        // output all the available command line options
        std::cerr << od_all << "\n";
        if(e.error())
            std::cerr << "Problem parsing command-line configuration: \n"
                      << e.what() << "\n";
        
        throw;
    }

}



void GobyMOOSApp::process_configuration()
{
    //
    // PROCESS CONFIGURATION
    //
    glog.add_stream(common_cfg_.verbosity(), &std::cout);
    if(common_cfg_.show_gui())
    {
        glog.enable_gui();
    }
    
    
    if(common_cfg_.log())
    {
        if(!common_cfg_.has_log_path())
        {
            glog.is(WARN) &&
                glog << "logging all terminal output to default directory (" << common_cfg_.log_path() << ")." << "set log_path for another path " << std::endl;
        }

        if(!common_cfg_.log_path().empty())
        {
            using namespace boost::posix_time;
            std::string file_name =
                boost::replace_all_copy(application_name_, "/", "_") + "_" +
                common_cfg_.community() + "_" +
                to_iso_string(second_clock::universal_time()) + ".txt";

            glog.is(VERBOSE) &&
                glog << "logging output to file: " << file_name << std::endl;
            
            fout_.open(std::string(common_cfg_.log_path() + "/" + file_name).c_str());
        
            // if fails, try logging to this directory
            if(!fout_.is_open())
            {
                fout_.open(std::string("./" + file_name).c_str());
                glog.is(WARN) &&
                    glog << "logging to current directory because given directory is unwritable!" << std::endl;
            }
            // if still no go, quit
            if(!fout_.is_open())
            {
                
                glog.is(DIE) && glog << die << "cannot write to current directory, so cannot log." << std::endl;
            }
            
            glog.add_stream(common_cfg_.log_verbosity(), &fout_);
        }
    }

}
