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
    {
        if(connected_)
            m_Comms.Post(msg);
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
    tes::DynamicMOOSVars dynamic_vars_;

    std::map<std::string, boost::function<void (const CMOOSMsg& msg)> > mail_handlers_;

    // CMOOSApp::OnConnectToServer()
    bool connected_;
    // CMOOSApp::OnStartUp()
    bool started_up_;

    std::deque<CMOOSMsg> msg_buffer_;
    
    // MOOS Variable name, blackout time
    std::deque<std::pair<std::string, int> > pending_subscriptions_;

    TesMoosAppConfig common_cfg_;

    bool ignore_stale_;
    
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
    started_up_(false),
    ignore_stale_(true)
{
    using goby::glog;

    read_configuration(cfg);
    
    // keep a copy for ourselves
    common_cfg_ = cfg->common();
    configuration_read_ = true;

    process_configuration();

    glog.is(verbose) && glog << cfg->DebugString() << std::endl;
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
