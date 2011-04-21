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
#include "goby/core/libcore/configuration_reader.h"
#include "goby/core/libcore/exception.h"
#include "moos_protobuf_helpers.h"
#include "goby/version.h"

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

    
    
    static int argc_;
    static char** argv_;
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
            goby::ConfigException e("");
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
        
        glogger().set_name(application_name_);
        glogger().add_stream("verbose", &std::cout);
    
    
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
                if(boost::algorithm::istarts_with(no_blanks_line, "PROCESSCONFIG=" +  application_name_))
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
                glogger() << die << "no ProcessConfig block for " << application_name_ << std::endl;

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
            glogger() << warn << "failed to open " << mission_file_ << std::endl;
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
            throw(goby::ConfigException(err_msg.str()));
        }
        
    }
    catch(goby::ConfigException& e)
    {
        // output all the available command line options
        std::cerr << od_all << "\n";
        if(e.error())
            std::cerr << "Problem parsing command-line configuration: \n"
                      << e.what() << "\n";
        
        throw;
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


    glogger() << cfg->DebugString() << std::endl;
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
    catch(goby::ConfigException& e)
    {
        // no further warning as the ApplicationBase Ctor handles this
        return 1;
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
