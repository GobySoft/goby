// copyright 2011 t. schneider tes@mit.edu
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


#ifndef MOOSDBOHELPER20110608H
#define MOOSDBOHELPER20110608H

#include "goby/core/libdbo/dbo_plugin.h"
#include "MOOSLIB/MOOSMsg.h"

namespace Wt
{
    namespace Dbo
    {
        template <>
        struct persist<CMOOSMsg>
        {
            template<typename A>
                static void apply(CMOOSMsg& msg, A& action)
            {
                std::string msg_type(1, msg.m_cMsgType);
                Wt::Dbo::field(action, msg_type, "msg_type");
                msg.m_cMsgType = msg_type[0];
                 
                std::string data_type(1, msg.m_cDataType);
                Wt::Dbo::field(action, data_type, "data_type");
                msg.m_cDataType = data_type[0];

                
                Wt::Dbo::field(action, msg.m_sKey, "key");
                Wt::Dbo::field(action, msg.m_nID, "moosmsg_id");
                Wt::Dbo::field(action, msg.m_dfTime, "moosmsg_time");
                Wt::Dbo::field(action, msg.m_dfVal, "double_value");
                Wt::Dbo::field(action, msg.m_sVal, "string_value");
                Wt::Dbo::field(action, msg.m_sSrc, "source");
                Wt::Dbo::field(action, msg.m_sSrcAux, "source_auxilary");
                Wt::Dbo::field(action, msg.m_sOriginatingCommunity, "originating_community");
            }
        };

    }
}

namespace goby
{
    namespace moos
    {
        class MOOSDBOPlugin : public goby::core::DBOPlugin
        {
            goby::core::MarshallingScheme provides()
            {
                return goby::core::MARSHALLING_MOOS;
            }
            
            void add_message(Wt::Dbo::Session* session, int unique_id, const std::string& identifier, const void* data, int size);
            void map_type(Wt::Dbo::Session* session);
        };
    }
}

extern "C" goby::core::DBOPlugin* create_goby_dbo_plugin();
extern "C" void destroy_goby_dbo_plugin(goby::core::DBOPlugin* plugin);


#endif
