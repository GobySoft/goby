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


TesMoosApp::TesMoosApp()
    : start_time_(MOOSTime()),
      configuration_read_(false),
      cout_cleared_(false)
{}

bool TesMoosApp::Iterate()
{
    if(!configuration_read_) return true;

    // clear out MOOSApp cout for ncurses "scope" mode
    // MOOS has stopped talking by first Iterate()
    if(!cout_cleared_)
    {
        logger().refresh();
        cout_cleared_ = true;
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

        if(msg.GetTime() < start_time_)
            logger() << warn << "ignoring normal mail from " << msg.GetKey()
                  << " from before we started (dynamics still updated)"
                  << std::endl;
        else if(mail_handlers_.count(msg.GetKey()))
            mail_handlers_[msg.GetKey()](msg);
        else
            inbox(msg);    
    }
    
    return true;    
}

bool TesMoosApp::readMissionParameters (CProcessConfigReader& config)
{
    logger().set_name(GetAppName());

    // if the user doesn't tell us otherwise, we're talking!
    std::string verbosity = "verbose";
    if (!config.GetConfigurationParam("verbosity", verbosity))
        logger() << warn << "verbosity not specified in configuration. using verbose mode." << std::endl;

    logger().add_stream(verbosity, &std::cout);
    
    bool log = true;
    config.GetConfigurationParam("log", log);
        
    if(log)
    {
        std::string log_path = ".";
        if(!config.GetValue("log_path", log_path) && log)
        {
            logger() << warn << "logging all terminal output to current directory (./). " << "setglobal value log_path for another path " << std::endl;
        }

        std::string community = "unknown";        
        config.GetValue("Community", community);
        
        if(!log_path.empty())
        {
            using namespace boost::posix_time;
            std::string file_name = GetAppName() + "_" + community + "_" + to_iso_string(second_clock::universal_time()) + ".txt";
            
            logger() << "logging output to file: " << file_name << std::endl;  
            
            fout_.open(std::string(log_path + "/" + file_name).c_str());
        
            // if fails, try logging to this directory
            if(!fout_.is_open())
            {
                fout_.open(std::string("./" + file_name).c_str());
                logger() << warn << "logging to current directory because given directory is unwritable!" << std::endl;
            }
            // if still no go, quit
            if(!fout_.is_open())
                logger() << die << "cannot write to current directory, so cannot log." << std::endl;

            logger().add_stream(goby::util::Logger::verbose, &fout_);
        }   
    }

    // virtual subclass method
    read_configuration(config);
    configuration_read_ = true;
    return true;    
}

void TesMoosApp::subscribe(const std::string& var,  InboxFunc handler /* = InboxFunc() */, int blackout /* = 0 */ )
{
    m_Comms.Register(var, blackout);
    
    mail_handlers_[var] = handler ? handler : boost::bind(&TesMoosApp::inbox, this, _1);
}


