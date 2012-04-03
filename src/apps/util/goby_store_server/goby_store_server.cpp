// Copyright 2009-2012 Toby Schneider (https://launchpad.net/~tes)
//                     Massachusetts Institute of Technology (2007-)
//                     Woods Hole Oceanographic Institution (2007-)
//                     Goby Developers Team (https://launchpad.net/~goby-dev)
// 
//
// This file is part of the Goby Underwater Autonomy Project Binaries
// ("The Goby Binaries").
//
// The Goby Binaries are free software: you can redistribute them and/or modify
// them under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// The Goby Binaries are distributed in the hope that they will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Goby.  If not, see <http://www.gnu.org/licenses/>.

#include "goby/common/zeromq_application_base.h"
#include "goby_store_server_config.pb.h"

namespace goby
{
    namespace util
    {
        class GobyStoreServer : public goby::common::ZeroMQApplicationBase
        {
        public:
            GobyStoreServer();
            ~GobyStoreServer() { }
            

        private:
            void loop();

        private:
            static goby::common::ZeroMQService zeromq_service_;
            static protobuf::GobyStoreServerConfig cfg_;
   
        };
    }
}

goby::common::ZeroMQService goby::util::GobyStoreServer::zeromq_service_;
goby::util::protobuf::GobyStoreServerConfig goby::util::GobyStoreServer::cfg_;


int main(int argc, char* argv[])
{
    goby::run<goby::util::GobyStoreServer>(argc, argv);
}

goby::util::GobyStoreServer::GobyStoreServer()
    : ZeroMQApplicationBase(&zeromq_service_, &cfg_)
{


}

void goby::util::GobyStoreServer::loop()
{
}
