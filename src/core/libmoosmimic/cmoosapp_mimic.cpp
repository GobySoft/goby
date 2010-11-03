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

#include <cstdio>
#include <cstring>

#include "cmoosapp_mimic.h"

goby::core::CMOOSApp::CMOOSApp() 
    : goby::core::ApplicationBase(0, true),
      m_Comms(*this),
      community_("#1")
{ }

bool goby::core::CMOOSApp::Run(const char * sName,
                               const char * sMissionFile,
                               const char * sSubscribeName /*=0*/)
{
    std::cout << "Starting goby CMOOSApp mimic: hold on to your hats." << std::endl;
    
    
    if(sSubscribeName)
        goby::core::ApplicationBase::set_application_name(sSubscribeName);
    else
        goby::core::ApplicationBase::set_application_name(sName);

    glogger().set_name(application_name());
            
    m_MissionReader.SetAppName(application_name());
    if(sMissionFile)
    {
        if(!m_MissionReader.SetFile(sMissionFile))
        {
            std::cerr <<  "mission file: " << sMissionFile << " not found" << std::endl;
        }
    }

    m_MissionReader.GetValue("Community", community_);
    goby::core::ApplicationBase::set_platform_name(community_);

    int freq = DEFAULT_MOOS_APP_FREQ_;
    m_MissionReader.GetConfigurationParam("AppTick", freq);

    std::string verbosity = "verbose";
    m_MissionReader.GetConfigurationParam("verbosity", verbosity);
    glogger().add_stream(verbosity, &std::cout);
        
    goby::core::ApplicationBase::set_loop_freq(freq);

    glogger() << "ApplicationBase::connect() called" << std::endl;
    goby::core::ApplicationBase::connect();
//    glogger() << "CMOOSApp::OnStartUp() called" << std::endl;

    std::cout << "onstartup" << std::endl;
    
    OnStartUp();

//    glogger() << "CMOOSApp::OnConnectToServer() called" << std::endl;

    std::cout << "onconnect" << std::endl;

    OnConnectToServer();
    
    glogger() << "AppBaseConfig: " << base_cfg_ << std::flush;

    goby::core::ApplicationBase::run();
    return true;
}

void goby::core::CMOOSApp::handle_incoming(const goby::core::proto::CMOOSMsgBase& heart)
{
    glogger() << warn << "******RECEIVED********" << heart << std::endl;
    
    goby::core::CMOOSMsg msg(heart);
    goby::core::MOOSMSG_LIST in_list(1, msg);
    OnNewMail(in_list);
}


        

// goby::core::proto::CMOOSMsg goby::core::CMOOSApp::moosmsg2proto_mimic(const CMOOSMsg& in)
// {
//     static goby::core::proto::CMOOSMsg out;
//     out.set_msg_type(in.m_cMsgType);
//     out.set_data_type(in.m_cDataType);
//     out.set_key(in.m_sKey);
//     out.set_msg_id(in.m_nID);
//     out.set_time(in.m_dfTime);
//     out.set_d_val(in.m_dfVal);
//     out.set_s_val(in.m_sVal);
//     out.set_src(in.m_sSrc);
//     out.set_src_aux(in.m_sSrcAux);
//     out.set_originating_community(in.m_sOriginatingCommunity);
//     return out;
// }
    

// CMOOSMsg goby::core::CMOOSApp::proto_mimic2moosmsg(const goby::core::proto::CMOOSMsg& in)
// {
//     static CMOOSMsg out;
//     out.m_cMsgType = in.msg_type();
//     out.m_cDataType = in.data_type();
//     out.m_sKey = in.key();
//     out.m_nID = in.msg_id();
//     out.m_dfTime = in.time();
//     out.m_dfVal = in.d_val();
//     out.m_sVal = in.s_val();
//     out.m_sSrc = in.src();
//     out.m_sSrcAux = in.src_aux();
//     out.m_sOriginatingCommunity = in.originating_community();
//     return out;
// }
