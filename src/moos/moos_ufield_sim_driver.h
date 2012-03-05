// copyright 2011 t. schneider tes@mit.edu
//
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

#ifndef UFieldSimDriver20120214H
#define UFieldSimDriver20120214H

#include "MOOSLIB/MOOSCommClient.h"

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
            tes::ModemIdConvert modem_lookup_;
        };
    }
}
#endif
