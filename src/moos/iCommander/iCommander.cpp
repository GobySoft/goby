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

using namespace std;
using boost::trim_copy;
using tes::stricmp;
using namespace goby::acomms;
//using namespace boost::posix_time;

// Construction / Destruction
CiCommander::CiCommander()
    : command_gui_(gui_, gui_mutex_, geodesy_, modem_lookup_, m_Comms, schema_, dccl_, loads_, community_, xy_only_),
      command_gui_thread_(boost::bind(&CommandGui::run, &command_gui_)),
      start_time_(MOOSTime())
{ }
CiCommander::~CiCommander()
{ }

// OnNewMail: called when new mail (previously registered for)
// has arrived.
bool CiCommander::OnNewMail(MOOSMSG_LIST & NewMail)
{        
    for(MOOSMSG_LIST::iterator p = NewMail.begin(); p != NewMail.end(); p++)
    {
        string key   = p->GetKey(); 	
        bool is_dbl  = p->IsDouble();
        //    bool is_str  = p->IsString();
        
        double dval  = p->GetDouble();
        string sval  = p->GetString();
        
        // uncomment as needed
        // double msg_time = msg.GetTime();
        // string msg_src  = msg.GetSource();
        // string msg_community = msg.GetCommunity();
        
        if((*p).GetTime() < start_time_)
        {
            tout_.EchoWarn(string("ignoring normal mail from " + p->GetKey() + " from before we started"));
        }
        else if(MOOSStrCmp("ACOMMS_ACK", key))
        {   
            vector<string> ack_msg;
            boost::split(ack_msg, sval, boost::is_any_of(":"));

            try
            {
                vector<string> mesg;
                mesg.push_back(string("</B>Message </40>acknowledged<!40> from queue " + dccl_.id2name(boost::lexical_cast<unsigned>(ack_msg.at(0)))));
                mesg.push_back(ack_msg.at(1));
                mesg.push_back(string(" at time: " + command_gui_.microsec_simple_time_of_day()));
                gui_.disp_info(mesg);
            }
            catch(std::exception&) // boost::lexical_cast and goby::acomms::DCCLCodec
            { }
        }
        else if(MOOSStrCmp("ACOMMS_EXPIRE", key))
        {
            vector<string> ack_msg;
            boost::split(ack_msg, sval, boost::is_any_of(":"));
            
            try
            {
                vector<string> mesg;
                mesg.push_back(string("</B>Message </16>expired<!16> from queue: " + dccl_.id2name(boost::lexical_cast<unsigned>(ack_msg.at(0))) + " at time: " + command_gui_.microsec_simple_time_of_day()));
                mesg.push_back(ack_msg.at(1));
                mesg.push_back(string(" at time: " + command_gui_.microsec_simple_time_of_day()));
                gui_.disp_info(mesg);
            }
            catch(std::exception&) // boost::lexical_cast and goby::acomms::DCCLCodec
            { }
        }
        else
        {
            map<string, string>::iterator it = show_vars_.find(key);
            if(it != show_vars_.end()) 
            {
                if(is_dbl)
                    show_vars_[key] = tes::doubleToString(dval);
                else
                    show_vars_[key] = sval;
            }
            vector<string> mesg;
            for(map<string, string>::iterator it = show_vars_.begin(), n = show_vars_.end(); it!=n; ++it)
                mesg.push_back(string(it->first + ": " + it->second));
            //gui_.disp_lower_info(mesg);            
         }
        
    }
    
    return true;
}

// OnConnectToServer: called when connection to the MOOSDB is
// complete
bool CiCommander::OnConnectToServer()
{
    if (!ReadConfiguration())
	return false;
 
    RegisterVariables();   

    MOOSPause(1000);

    command_gui_.initialize();
    gui_.set_lower_box_size(show_vars_.size()+2);    
    
    return true;
}

// Iterate: called AppTick times per second
bool CiCommander::Iterate()
{ return true; }

// OnStartUp: called when program is run
bool CiCommander::OnStartUp()
{ return true; }

// RegisterVariables: register for variables we want to get mail for
void CiCommander::RegisterVariables()
{
    m_Comms.Register("ACOMMS_ACK", 0);
    m_Comms.Register("ACOMMS_EXPIRE", 0);

    for(map<string, string>::iterator it = show_vars_.begin(), n = show_vars_.end(); it!=n; ++it)
        m_Comms.Register(it->first, 0);
}

// ReadConfiguration: reads in configuration values from the .moos file
// parameter keys are case insensitive
bool CiCommander::ReadConfiguration()
{
    tout_.SetProcessName("iCommander");

    // if the user doesn't tell us otherwise, we're talking!
    string verbosity = "verbose";
    if (!m_MissionReader.GetConfigurationParam("verbosity", verbosity))
	tout_.EchoWarn("verbosity not specified in configuration. using verbose mode.");
    
    tout_.SetVerbosity(verbosity);

    // read the schema
    if (!m_MissionReader.GetConfigurationParam("schema", schema_))
    {
        tout_.EchoWarn("no schema specified. xml error checking disabled!");
    }

    double latOrigin, longOrigin;
    tout_.EchoInform("reading in geodesy information: ");
    if (!m_MissionReader.GetValue("LatOrigin", latOrigin) || !m_MissionReader.GetValue("LongOrigin", longOrigin))
    {
        tout_.EchoDie("no LatOrigin and/or LongOrigin in moos file. this is required for geodesic conversion");
    }
    else if (!geodesy_.Initialise(latOrigin, longOrigin))
    {
        tout_.EchoDie("CMOOSGeodesy initialization failed.");
    }

    string path;
    if (!m_MissionReader.GetValue("modem_id_lookup_path", path))
    {
        tout_.EchoWarn("no modem_id_lookup_path in moos file. this is required for modem_id conversion");
    }
    else
    {
        string message = modem_lookup_.read_lookup_file(path);

        while(message != "")
        {
            string line = MOOSChomp(message, "\n");
            tout_.EchoInform(line);
        }
    }

    if (!m_MissionReader.GetValue("Community", community_))
    {
        tout_.EchoWarn("no Community in moos file. this is required for special:community");
    }
    
    xy_only_ = false;
    m_MissionReader.GetConfigurationParam("force_xy_only", xy_only_);
    
    list<string> params;
    if(m_MissionReader.GetConfiguration(GetAppName(), params))
    {
        params.reverse();        

        for(list<string>::iterator it = params.begin(); it !=params.end(); ++it)
        {
            // trim_copy is from boost
            string value = trim_copy(*it);
            string key = trim_copy(MOOSChomp(value, "="));

            if(stricmp(key, "message_file"))
            {
                tout_.EchoInform(string("try to parse file: " + value));
                // parse the message file
                dccl_.add_xml_message_file(value, schema_);
                tout_.EchoInform("success!");
            }
            else if(stricmp(key, "load"))
            {
                loads_.push_back(value);
            }                
            else if(stricmp(key, "show_variable"))
            {
                show_vars_[value] = "";
            }                
        }
    }
    
    return true;
}

