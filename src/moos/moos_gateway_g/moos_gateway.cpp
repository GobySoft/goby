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

#include "goby/core/libcore/zero_mq_node.h"
#include "goby/core/libcore/minimal_application_base.h"
#include "goby/util/logger.h"

#include "moos_gateway_config.pb.h"

namespace goby
{
    namespace moos
    {
        class MOOSGateway : public goby::core::MinimalApplicationBase,
                            public goby::core::ZeroMQNode,
                            public CMOOSCommClient
        {
        public:
            MOOSGateway();
            ~MOOSGateway();

        private:
            // from ZeroMQNode
            void inbox(core::MarshallingScheme marshalling_scheme,
                       const std::string& identifier,
                       const void* data,
                       int size);
    

            // from MinimalApplicationBase
            void iterate();

        private:
            static protobuf::MOOSGatewayConfig cfg_;
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
    : goby::core::MinimalApplicationBase(&cfg_),
      goby::core::ZeroMQNode(&glogger())
{
    
    
}


goby::moos::MOOSGateway::~MOOSGateway()
{
}

void goby::moos::MOOSGateway::inbox(core::MarshallingScheme marshalling_scheme,
                                    const std::string& identifier,
                                    const void* data,
                                    int size)
{
}

    

// from MinimalApplicationBase
void goby::moos::MOOSGateway::iterate()
{
    sleep(1);
}

