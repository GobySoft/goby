// t. schneider tes@mit.edu 02.19.09
// ocean engineering graduate student - mit / whoi joint program
// massachusetts institute of technology (mit)
// laboratory for autonomous marine sensing systems (lamss)
// 
// this is iCommander.h 
//
// see the readme file within this directory for information
// pertaining to usage and purpose of this script.
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

#ifndef iCommanderH
#define iCommanderH

#include <cctype>
#include <cmath>
#include <fstream>

#include <boost/thread.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/bind.hpp>

#include "MOOSLIB/MOOSLib.h"
#include "MOOSUtilityLib/MOOSGeodesy.h"
#include "goby/acomms/dccl.h"
#include "command_gui.h"
#include "iCommander_config.pb.h"

#include "goby/moos/libmoos_util/tes_moos_app.h"

class CiCommander : public TesMoosApp
{
  public:
    static CiCommander* get_instance();
    
  private:
    CiCommander();
    virtual ~CiCommander();

    friend class CommandGui;

    void loop(); // from TesMoosApp
    void inbox(const CMOOSMsg& msg);

    static CommanderCdk gui_;
    static CMOOSGeodesy geodesy_;
    static tes::ModemIdConvert modem_lookup_;
    static goby::transitional::DCCLTransitionalCodec dccl_;

    CommandGui command_gui_;
    boost::thread command_gui_thread_;
    static boost::mutex gui_mutex_;
    
    double start_time_;
    std::map<std::string, std::string> show_vars_;

    bool is_started_up_;
    
    static iCommanderConfig cfg_;
    static CiCommander* inst_;
};



#endif 
