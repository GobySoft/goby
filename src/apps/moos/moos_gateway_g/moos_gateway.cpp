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

#include "goby/moos/moos_node.h"
#include "goby/util/logger.h"
#include "goby/pb/zeromq_application_base.h"
#include "goby/pb/pubsub_node_wrapper.h"
#include "goby/util/logger/term_color.h"

#include "moos_gateway_config.pb.h"

using namespace goby::util::logger;

namespace goby
{
    namespace moos
    {
        class MOOSGateway : public goby::core::ZeroMQApplicationBase, public MOOSNode
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
            static goby::core::ZeroMQService zeromq_service_;
            goby::core::PubSubNodeWrapper<CMOOSMsg> goby_moos_pubsub_client_;
            CMOOSCommClient moos_client_;
            
            enum { MAX_CONNECTION_TIMEOUT = 10 };
            
            static protobuf::MOOSGatewayConfig cfg_;

            std::set<std::string> subscribed_vars_;
        };
    }
}

goby::core::ZeroMQService goby::moos::MOOSGateway::zeromq_service_;
goby::moos::protobuf::MOOSGatewayConfig goby::moos::MOOSGateway::cfg_;

int main(int argc, char* argv[])
{
    goby::run<goby::moos::MOOSGateway>(argc, argv);
}


using goby::glog;

goby::moos::MOOSGateway::MOOSGateway()
    : ZeroMQApplicationBase(&zeromq_service_, &cfg_),
      MOOSNode(&zeromq_service_),
      goby_moos_pubsub_client_(this, cfg_.base().pubsub_config())
{
    moos_client_.Run(cfg_.moos_server_host().c_str(), cfg_.moos_server_port(), cfg_.base().app_name().c_str(), cfg_.moos_comm_tick());

    glog.is(VERBOSE) &&
        glog << "Waiting to connect to MOOSDB ... " << std::endl;

    int i = 0;
    while(!moos_client_.IsConnected())
    {
        i++;
        if(i > MAX_CONNECTION_TIMEOUT)
        {
            glog.is(DIE) &&
                glog << "Failed to connect to MOOSDB in " << MAX_CONNECTION_TIMEOUT << " seconds. Check `moos_server_host` and `moos_server_port`" << std::endl;
        }
        
        sleep(1);
    }

    for(int i = 0, n = cfg_.goby_subscribe_filter_size(); i < n; ++i)
        goby_moos_pubsub_client_.subscribe(cfg_.goby_subscribe_filter(i));


    glog.add_group("from_moos", util::Colors::lt_magenta, "MOOS -> Goby");
    glog.add_group("to_moos", util::Colors::lt_green, "Goby -> MOOS");
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

    glog.is(VERBOSE) &&
        glog << group("to_moos") << msg << std::endl;
    
    moos_client_.Post(msg);
}

    

// from MinimalApplicationBase
void goby::moos::MOOSGateway::loop()
{
    check_for_new_moos_variables();
    std::list<CMOOSMsg> moos_msgs;
    
    moos_client_.Fetch(moos_msgs);

    BOOST_FOREACH(CMOOSMsg& msg, moos_msgs)
    {
        // we wrote this, so ignore
        if(msg.GetSourceAux().find(application_name()) != std::string::npos)
            continue;

        msg.SetSourceAux(msg.GetSourceAux() +
                         (msg.GetSourceAux().size() ? "/" : "")
                         + application_name());

        glog.is(VERBOSE) &&
            glog << group("from_moos") << msg << std::endl;    
        
        goby_moos_pubsub_client_.publish(msg);
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
        if(moos_client_.ServerRequest("VAR_SUMMARY", InMail, 2.0, false))
        {
            if(InMail.size()!=1)
            {
                glog.is(WARN) &&
                    glog << "ServerRequest for VAR_SUMMARY returned incorrect mail size (should be one)";
                return;
            }
            
            std::vector<std::string> all_var;
            std::string var_str(InMail.begin()->GetString());
            boost::split(all_var, var_str, boost::is_any_of(","));

            BOOST_FOREACH(const std::string& s, all_var)
            {
                glog.is(DEBUG1) &&
                    glog << s << std::endl;
                if(!subscribed_vars_.count(s))
                {
                    if(clears_subscribe_filters(s))
                    {
                        glog.is(VERBOSE) &&
                            glog << "moos_client_.Register for " << s << std::endl;
                        moos_client_.Register(s, 0);
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
    
