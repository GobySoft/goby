// Copyright 2009-2014 Toby Schneider (https://launchpad.net/~tes)
//                     GobySoft, LLC (2013-)
//                     Massachusetts Institute of Technology (2007-2014)
//                     Goby Developers Team (https://launchpad.net/~goby-dev)
// 
//
// This file is part of the Goby Underwater Autonomy Project Binaries
// ("The Goby Binaries").
//
// The Goby Binaries are free software: you can redistribute them and/or modify
// them under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// The Goby Binaries are distributed in the hope that they will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Goby.  If not, see <http://www.gnu.org/licenses/>.

#include "ip_gateway_config.pb.h"
#include "goby/pb/application.h"

using namespace goby::common::logger;

namespace goby
{
    namespace acomms
    {
    
        class IPGateway : public goby::pb::Application
        {
        public:
            IPGateway(goby::acomms::protobuf::IPGatewayConfig* cfg);
        private:
            void loop();

    
        private:
            goby::acomms::protobuf::IPGatewayConfig& cfg_;
    
        };
    }
}



goby::acomms::IPGateway::IPGateway(goby::acomms::protobuf::IPGatewayConfig* cfg)
    : Application(cfg),
      cfg_(*cfg)
{
    
}

void goby::acomms::IPGateway::loop()
{
}


int main(int argc, char* argv[])
{
    goby::acomms::protobuf::IPGatewayConfig cfg;
    goby::run<goby::acomms::IPGateway>(argc, argv, &cfg);
}
                            
    
