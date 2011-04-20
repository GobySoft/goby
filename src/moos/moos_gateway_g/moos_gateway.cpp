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

#include "MOOSLIB/MOOSCommClient.h"

#include "goby/moos/libmoos_util/moos_node.h"
#include "goby/util/logger.h"
#include "goby/core/libcore/zeromq_application_base.h"
#include "goby/util/liblogger/term_color.h"

#include "moos_gateway_config.pb.h"

namespace goby
{
    namespace moos
    {
        class MOOSGateway : public goby::core::ZeroMQApplicationBase, public MOOSNode, public CMOOSCommClient
        {
        public:
            MOOSGateway();
            ~MOOSGateway();

        private:
            void moos_inbox(CMOOSMsg& msg);
            void loop();
            
            void check_for_new_moos_variables();
            bool clears_subscribe_filters(const std::string& moos_variable);
            
            
        private:
            enum { MAX_CONNECTION_TIMEOUT = 10 };
            
            static protobuf::MOOSGatewayConfig cfg_;

            std::set<std::string> subscribed_vars_;
        };
    }
}

goby::moos::protobuf::MOOSGatewayConfig goby::moos::MOOSGateway::cfg_;

int main(int argc, char* argv[])
{
    goby::run<goby::moos::MOOSGateway>(argc, argv);
}


using goby::util::glogger;

goby::moos::MOOSGateway::MOOSGateway()
    : ZeroMQApplicationBase(&cfg_)
{
    CMOOSCommClient::Run(cfg_.moos_server_host().c_str(), cfg_.moos_server_port(), cfg_.base().app_name().c_str(), cfg_.moos_comm_tick());
    
    glogger() << "Waiting to connect to MOOSDB ... " << std::endl;

    int i = 0;
    while(!CMOOSCommClient::IsConnected())
    {
        i++;
        if(i > MAX_CONNECTION_TIMEOUT)
            glogger() << die << "Failed to connect to MOOSDB in " << MAX_CONNECTION_TIMEOUT << " seconds. Check `moos_server_host` and `moos_server_port`" << std::endl;
        sleep(1);
    }

    for(int i = 0, n = cfg_.goby_subscribe_filter_size(); i < n; ++i)
        MOOSNode::subscribe(cfg_.goby_subscribe_filter(i));


    glogger().add_group("from_moos", util::Colors::lt_magenta, "MOOS -> Goby");
    glogger().add_group("to_moos", util::Colors::lt_green, "Goby -> MOOS");
}


goby::moos::MOOSGateway::~MOOSGateway()
{
}


void goby::moos::MOOSGateway::moos_inbox(CMOOSMsg& msg)
{
    // we wrote this, so ignore
    if(msg.GetSourceAux().find(application_name()) != std::string::npos)
        return;
    
    // identify us as the writer
    msg.SetSourceAux(msg.GetSourceAux() +
                     (msg.GetSourceAux().size() ? "/" : "")
                     + application_name());
    
    glogger() << group("to_moos") << msg << std::endl;
    
    CMOOSCommClient::Post(msg);
}

    

// from MinimalApplicationBase
void goby::moos::MOOSGateway::loop()
{
    check_for_new_moos_variables();
    std::list<CMOOSMsg> moos_msgs;
    
    CMOOSCommClient::Fetch(moos_msgs);

    BOOST_FOREACH(CMOOSMsg& msg, moos_msgs)
    {
        // we wrote this, so ignore
        if(msg.GetSourceAux().find(application_name()) != std::string::npos)
            continue;

        msg.SetSourceAux(msg.GetSourceAux() +
                         (msg.GetSourceAux().size() ? "/" : "")
                         + application_name());

        glogger() << group("from_moos") << msg << std::endl;    

        MOOSNode::publish(msg);
    }
    
    
}

// adapted from CMOOSLogger::HandleWildCardLogging
// as MOOS has no "accepted" way of handling wild card subscriptions
void goby::moos::MOOSGateway::check_for_new_moos_variables()
{
    const double DEFAULT_WILDCARD_TIME = 1.0;
    static double dfLastWildCardTime = -1.0;
    if(MOOSTime()-dfLastWildCardTime > DEFAULT_WILDCARD_TIME)
    {
        MOOSMSG_LIST InMail;
        if(CMOOSCommClient::ServerRequest("VAR_SUMMARY", InMail, 2.0, false))
        {
            if(InMail.size()!=1)
            {
                glogger() << warn << "ServerRequest for VAR_SUMMARY returned incorrect mail size (should be one)";
                return;
            }
            
            std::vector<std::string> all_var;
            std::string var_str(InMail.begin()->GetString());
            boost::split(all_var, var_str, boost::is_any_of(","));

            BOOST_FOREACH(const std::string& s, all_var)
            {
                glogger() << s << std::endl;
                if(!subscribed_vars_.count(s))
                {
                    if(clears_subscribe_filters(s))
                    {
                        glogger() << "CMOOSCommClient::Register for " << s << std::endl;
                        CMOOSCommClient::Register(s, 0);
                        subscribed_vars_.insert(s);
                    }
                }
            }
            
            dfLastWildCardTime = MOOSTime();
        }
    }
}

bool goby::moos::MOOSGateway::clears_subscribe_filters(const std::string& moos_variable)
{
    for(int i = 0, n = cfg_.moos_subscribe_filter_size(); i < n; ++i)
    {
        if(moos_variable.compare(0, cfg_.moos_subscribe_filter(i).size(), cfg_.moos_subscribe_filter(i)) == 0)
            return true;
    }
    return false;    
}
    
