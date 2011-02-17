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

class CiCommander : public CMOOSApp  
{
  public:
    CiCommander();
    virtual ~CiCommander();

    
  private:
    bool OnNewMail(MOOSMSG_LIST &NewMail);
    bool Iterate();
    bool OnConnectToServer();
    bool OnStartUp();

    void RegisterVariables();
    bool ReadConfiguration();

    CommanderCdk gui_;
    CMOOSGeodesy geodesy_;
    tes::ModemIdConvert modem_lookup_;
    goby::acomms::DCCLCodec dccl_;
    std::vector<std::string> loads_;
    std::string community_;
    bool xy_only_;

    CommandGui command_gui_;
    boost::thread command_gui_thread_;
    boost::mutex gui_mutex_;
    
    double start_time_;
    std::map<std::string, std::string> show_vars_;    
};



#endif 
