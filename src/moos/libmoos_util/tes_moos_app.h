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

#ifndef TESMOOSAPP20100726H
#define TESMOOSAPP20100726H

#include "MOOSLIB/MOOSApp.h"

#include <boost/date_time/posix_time/posix_time.hpp>
#include <map>
#include <boost/function.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/bind.hpp>
#include <boost/filesystem.hpp>

#include "dynamic_moos_vars.h"
#include "goby/util/logger.h"
#include "tes_moos_app.pb.h"


namespace goby
{
    namespace moos
    {
        template<typename App>
            int run(int argc, char* argv[]);
    }
}



class TesMoosApp : public CMOOSApp
{   
  protected:
    typedef boost::function<void (const CMOOSMsg& msg)> InboxFunc;

    template<typename ProtobufConfig>
        explicit TesMoosApp(ProtobufConfig* cfg);
    
  
    virtual ~TesMoosApp() { }    
  
    void publish(CMOOSMsg& msg)
    { m_Comms.Post(msg); }
    
    template<typename T>
        void publish(const std::string& key, const T& value)
    { m_Comms.Notify(key, value); }
    
    tes::DynamicMOOSVars& dynamic_vars() { return dynamic_vars_; }
    double start_time() const { return start_time_; }

    void subscribe(const std::string& var,
                   InboxFunc handler,
                   int blackout = 0);    
        
    template<typename V, typename A1>
        void subscribe(const std::string& var,
                       void(V::*mem_func)(A1),
                       V* obj,
                       int blackout = 0)
    { subscribe(var, boost::bind(mem_func, obj, _1), blackout); }    

    
    
    template<typename App>
        friend int ::goby::moos::run(int argc, char* argv[]);
    
    virtual void loop() = 0;
    virtual void inbox(const CMOOSMsg& msg) { }    
    
  private:
    // from CMOOSApp
    bool Iterate();
    bool OnStartUp();
    bool OnConnectToServer();
    bool OnNewMail(MOOSMSG_LIST &NewMail);
    void try_subscribing();
    void do_subscriptions();
    

  private:
    
    // when we started (seconds since UNIX)
    double start_time_;

    // have we read the configuration file fully?
    bool configuration_read_;
    bool cout_cleared_;
    
    std::ofstream fout_;

    // allows direct reading of newest publish to a given MOOS variable
    tes::DynamicMOOSVars dynamic_vars_;

    std::map<std::string, boost::function<void (const CMOOSMsg& msg)> > mail_handlers_;

    // CMOOSApp::OnConnectToServer()
    bool connected_;
    // CMOOSApp::OnStartUp()
    bool started_up_;

    // MOOS Variable name, blackout time
    std::deque<std::pair<std::string, int> > pending_subscriptions_;

    TesMoosAppConfig common_cfg_;
    
    static std::string mission_file_;
    static std::string application_name_;
};


template<typename ProtobufConfig>
TesMoosApp::TesMoosApp(ProtobufConfig* cfg)
: start_time_(MOOSTime()),
    configuration_read_(false),
    cout_cleared_(false),
    connected_(false),
    started_up_(false)
{
    using goby::util::glogger;
    
    glogger().set_name(application_name_);
    glogger().add_stream("verbose", &std::cout);

    //
    // READ CONFIGURATION
    //
    std::string protobuf_text;
    std::ifstream fin;
    fin.open(mission_file_.c_str());
    if(fin.is_open())
    {
        std::string line;
        bool in_process_config = false;
        while(!getline(fin, line).eof())
        {
            if(!boost::algorithm::ifind_first(line, "PROCESSCONFIG").empty() &&
               !boost::algorithm::ifind_first(line, application_name_).empty())
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
            glogger() << die << "fatal configuration errors (see above)" << std::endl;    
        
    }
    else
    {
        std::cerr << "failed to open " << mission_file_ << std::endl;
    }
    
    fin.close();

    CMOOSFileReader moos_file_reader;
    moos_file_reader.SetFile(mission_file_);

    const google::protobuf::Descriptor* desc = cfg->common().GetDescriptor();
    const google::protobuf::Reflection* refl = cfg->common().GetReflection();
    for(int i = 0, n = desc->field_count(); i < n; ++i)
    {
        const google::protobuf::FieldDescriptor* field_desc = desc->field(i);
        
        std::string moos_global = field_desc->options().GetExtension(::moos_global);
        
        switch(field_desc->cpp_type())
        {
            case google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE:
                break;    
                    
            case google::protobuf::FieldDescriptor::CPPTYPE_INT32:
            {
                int result;
                if(moos_file_reader.GetValue(moos_global, result))
                    refl->SetInt32(cfg->mutable_common(), field_desc, result);
                break;
            }
            
            case google::protobuf::FieldDescriptor::CPPTYPE_INT64:
            {
                int result;
                if(moos_file_reader.GetValue(moos_global, result))
                    refl->SetInt64(cfg->mutable_common(), field_desc, result);
                break;
            }

            case google::protobuf::FieldDescriptor::CPPTYPE_UINT32:
            {
                unsigned result;
                if(moos_file_reader.GetValue(moos_global, result))
                    refl->SetUInt32(cfg->mutable_common(), field_desc, result);
                break;
            }

            case google::protobuf::FieldDescriptor::CPPTYPE_UINT64:
            {
                unsigned result;
                if(moos_file_reader.GetValue(moos_global, result))
                    refl->SetUInt64(cfg->mutable_common(), field_desc, result);
                break;
            }
                        
            case google::protobuf::FieldDescriptor::CPPTYPE_BOOL:
            {
                bool result;
                if(moos_file_reader.GetValue(moos_global, result))
                    refl->SetBool(cfg->mutable_common(), field_desc, result);
                break;
            }
                    
            case google::protobuf::FieldDescriptor::CPPTYPE_STRING:
            {
                std::string result;
                if(moos_file_reader.GetValue(moos_global, result))
                    refl->SetString(cfg->mutable_common(), field_desc, result);
                break;
            }
                
            case google::protobuf::FieldDescriptor::CPPTYPE_FLOAT:
            {
                float result;
                if(moos_file_reader.GetValue(moos_global, result))
                    refl->SetFloat(cfg->mutable_common(), field_desc, result);
                break;
            }

            case google::protobuf::FieldDescriptor::CPPTYPE_DOUBLE:
            {
                double result;
                if(moos_file_reader.GetValue(moos_global, result))
                    refl->SetDouble(cfg->mutable_common(), field_desc, result);
                break;
            }
                
            case google::protobuf::FieldDescriptor::CPPTYPE_ENUM:
            {
                std::string result;
                if(moos_file_reader.GetValue(moos_global, result))
                {                
                    const google::protobuf::EnumValueDescriptor* enum_desc =
                        refl->GetEnum(cfg->common(), field_desc)->type()->FindValueByName(result);
                    if(!enum_desc)
                        throw(std::runtime_error(std::string("invalid enumeration " +
                                                             result + " for field " +
                                                             field_desc->name())));
                    
                    refl->SetEnum(cfg->mutable_common(), field_desc, enum_desc);
                }
                break;
            }
        }
    }    

    // keep a copy for ourselves
    common_cfg_ = cfg->common();
    configuration_read_ = true;
    

    //
    // PROCESS CONFIGURATION
    //
    switch(cfg->common().verbosity())
    {
        case TesMoosAppConfig::VERBOSITY_VERBOSE:
            glogger().add_stream(goby::util::Logger::verbose, &std::cout);
            break;
        case TesMoosAppConfig::VERBOSITY_WARN:
            glogger().add_stream(goby::util::Logger::warn, &std::cout);
            break;
        case TesMoosAppConfig::VERBOSITY_DEBUG:
            glogger().add_stream(goby::util::Logger::debug, &std::cout);
            break;
        case TesMoosAppConfig::VERBOSITY_GUI:
            glogger().add_stream(goby::util::Logger::gui, &std::cout);
            break;
        case TesMoosAppConfig::VERBOSITY_QUIET:
            glogger().add_stream(goby::util::Logger::quiet, &std::cout);
            break;
    }    


    if(cfg->common().log())
    {
        if(!cfg->common().has_log_path())
        {
            glogger() << warn << "logging all terminal output to default directory (" << cfg->common().log_path() << ")." << "set log_path for another path " << std::endl;
        }

        if(!cfg->common().log_path().empty())
        {
            using namespace boost::posix_time;
            std::string file_name = application_name_ + "_" + cfg->common().community() + "_" + to_iso_string(second_clock::universal_time()) + ".txt";
            
            glogger() << "logging output to file: " << file_name << std::endl;
            
            fout_.open(std::string(cfg->common().log_path() + "/" + file_name).c_str());
        
            // if fails, try logging to this directory
            if(!fout_.is_open())
            {
                fout_.open(std::string("./" + file_name).c_str());
                glogger() << warn << "logging to current directory because given directory is unwritable!" << std::endl;
            }
            // if still no go, quit
            if(!fout_.is_open())
                glogger() << die << "cannot write to current directory, so cannot log." << std::endl;

            glogger().add_stream(goby::util::Logger::verbose, &fout_);
        }
    }

}


// designed to run CMOOSApp derived applications
// using the MOOS "convention" of argv[1] == mission file, argv[2] == alternative name
template<typename App>
int goby::moos::run(int argc, char* argv[])
{
    boost::filesystem::path launch_path(argv[0]);
    App::application_name_ = launch_path.filename();
    App::mission_file_ = App::application_name_ + ".moos";

    switch(argc)
    {
        case 3:
            // command line says don't register with default name
            App::application_name_ = argv[2];
        case 2:
            // command line says don't use default config file
            App::mission_file_ = argv[1];
    }
    
    try
    {
        App app;
        app.Run(App::application_name_.c_str(), App::mission_file_.c_str());
    }
    catch(std::exception& e)
    {
        // some other exception
        std::cerr << "uncaught exception: " << e.what() << std::endl;
        return 2;
    }

    return 0;
}



#endif
