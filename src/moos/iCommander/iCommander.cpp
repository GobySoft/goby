// t. schneider tes@mit.edu 02.19.09
// ocean engineering graduate student - mit / whoi joint program
// massachusetts institute of technology (mit)
// laboratory for autonomous marine sensing systems (lamss)
// 
// this is iCommander.cpp 
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

#include "iCommander.h"
#include "goby/moos/libmoos_util/tes_moos_app.h"
#include "goby/protobuf/modem_message.pb.h"

using namespace std;
using boost::trim_copy;
using namespace goby::transitional;
using goby::util::glogger;

iCommanderConfig CiCommander::cfg_;
CommanderCdk CiCommander::gui_;
CMOOSGeodesy CiCommander::geodesy_;
tes::ModemIdConvert CiCommander::modem_lookup_;
goby::transitional::DCCLTransitionalCodec CiCommander::dccl_;
boost::mutex CiCommander::gui_mutex_;
CiCommander* CiCommander::inst_ = 0;


CiCommander* CiCommander::get_instance()
{
    if(!inst_)
        inst_ = new CiCommander();
    return inst_;
}

// Construction / Destruction
CiCommander::CiCommander()
    : TesMoosApp(&cfg_),
      command_gui_thread_(boost::bind(&CommandGui::run, &command_gui_)),
      start_time_(MOOSTime()),
      is_started_up_(false)
{

    if (!cfg_.common().has_lat_origin() || !cfg_.common().has_lon_origin())
    {
        glogger() << die << "no lat_origin or lon_origin specified in configuration. this is required for geodesic conversion" << std::endl;
    }
    else
    {
        if(geodesy_.Initialise(cfg_.common().lat_origin(), cfg_.common().lon_origin()))
            glogger() << "success!" << std::endl;
        else
            glogger() << die << "could not initialize geodesy" << std::endl;
    }

    if(cfg_.has_modem_id_lookup_path())
        glogger() << modem_lookup_.read_lookup_file(cfg_.modem_id_lookup_path()) << std::endl;
    else
        glogger() << warn << "no modem_id_lookup_path in moos file. this is required for conversions between modem_id and vehicle name / type." << std::endl;    
    
    dccl_.merge_cfg(cfg_.transitional_cfg());


    for(int i = 0, n = cfg_.show_variable_size(); i < n; ++i)
    {
        show_vars_[cfg_.show_variable(i)] = "";
        subscribe(cfg_.show_variable(i), &CiCommander::inbox, this);
    }
    
    
    subscribe("ACOMMS_ACK", &CiCommander::inbox, this);
    subscribe("ACOMMS_EXPIRE", &CiCommander::inbox, this);
}

CiCommander::~CiCommander()
{ }

// OnNewMail: called when new mail (previously registered for)
// has arrived.
void CiCommander::inbox(const CMOOSMsg& msg)
{        
    string key   = msg.GetKey(); 	
    bool is_dbl  = msg.IsDouble();
    //    bool is_str  = p->IsString();
    
    double dval  = msg.GetDouble();
    string sval  = msg.GetString();
    
    // uncomment as needed
    // double msg_time = msg.GetTime();
    // string msg_src  = msg.GetSource();
    // string msg_community = msg.GetCommunity();
    
    if(msg.GetTime() < start_time_)
    {
        std::cerr << "ignoring normal mail from " << msg.GetKey() << " from before we started" << std::endl;
    }
    else if(MOOSStrCmp("ACOMMS_ACK", key))
    {   
        try
        {
            vector<string> mesg;
            mesg.push_back(string("</B>Message </40>acknowledged<!40>. Original Message:"));
            mesg.push_back(sval.substr(0,30) + "...");
            gui_.disp_info(mesg);
        }
        catch(...)
        { }
        
        
    }
    else if(MOOSStrCmp("ACOMMS_EXPIRE", key))
    {
        try
        {
            vector<string> mesg;
            mesg.push_back(string("</B>Message </16>expired<!16> Original Message:"));
            mesg.push_back(sval.substr(0,30) + "...");
            gui_.disp_info(mesg);
        }
        catch(...)
        { }
        
    }
    else
    {
        map<string, string>::iterator it = show_vars_.find(key);
        if(it != show_vars_.end()) 
        {
            if(is_dbl)
                show_vars_[key] = goby::util::as<string>(dval);
            else
                show_vars_[key] = sval;
        }
        vector<string> mesg;
        for(map<string, string>::iterator it = show_vars_.begin(), n = show_vars_.end(); it!=n; ++it)
            mesg.push_back(string(it->first + ": " + it->second));
        //gui_.disp_lower_info(mesg);            
    }    
}


void CiCommander::loop()
{
    if(!is_started_up_)
    {
        command_gui_.initialize();
        gui_.set_lower_box_size(show_vars_.size()+2);    
        is_started_up_ = true;
        
    }
}
