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

#ifndef Modem20110225H
#define Modem20110225H

#include "goby/util/time.h"

#include "driver_base.h"
#include "goby/protobuf/abc_driver.pb.h"
#include "goby/acomms/acomms_helpers.h"

namespace goby
{
    namespace acomms
    {
        class ABCDriver : public ModemDriverBase
        {
          public:
            ABCDriver(std::ostream* log = 0);
            void startup(const protobuf::DriverConfig& cfg);
            void shutdown();            
            void do_work();
            void handle_initiate_transmission(protobuf::ModemMsgBase* m);

          private:
            void parse_in(const std::string& in,
                          std::map<std::string, std::string>* out);
            void signal_and_write(const std::string& raw,
                                  protobuf::ModemMsgBase* base_msg = 0);
            
            
          private:
            enum { DEFAULT_BAUD = 4800 };
            
            
            protobuf::DriverConfig driver_cfg_; // configuration given to you at launch
            std::ostream* log_; // place to log all human readable debugging messages
            // rest is up to you!
        };
    }
}
#endif
