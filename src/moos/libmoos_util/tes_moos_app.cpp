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

using goby::util::glogger;
using goby::util::as;

std::string TesMoosApp::mission_file_;
std::string TesMoosApp::application_name_;

bool TesMoosApp::Iterate()
{
    if(!configuration_read_) return true;

    // clear out MOOSApp cout for ncurses "scope" mode
    // MOOS has stopped talking by first Iterate()
    if(!cout_cleared_)
    {
        glogger().refresh();
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
            glogger() << warn << "ignoring normal mail from " << msg.GetKey()
                  << " from before we started (dynamics still updated)"
                  << std::endl;
        else if(mail_handlers_.count(msg.GetKey()))
            mail_handlers_[msg.GetKey()](msg);
        else
            glogger() << die << "received mail that we have no handler for!" << std::endl;
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


void TesMoosApp::subscribe(const std::string& var,  InboxFunc handler /* = InboxFunc() */, int blackout /* = 0 */ )
{
    pending_subscriptions_.push_back(std::make_pair(var, blackout));
    try_subscribing();
    mail_handlers_[var] = handler ? handler : boost::bind(&TesMoosApp::inbox, this, _1);
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
        pending_subscriptions_.pop_front();
    }
}
