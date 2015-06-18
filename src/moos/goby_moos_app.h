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


#ifndef GOBYMOOSAPP20100726H
#define GOBYMOOSAPP20100726H

#include "goby/moos/moos_header.h"
#include "goby/util/as.h"

#include <boost/date_time/posix_time/posix_time.hpp>
#include <map>
#include <boost/function.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/bind.hpp>
#include <boost/signals2.hpp>
#include <boost/filesystem.hpp>

#include "dynamic_moos_vars.h"
#include "goby/common/logger.h"
#include "goby/moos/protobuf/goby_moos_app.pb.h"
#include "goby/common/configuration_reader.h"
#include "goby/common/exception.h"
#include "moos_protobuf_helpers.h"
#include "goby/version.h"

namespace goby
{
    namespace moos
    {
        template<typename App>
            int run(int argc, char* argv[]);


        template<typename ProtobufMessage>
        inline void protobuf_inbox(const CMOOSMsg& msg,
                                   boost::function<void (const ProtobufMessage& msg)> handler)
        {
            ProtobufMessage pb_msg;
            parse_for_moos(msg.GetString(), &pb_msg);
            handler(pb_msg);
        }
    }
}

// shell implementation so we can call superclass methods for when
// using AppCastingMOOSApp
class MOOSAppShell : public CMOOSApp
{
  protected:
    bool Iterate() { return true; }
    bool OnStartUp() { return true; }
    bool OnConnectToServer() { return true; }
    bool OnNewMail(MOOSMSG_LIST &NewMail) { return true; }
    void RegisterVariables() { }
    void PostReport() { }
};

template <class MOOSAppType = MOOSAppShell>
    class GobyMOOSAppSelector : public MOOSAppType
    {
      protected:
      typedef boost::function<void (const CMOOSMsg& msg)> InboxFunc;
      
      template<typename ProtobufConfig>
      explicit GobyMOOSAppSelector(ProtobufConfig* cfg)
      : start_time_(MOOSTime()),
      configuration_read_(false),
      cout_cleared_(false),
      connected_(false),
      started_up_(false),
      ignore_stale_(true)
      {
          using goby::glog;

          read_configuration(cfg);
    
          // keep a copy for ourselves
          common_cfg_ = cfg->common();
          configuration_read_ = true;

          process_configuration();

          glog.is(goby::common::logger::DEBUG2) && glog << cfg->DebugString() << std::endl;
      }

      
      virtual ~GobyMOOSAppSelector() { }

      template<typename ProtobufMessage>
      void publish_pb(const std::string& key, const ProtobufMessage& msg)
      {
          std::string serialized;
          serialize_for_moos(&serialized, msg);
          publish(key, serialized);
      }
      
  
      void publish(CMOOSMsg& msg)
      {
          if(connected_ && started_up_)
              MOOSAppType::m_Comms.Post(msg);
          else
              msg_buffer_.push_back(msg);
      }
    
      void publish(const std::string& key, const std::string& value)
      {
          CMOOSMsg msg(MOOS_NOTIFY, key, value);
          publish(msg);
      }
    
      void publish(const std::string& key, double value)
      {
          CMOOSMsg msg(MOOS_NOTIFY, key, value);
          publish(msg);
      }    
    
      goby::moos::DynamicMOOSVars& dynamic_vars() { return dynamic_vars_; }
      double start_time() const { return start_time_; }

      void subscribe(const std::string& var,
                     InboxFunc handler = InboxFunc(),
                     int blackout = 0);
      
      template<typename V, typename A1>
      void subscribe(const std::string& var,
                     void(V::*mem_func)(A1),
                     V* obj,
                     int blackout = 0)
      { subscribe(var, boost::bind(mem_func, obj, _1), blackout); }    


      template<typename V, typename ProtobufMessage>
      void subscribe_pb(const std::string& var,
                     void(V::*mem_func)(const ProtobufMessage&),
                     V* obj,
                     int blackout = 0)
      { subscribe_pb<ProtobufMessage>(var, boost::bind(mem_func, obj, _1), blackout); }
            
      template<typename ProtobufMessage>
      void subscribe_pb(const std::string& var,
                        boost::function<void (const ProtobufMessage& msg)> handler,
                        int blackout = 0)
      {
          subscribe(var,
                    boost::bind(&goby::moos::protobuf_inbox<ProtobufMessage>, _1, handler),
                    blackout);
      }
      
      template<typename App>
      friend int ::goby::moos::run(int argc, char* argv[]);

      virtual void loop() = 0;

      bool ignore_stale() { return ignore_stale_; }
      void set_ignore_stale(bool b) { ignore_stale_ = b; }

    
      private:
      // from CMOOSApp
      bool Iterate();
      bool OnStartUp();
      bool OnConnectToServer();
      bool OnNewMail(MOOSMSG_LIST &NewMail);
      void try_subscribing();
      void do_subscriptions();
    
      void fetch_moos_globals(google::protobuf::Message* msg,
                              CMOOSFileReader& moos_file_reader);

      void read_configuration(google::protobuf::Message* cfg);
      void process_configuration();

      private:
    
      // when we started (seconds since UNIX)
      double start_time_;

      // have we read the configuration file fully?
      bool configuration_read_;
      bool cout_cleared_;
    
      std::ofstream fout_;

      // allows direct reading of newest publish to a given MOOS variable
      goby::moos::DynamicMOOSVars dynamic_vars_;

      std::map<std::string, boost::shared_ptr<boost::signals2::signal<void (const CMOOSMsg& msg)> > > mail_handlers_;

      // CMOOSApp::OnConnectToServer()
      bool connected_;
      // CMOOSApp::OnStartUp()
      bool started_up_;

      std::deque<CMOOSMsg> msg_buffer_;
    
      // MOOS Variable name, blackout time
      std::deque<std::pair<std::string, int> > pending_subscriptions_;

      GobyMOOSAppConfig common_cfg_;

      bool ignore_stale_;
    
      static int argc_;
      static char** argv_;
      static std::string mission_file_;
      static std::string application_name_;
    };

class GobyMOOSApp : public GobyMOOSAppSelector<>
{
  public:
    template<typename ProtobufConfig>
        explicit GobyMOOSApp(ProtobufConfig* cfg) : GobyMOOSAppSelector<>(cfg) { }

};



template <class MOOSAppType>
    std::string GobyMOOSAppSelector<MOOSAppType>::mission_file_;

template <class MOOSAppType>
    std::string GobyMOOSAppSelector<MOOSAppType>::application_name_;

template <class MOOSAppType>
    int GobyMOOSAppSelector<MOOSAppType>::argc_ = 0;
template <class MOOSAppType>
    char** GobyMOOSAppSelector<MOOSAppType>::argv_ = 0;

template <class MOOSAppType>
    bool GobyMOOSAppSelector<MOOSAppType>::Iterate()
{
    MOOSAppType::Iterate();
    
    if(!configuration_read_) return true;

    // clear out MOOSApp cout for ncurses "scope" mode
    // MOOS has stopped talking by first Iterate()
    if(!cout_cleared_)
    {
        goby::glog.refresh();
        cout_cleared_ = true;
    }

    while(!msg_buffer_.empty() && (connected_ && started_up_))
    {
        goby::glog << "writing from buffer: " << msg_buffer_.front().GetKey() << ": " << msg_buffer_.front().GetAsString() << std::endl;
        MOOSAppType::m_Comms.Post(msg_buffer_.front());
        msg_buffer_.pop_front();
    }
    
    
    loop();
    return true;
}    

template <class MOOSAppType>
    bool GobyMOOSAppSelector<MOOSAppType>::OnNewMail(MOOSMSG_LIST &NewMail)
{
    // for AppCasting (otherwise no-op)
    MOOSAppType::OnNewMail(NewMail);
    
    for(MOOSMSG_LIST::const_iterator it = NewMail.begin(),
        end = NewMail.end(); it != end; ++it)
    {
        const CMOOSMsg& msg = *it;
        goby::glog.is(goby::common::logger::DEBUG3) && goby::glog << "Received mail: " << msg.GetKey() << ", time: " << std::setprecision(15) << msg.GetTime() << std::endl;        
        
        // update dynamic moos variables - do this inside the loop so the newest is
        // also the one referenced in the call to inbox()
        dynamic_vars().update_moos_vars(msg);   

        if(msg.GetTime() < start_time_ && ignore_stale_)
        {
            goby::glog.is(goby::common::logger::WARN) &&
                goby::glog << "ignoring normal mail from " << msg.GetKey()
                          << " from before we started (dynamics still updated)"
                          << std::endl;
        }
        else if(mail_handlers_.count(msg.GetKey()))
            (*mail_handlers_[msg.GetKey()])(msg);
    }
    
    return true;    
}

template <class MOOSAppType>
    bool GobyMOOSAppSelector<MOOSAppType>::OnConnectToServer()
{
    std::cout << MOOSAppType::m_MissionReader.GetAppName() << ", connected to server." << std::endl;
    connected_ = true;
    try_subscribing();


    for(google::protobuf::RepeatedPtrField<GobyMOOSAppConfig::Initializer>::const_iterator it = common_cfg_.initializer().begin(), end = common_cfg_.initializer().end(); it != end; ++it)
    {
        const GobyMOOSAppConfig::Initializer& ini = *it;
        if(ini.has_global_cfg_var())
        {
            std::string result;
            if(MOOSAppType::m_MissionReader.GetValue(ini.global_cfg_var(), result))
            {
                if(ini.type() == GobyMOOSAppConfig::Initializer::INI_DOUBLE)
                    publish(ini.moos_var(), goby::util::as<double>(result));
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

template <class MOOSAppType>
    bool GobyMOOSAppSelector<MOOSAppType>::OnStartUp()
{
    MOOSAppType::OnStartUp();

    std::cout << MOOSAppType::m_MissionReader.GetAppName () << ", starting ..." << std::endl;
    CMOOSApp::SetCommsFreq(common_cfg_.comm_tick());
    CMOOSApp::SetAppFreq(common_cfg_.app_tick());
    started_up_ = true;
    try_subscribing();
    return true;
}



template <class MOOSAppType>
    void GobyMOOSAppSelector<MOOSAppType>::subscribe(const std::string& var,  InboxFunc handler, int blackout /* = 0 */ )
{
    goby::glog.is(goby::common::logger::VERBOSE) &&
        goby::glog << "subscribing for MOOS variable: " << var << " @ " << blackout << std::endl;
    
    pending_subscriptions_.push_back(std::make_pair(var, blackout));
    try_subscribing();

    if(!mail_handlers_[var])
        mail_handlers_[var].reset(new boost::signals2::signal<void (const CMOOSMsg& msg)>);
        
    if(handler)
        mail_handlers_[var]->connect(handler);
}

template <class MOOSAppType>
    void GobyMOOSAppSelector<MOOSAppType>::try_subscribing()
{
    if (connected_ && started_up_)
        do_subscriptions();
}

template <class MOOSAppType>
    void GobyMOOSAppSelector<MOOSAppType>::do_subscriptions()
{
    MOOSAppType::RegisterVariables();

    while(!pending_subscriptions_.empty())
    {
        // variable name, blackout
        MOOSAppType::m_Comms.Register(pending_subscriptions_.front().first,
                         pending_subscriptions_.front().second);
        goby::glog.is(goby::common::logger::VERBOSE) &&
            goby::glog << "subscribed for: " << pending_subscriptions_.front().first << std::endl;
        pending_subscriptions_.pop_front();
    }
}


template <class MOOSAppType>
    void GobyMOOSAppSelector<MOOSAppType>::fetch_moos_globals(google::protobuf::Message* msg,
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

template <class MOOSAppType>
    void GobyMOOSAppSelector<MOOSAppType>::read_configuration(google::protobuf::Message* cfg)
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
        
        goby::glog.set_name(application_name_);
        goby::glog.add_stream(goby::common::protobuf::GLogConfig::VERBOSE, &std::cout);    
    
        std::string protobuf_text;
        std::ifstream fin;
        fin.open(mission_file_.c_str());
        if(fin.is_open())
        {
            std::string line;
            bool in_process_config = false;
            while(getline(fin, line))
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
                goby::glog.is(goby::common::logger::DIE) &&
                    goby::glog << "no ProcessConfig block for " << application_name_ << std::endl;
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
                goby::glog.is(goby::common::logger::DIE) && 
                    goby::glog << "fatal configuration errors (see above)" << std::endl;    
            }            
        }
        else
        {
            goby::glog.is(goby::common::logger::WARN) &&
                goby::glog << "failed to open " << mission_file_ << std::endl;
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



template <class MOOSAppType>
    void GobyMOOSAppSelector<MOOSAppType>::process_configuration()
{
    //
    // PROCESS CONFIGURATION
    //
    goby::glog.add_stream(common_cfg_.verbosity(), &std::cout);
    if(common_cfg_.show_gui())
    {
        goby::glog.enable_gui();
    }
    
    
    if(common_cfg_.log())
    {
        if(!common_cfg_.has_log_path())
        {
            goby::glog.is(goby::common::logger::WARN) &&
                goby::glog << "logging all terminal output to default directory (" << common_cfg_.log_path() << ")." << "set log_path for another path " << std::endl;
        }

        if(!common_cfg_.log_path().empty())
        {
            using namespace boost::posix_time;
            std::string file_name_base =
                boost::replace_all_copy(application_name_, "/", "_") + "_" +
                common_cfg_.community();

            std::string file_name = file_name_base + "_" +
                to_iso_string(second_clock::universal_time()) + ".txt";

            std::string file_symlink = file_name_base + "_latest.txt";
            
            goby::glog.is(goby::common::logger::VERBOSE) &&
                goby::glog << "logging output to file: " << file_name << std::endl;
            
            fout_.open(std::string(common_cfg_.log_path() + "/" + file_name).c_str());

            // symlink to "latest.txt"
            remove(std::string(common_cfg_.log_path() + "/" + file_symlink).c_str());
            symlink(file_name.c_str(), std::string(common_cfg_.log_path() + "/" + file_symlink).c_str());
        
            // if fails, try logging to this directory
            if(!fout_.is_open())
            {
                fout_.open(std::string("./" + file_name).c_str());
                goby::glog.is(goby::common::logger::WARN) &&
                    goby::glog << "logging to current directory because given directory is unwritable!" << std::endl;
            }
            // if still no go, quit
            if(!fout_.is_open())
            {
                
                goby::glog.is(goby::common::logger::DIE) && goby::glog << "cannot write to current directory, so cannot log." << std::endl;
            }
            
            goby::glog.add_stream(common_cfg_.log_verbosity(), &fout_);
        }
    }

}


// designed to run CMOOSApp derived applications
// using the MOOS "convention" of argv[1] == mission file, argv[2] == alternative name
template<typename App>
int goby::moos::run(int argc, char* argv[])
{
    App::argc_ = argc;
    App::argv_ = argv;

    try
    {
        App* app = App::get_instance();
        app->Run(App::application_name_.c_str(), App::mission_file_.c_str());
    }
    catch(goby::common::ConfigException& e)
    {
        // no further warning as the ApplicationBase Ctor handles this
        return 1;
    }
    catch(std::exception& e)
    {
        // some other exception
        goby::glog.is(goby::common::logger::DIE) &&  goby::glog << "uncaught exception: " << e.what() << std::endl;
        return 2;
    }

    return 0;
}



#endif
