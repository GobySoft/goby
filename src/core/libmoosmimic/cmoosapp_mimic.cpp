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
    : m_Comms(*this),
      community_("#1")
{ }

bool goby::core::CMOOSApp::Run(const char * sName,
                       const char * sMissionFile,
                       const char * sSubscribeName /*=0*/)
{
    std::cout << "run called" << std::endl;
    goby::core::ApplicationBase::set_application_name(sName);
            
    if(sMissionFile)
    {
        if(!m_MissionReader.SetFile(sMissionFile))
        {
            std::cerr <<  "mission file: " << sMissionFile << " not found" << std::endl;
        }
    }

    m_MissionReader.SetAppName(sName);
    m_MissionReader.GetValue("Community", community_);

    int freq = DEFAULT_MOOS_APP_FREQ_;
    m_MissionReader.GetConfigurationParam("AppTick", freq);
    goby::core::ApplicationBase::set_loop_freq(freq);

    std::cout << "start called" << std::endl;
    goby::core::ApplicationBase::connect();
    std::cout << "OnStartUp called" << std::endl;
    OnStartUp();
    OnConnectToServer();
 
    goby::core::ApplicationBase::run();

    return true;
}


goby::core::CMOOSApp::CMOOSCommClient::CMOOSCommClient(goby::core::CMOOSApp& base)
    : base_(base)
{ }

bool goby::core::CMOOSApp::CMOOSCommClient::Notify(const std::string &sVar,
                                           const std::string & sVal,
                                           const std::string & sSrcAux,
                                           double dfTime/*=-1*/)
{
    CMOOSMsg Msg(MOOS_NOTIFY,sVar.c_str(),sVal.c_str(),dfTime);
    Msg.SetSourceAux(sSrcAux);
    return Post(Msg);
}
        

bool goby::core::CMOOSApp::CMOOSCommClient::Notify(const std::string & sVar,
                                           double dfVal,
                                           const std::string & sSrcAux,
                                           double dfTime/*=-1*/)
{
    CMOOSMsg Msg(MOOS_NOTIFY,sVar.c_str(),dfVal,dfTime);
    Msg.SetSourceAux(sSrcAux);
    return Post(Msg);
}

bool goby::core::CMOOSApp::CMOOSCommClient::Register(const std::string & sVar,double dfInterval)
{
    base_.goby::core::ApplicationBase::subscribe(
        &goby::core::CMOOSApp::handle_incoming,
        &base_,
        make_filter("key", Filter::EQUAL, sVar));

    return true;
}

bool goby::core::CMOOSApp::CMOOSCommClient::Post(CMOOSMsg& Msg)
{
    Msg.m_sSrc = base_.application_name();
    Msg.m_sOriginatingCommunity = base_.community_;
    
    base_.goby::core::ApplicationBase::publish(moosmsg2proto_mimic(Msg));
    return true;
}

goby::core::proto::CMOOSMsg goby::core::CMOOSApp::moosmsg2proto_mimic(const CMOOSMsg& in)
{
    static goby::core::proto::CMOOSMsg out;
    out.set_msg_type(in.m_cMsgType);
    out.set_data_type(in.m_cDataType);
    out.set_key(in.m_sKey);
    out.set_msg_id(in.m_nID);
    out.set_time(in.m_dfTime);
    out.set_d_val(in.m_dfVal);
    out.set_s_val(in.m_sVal);
    out.set_src(in.m_sSrc);
    out.set_src_aux(in.m_sSrcAux);
    out.set_originating_community(in.m_sOriginatingCommunity);
    return out;
}
    

CMOOSMsg goby::core::CMOOSApp::proto_mimic2moosmsg(const goby::core::proto::CMOOSMsg& in)
{
    static CMOOSMsg out;
    out.m_cMsgType = in.msg_type();
    out.m_cDataType = in.data_type();
    out.m_sKey = in.key();
    out.m_nID = in.msg_id();
    out.m_dfTime = in.time();
    out.m_dfVal = in.d_val();
    out.m_sVal = in.s_val();
    out.m_sSrc = in.src();
    out.m_sSrcAux = in.src_aux();
    out.m_sOriginatingCommunity = in.originating_community();
    return out;
}
