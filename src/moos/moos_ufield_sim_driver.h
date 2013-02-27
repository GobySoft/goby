// Copyright 2009-2013 Toby Schneider (https://launchpad.net/~tes)
//                     Massachusetts Institute of Technology (2007-)
//                     Woods Hole Oceanographic Institution (2007-)
//                     Goby Developers Team (https://launchpad.net/~goby-dev)
// 
//
// This file is part of the Goby Underwater Autonomy Project Libraries
// ("The Goby Libraries").
//
// The Goby Libraries are free software: you can redistribute them and/or modify
// them under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// The Goby Libraries are distributed in the hope that they will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with Goby.  If not, see <http://www.gnu.org/licenses/>.


#ifndef UFieldSimDriver20120214H
#define UFieldSimDriver20120214H

#include "MOOS/libMOOS/Comms/MOOSCommClient.h"

//#include <boost/bimap.hpp>

#include "goby/common/time.h"

#include "goby/acomms/modemdriver/driver_base.h"
#include "goby/acomms/acomms_helpers.h"

#include "goby/moos/modem_id_convert.h"

namespace goby
{
    namespace moos
    {
        /// \brief provides an simulator driver to the uFldNodeComms MOOS module: http://oceanai.mit.edu/moos-ivp/pmwiki/pmwiki.php?n=Modules.UFldNodeComms
        /// \ingroup acomms_api
        /// 
        class UFldDriver : public goby::acomms::ModemDriverBase
        {
          public:
            UFldDriver();
            void startup(const goby::acomms::protobuf::DriverConfig& cfg);
            void shutdown();            
            void do_work();
            void handle_initiate_transmission(const goby::acomms::protobuf::ModemTransmission& m);

          private:
            void send_message(const goby::acomms::protobuf::ModemTransmission& msg);
            void receive_message(const goby::acomms::protobuf::ModemTransmission& msg);
            
          private:
            enum { DEFAULT_PACKET_SIZE=64 };
            CMOOSCommClient moos_client_;
            goby::acomms::protobuf::DriverConfig driver_cfg_; // configuration given to you at launch

            //boost::bimap<int, std::string> modem_id2name_;
            goby::moos::ModemIdConvert modem_lookup_;
        };
    }
}
#endif
