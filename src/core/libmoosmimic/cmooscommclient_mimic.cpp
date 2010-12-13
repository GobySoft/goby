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

#include "goby/util/time.h"
#include "cmoosapp_mimic.h"

goby::core::CMOOSCommClient::CMOOSCommClient(goby::core::CMOOSApp& base)
    : base_(base)
{ }

bool goby::core::CMOOSCommClient::Notify(const std::string &sVar,
                                           const std::string & sVal,
                                           const std::string & sSrcAux,
                                           double dfTime/*=-1*/)
{
    goby::core::CMOOSMsg msg;
    msg.mutable_heart()->set_key(sVar);
    msg.mutable_heart()->set_s_val(sVal);
    msg.mutable_heart()->set_time(dfTime);
    msg.mutable_heart()->set_src_aux(sSrcAux);

    return Post(msg);
}
        

bool goby::core::CMOOSCommClient::Notify(const std::string & sVar,
                                           double dfVal,
                                           const std::string & sSrcAux,
                                           double dfTime/*=-1*/)
{
    goby::core::CMOOSMsg msg;
    msg.mutable_heart()->set_key(sVar);
    msg.mutable_heart()->set_d_val(dfVal);
    msg.mutable_heart()->set_time(dfTime);
    msg.mutable_heart()->set_src_aux(sSrcAux);

    return Post(msg);
}

bool goby::core::CMOOSCommClient::Register(const std::string & sVar,double dfInterval)
{
    base_.goby::core::ApplicationBase::subscribe<goby::core::proto::CMOOSMsgBase>(
        &goby::core::CMOOSApp::handle_incoming,
        &base_,
        goby::core::ApplicationBase::make_filter("key", goby::core::proto::Filter::EQUAL, sVar));
    
    return true;
}

bool goby::core::CMOOSCommClient::Post(goby::core::CMOOSMsg& msg)
{
    msg.mutable_heart()->set_msg_type('N');
    if(msg.heart().has_d_val())
        msg.mutable_heart()->set_data_type('D');
    else
        msg.mutable_heart()->set_data_type('S');
    
    msg.mutable_heart()->set_src(base_.application_name());
    msg.mutable_heart()->set_originating_community(base_.community_);

    if(msg.heart().time() == -1)
        msg.mutable_heart()->set_time(goby::util::ptime2unix_double(goby::util::goby_time()));
    
    base_.goby::core::ApplicationBase::publish<goby::core::proto::CMOOSMsgBase>(*msg.mutable_heart());
    return true;
}


bool goby::core::CMOOSCommClient::IsConnected()
{ return base_.ApplicationBase::connected(); }
        

