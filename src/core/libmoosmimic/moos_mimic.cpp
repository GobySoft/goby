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


#include "moos_mimic.h"

GobyCMOOSApp::GobyCMOOSApp() 
    : m_Comms(*this),
      community_("#1")
{
    OnStartUp();
}

bool GobyCMOOSApp::Run(const char * sName,
                       const char * sMissionFile,
                       const char * sSubscribeName /*=0*/)
{
    std::cout << "run called" << std::endl;
            
    if(sMissionFile)
    {
        if(!m_MissionReader.SetFile(sMissionFile))
        {
            std::cerr <<  "mission file: " << sMissionFile << " not found" << std::endl;
        }
    }

    m_MissionReader.GetValue("Community", community_);
    
    GobyAppBase::set_application_name(sName);

    std::cout << "set loop freq" << std::endl;
    GobyAppBase::set_loop_freq(DEFAULT_MOOS_APP_FREQ_);

    std::cout << "start called" << std::endl;
    GobyAppBase::start();
    OnConnectToServer();
    GobyAppBase::run();
}


GobyCMOOSApp::CMOOSCommClient::CMOOSCommClient(GobyCMOOSApp& base)
    : base_(base)
{ }

bool GobyCMOOSApp::CMOOSCommClient::Notify(const std::string &sVar,
                                           const std::string & sVal,
                                           const std::string & sSrcAux,
                                           double dfTime/*=-1*/)
{
    CMOOSMsg Msg(MOOS_NOTIFY,sVar.c_str(),sVal.c_str(),dfTime);
    Msg.SetSourceAux(sSrcAux);
    Post(Msg);
}
        

bool GobyCMOOSApp::CMOOSCommClient::Notify(const std::string & sVar,
                                           double dfVal,
                                           const std::string & sSrcAux,
                                           double dfTime/*=-1*/)
{
    CMOOSMsg Msg(MOOS_NOTIFY,sVar.c_str(),dfVal,dfTime);
    Msg.SetSourceAux(sSrcAux);
    Post(Msg);
}

bool GobyCMOOSApp::CMOOSCommClient::Register(const std::string & sVar,double dfInterval)
{
    base_.GobyAppBase::subscribe(&GobyCMOOSApp::handle_incoming, &base_, sVar);
} 

bool GobyCMOOSApp::CMOOSCommClient::Post(CMOOSMsg& Msg)
{
    Msg.m_sSrc = base_.application_name();
    Msg.m_sOriginatingCommunity = base_.community_;
    
    base_.GobyAppBase::publish(moosmsg2proto_mimic(Msg));
    return true;
}


proto::CMOOSMsg GobyCMOOSApp::moosmsg2proto_mimic(const CMOOSMsg& in)
{
    proto::CMOOSMsg out;
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
    

CMOOSMsg GobyCMOOSApp::proto_mimic2moosmsg(const proto::CMOOSMsg& in)
{
    CMOOSMsg out;
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
