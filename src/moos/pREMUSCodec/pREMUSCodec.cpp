// t. schneider tes@mit.edu 07.25.08
// ocean engineering graduate student - mit / whoi joint program
// massachusetts institute of technology (mit)
// laboratory for autonomous marine sensing systems (lamss)
// 
// this is pREMUSCodec.cpp 
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

#include "pREMUSCodec.h"

#include "goby/util/string.h"
#include "boost/algorithm/string.hpp"
#include "goby/acomms/protobuf/modem_message.pb.h"
#include "goby/moos/libmoos_util/moos_protobuf_helpers.h"

using namespace std;
using goby::util::as;



// Construction / Destruction
CpREMUSCodec::CpREMUSCodec()
    : my_id(0)
{
  got_x =false;
  got_y = false;
  got_heading = false;
  got_speed = false;
  got_depth = false;
  status_interval = 10;
  status_time = 10;
  remus_status = false;

  mdat_state_var = "IN_REMUS_STATUS_HEX_30B";
  mdat_state_out = "OUT_REMUS_STATUS_HEX_30B";

  mdat_ranger_var = "IN_REMUS_RANGER_HEX_30B";
  mdat_ranger_out = "OUT_REMUS_RANGER_HEX_30B";

  mdat_redirect_var = "IN_REMUS_REDIRECT_HEX_30B";
  mdat_redirect_out = "OUT_REMUS_REDIRECT_HEX_30B";

  mdat_alert_var = "IN_REMUS_ALERT_HEX_30B";
  mdat_alert_out = "OUT_REMUS_ALERT_HEX_30B";

  mdat_alert2_var = "IN_REMUS_ALERT2_HEX_30B";
  mdat_alert2_out = "OUT_REMUS_ALERT2_HEX_30B";

}
CpREMUSCodec::~CpREMUSCodec()
{
}

// OnNewMail: called when new mail (previously registered for)
// has arrived.
bool CpREMUSCodec::OnNewMail(MOOSMSG_LIST &NewMail)
{
for(MOOSMSG_LIST::iterator p = NewMail.begin(); p != NewMail.end(); p++)
    {
        CMOOSMsg &msg = *p;
        
        string key   = msg.GetKey(); 	
	//bool is_dbl  = msg.IsDouble();
	//bool is_str  = msg.IsString();
        //double dval  = msg.GetDouble();
	string sval  = msg.GetString();

	// uncomment as needed
        // double msg_time = msg.GetTime();
	// string msg_src  = msg.GetSource();
        string msg_community = msg.GetCommunity();
	

	if(MOOSStrCmp(key, mdat_state_var) || MOOSStrCmp(key, mdat_ranger_var)
           || MOOSStrCmp(key, mdat_redirect_var) || MOOSStrCmp(key, mdat_alert_var)
            || MOOSStrCmp(key, mdat_alert2_var)) // case insensitive
        {
            goby::acomms::protobuf::ModemDataTransmission msg_in;
            parse_for_moos(sval, &msg_in);
            
	    vector<unsigned int> bytes;
	    for (unsigned i=0; i<32; i++)
            {
                if(i < msg_in.data().size())
                    bytes.push_back(msg_in.data().at(i));
                else
                    bytes.push_back(0);
            }
            
	    int node = msg_in.base().src();
            string name = modem_lookup[atoi(msg_community.c_str())].name;
            string type = modem_lookup[atoi(msg_community.c_str())].type;

            string human;
            
            if(MOOSStrCmp(key, mdat_state_var))
            {
                human = decode_mdat_state(bytes, node, name, type);
                m_Comms.Notify("NODE_REPORT", human);
            }
            else if(MOOSStrCmp(key, mdat_ranger_var))
            {
                human = decode_mdat_ranger(bytes, node, name, type);
                m_Comms.Notify("NODE_REPORT", human);
            }
            else if(MOOSStrCmp(key, mdat_redirect_var))
                human = decode_mdat_redirect(bytes, node, name, type);
            else if(MOOSStrCmp(key, mdat_alert_var))
                human = decode_mdat_alert(bytes, node, name, type);
            else if(MOOSStrCmp(key, mdat_alert2_var))
                human = decode_mdat_alert2(bytes, node, name, type);
	  }
	else if(MOOSStrCmp(key,"MODEM_ID"))
	  {	 
	    my_id = atoi(sval.c_str());
	  } 
	else if(MOOSStrCmp(key,"NAV_X"))
	  {	 
	    nav_x = msg.m_dfVal;
	    got_x =true; 
	  } 
	else if(MOOSStrCmp(key,"NAV_Y"))
	  {	 
	    nav_y = msg.m_dfVal; 
	    got_y =true; 
	  } 
	else if(MOOSStrCmp(key,"NAV_DEPTH"))
	  {	 
	    nav_depth = msg.m_dfVal; 
	    got_depth =true; 
	  } 
	else if(MOOSStrCmp(key,"NAV_SPEED"))
	  {	 
	    nav_speed = msg.m_dfVal; 
	    got_speed =true; 
	  } 
	else if(MOOSStrCmp(key,"NAV_HEADING"))
	  {	 
	    nav_heading = msg.m_dfVal; 
	    got_heading =true; 
	  } 
	else if(MOOSStrCmp(key,"OUTGOING_COMMAND"))
	  {	 
	    int dest_id;
	    // check if it is a CCL_REDIRECT and then encode
	    if (parseRedirect(msg.m_sVal, dest_id))
	      {
		MOOSTrace("parseRedirect: dest_id= %d\n", dest_id);

                vector<unsigned int> bytes;
		for (int i=0; i<32; i++)
                    bytes.push_back(0);
		if (encode_mdat_redirect(bytes))
                {
                    goby::acomms::protobuf::ModemDataTransmission msg_out;
                    
                    for (int i=0, n=bytes.size(); i<n; i++)
                        msg_out.mutable_data()->append(1, char(bytes[i]));

                    msg_out.mutable_base()->set_src(my_id);
                    msg_out.mutable_base()->set_dest(dest_id);

                    std::string out;
                    serialize_for_moos(&out, msg_out);

                    std::cerr << "publishing to " << mdat_redirect_out << " " << out << std::endl;
                    m_Comms.Notify(mdat_redirect_out, out);
                }
	      }
	  } 
    }  
    return true;
}

// OnConnectToServer: called when connection to the MOOSDB is
// complete
bool CpREMUSCodec::OnConnectToServer()
{
    RegisterVariables();    
    return true;
}

// Iterate: called AppTick times per second
bool CpREMUSCodec::Iterate()
{ 

  double curr_time = MOOSTime();
  bool got_nav = got_x && got_y && got_heading && got_speed && got_depth;

  if (remus_status && got_nav && curr_time - status_time  >= status_interval)
    {
      m_geodesy.UTM2LatLong(nav_x, nav_y, nav_lat, nav_lon);
      
      vector<unsigned int> bytes;
      for (int i=0; i<32; i++)
	bytes.push_back(0);
      if (encode_mdat_state(bytes))
	{
            goby::acomms::protobuf::ModemDataTransmission msg_out;
            
            for (int i=0, n=bytes.size(); i<n; i++)
                msg_out.mutable_data()->append(1, char(bytes[i]));

            msg_out.mutable_base()->set_src(my_id);
            msg_out.mutable_base()->set_dest(0);
            
            std::string out;
            serialize_for_moos(&out, msg_out);

            std::cerr << "publishing to " << mdat_state_out << " " << out << std::endl;

            m_Comms.Notify(mdat_state_out, out);
            status_time = curr_time;
	}
    }
  
  return true;
}

// OnStartUp: called when program is run
bool CpREMUSCodec::OnStartUp()
{
    if (!ReadConfiguration())
	return false;

    RegisterVariables();
    
    return true;
}

// RegisterVariables: register for variables we want to get mail for
void CpREMUSCodec::RegisterVariables()
{
    m_Comms.Register(mdat_state_var, 0);
    m_Comms.Register(mdat_state_out, 0);
    m_Comms.Register(mdat_ranger_var, 0);
    m_Comms.Register(mdat_ranger_out, 0);
    m_Comms.Register(mdat_redirect_var, 0);
    m_Comms.Register(mdat_redirect_out, 0);
    m_Comms.Register(mdat_alert_var, 0);
    m_Comms.Register(mdat_alert_out, 0);
    m_Comms.Register(mdat_alert2_var, 0);
    m_Comms.Register(mdat_alert2_out, 0);
    m_Comms.Register("MODEM_ID", 0);
    m_Comms.Register("NAV_X", 0);
    m_Comms.Register("NAV_Y", 0);
    m_Comms.Register("NAV_SPEED", 0);
    m_Comms.Register("NAV_DEPTH", 0);
    m_Comms.Register("NAV_HEADING", 0);
    m_Comms.Register("OUTGOING_COMMAND", 0);
    return;
}

// ReadConfiguration: reads in configuration values from the .moos file
// parameter keys are case insensitive
bool CpREMUSCodec::ReadConfiguration()
{

    string create_status;
    // do we want to generate Remus CCL Status reports
    remus_status = false;
    if (!m_MissionReader.GetConfigurationParam("create_status", create_status))
        std::cerr << "create_status not specified. Will not generate Remus status" << std::endl;
    else
      remus_status = MOOSStrCmp(create_status, "true");

    string bufstr;
    // for reading global values, use m_MissionReader.GetValue(parameter, variable)

    if (!m_MissionReader.GetValue("mdat_state_var", bufstr))
        MOOSTrace("mdat_state_var not specified in .moos file. Using default.%s\n",mdat_state_var.c_str());
    else
      mdat_state_var = bufstr;

    if (!m_MissionReader.GetValue("mdat_state_out", bufstr))
      MOOSTrace("mdat_state_out not specified in .moos file. Using default.%s\n",mdat_state_out.c_str());
    else
      mdat_state_out = bufstr;
    // MDAT_RANGER
    if (!m_MissionReader.GetValue("mdat_ranger_var", bufstr))
      MOOSTrace("mdat_state_var not specified in .moos file. Using default.%s\n",mdat_ranger_var.c_str());
    else
      mdat_ranger_var = bufstr;

    if (!m_MissionReader.GetValue("mdat_ranger_out", bufstr))
      MOOSTrace("mdat_state_out not specified in .moos file. Using default.%s\n",mdat_ranger_out.c_str());
    else
      mdat_ranger_out = bufstr;

    if (!m_MissionReader.GetValue("mdat_redirect_var", bufstr))
      MOOSTrace("mdat_redirect_var not specified in .moos file. Using default.%s\n",mdat_redirect_var.c_str());
    else
      mdat_redirect_var = bufstr;

    if (!m_MissionReader.GetValue("mdat_redirect_out", bufstr))
      MOOSTrace("mdat_redirect_out not specified in .moos file. Using default.%s\n",mdat_redirect_out.c_str());
    else
      mdat_redirect_out = bufstr;

    if (!m_MissionReader.GetValue("mdat_alert_var", bufstr))
      MOOSTrace("mdat_alert_var not specified in .moos file. Using default.%s\n",mdat_alert_var.c_str());
    else
      mdat_alert_var = bufstr;

    if (!m_MissionReader.GetValue("mdat_alert_out", bufstr))
      MOOSTrace("mdat_alert_out not specified in .moos file. Using default.%s\n",mdat_alert_out.c_str());
    else
      mdat_alert_out = bufstr;

    if (!m_MissionReader.GetValue("mdat_alert2_var", bufstr))
      MOOSTrace("mdat_alert2_var not specified in .moos file. Using default.%s\n",mdat_alert2_var.c_str());
    else
      mdat_alert2_var = bufstr;

    if (!m_MissionReader.GetValue("mdat_alert2_out", bufstr))
      MOOSTrace("mdat_alert2_out not specified in .moos file. Using default.%s\n",mdat_alert2_out.c_str());
    else
      mdat_alert2_out = bufstr;

    string path;
    if (!m_MissionReader.GetValue("modem_id_lookup_path", path))
    {
        MOOSTrace("no modem_id_lookup_path specified!\n");
        exit(1);
    }
    
    ifstream fin;
    
    fin.open(path.c_str(), ifstream::in);
    
    if (fin.fail())
    {
        MOOSTrace("cannot open %s for reading!\n", path.c_str());
        exit(1);   
    }
    
    MOOSTrace("reading in modem id lookup table:\n");
    
    while(fin)
    {
        char line[5000];
        fin.getline(line, 5000);
        
        string sline = line;
        
        // strip the spaces and comments out
        string::size_type pos = sline.find(' ');
        while(pos != string::npos)
        {
	  sline.erase(pos, 1);
	  pos = sline.find(' ');
        }
        pos = sline.find('#');
        while(pos != string::npos)
        {
            sline.erase(pos);
            pos = sline.find('#');
        }
        pos = sline.find("//");
        while(pos != string::npos)
        {
            sline.erase(pos);
            pos = sline.find("//");
        }
        
        // ignore blank lines
        if(sline != "")
        {
            vector<string> line_parsed;
            boost::split(line_parsed, sline, boost::is_any_of(","));
            if(line_parsed.size() < 3)
                MOOSTrace("invalid line: %s\n", line);
            else
            {
                cout << "modem id [" << line_parsed[0] << "], name [" << line_parsed[1] << "], type [" << line_parsed[2] << "]" << endl;
                
                vehicle_nametype new_vehicle =
                    {line_parsed[1], line_parsed[2]};
                
                //add the entry to our lookup map
                modem_lookup[atoi(line_parsed[0].c_str())] = new_vehicle;
            }
        }
    }
    
    fin.close();

    double latOrigin, longOrigin;
    if (!m_MissionReader.GetValue("LatOrigin", latOrigin))
    {
        std::cerr << "LatOrigin not set in *.moos file." << std::endl;
        exit(EXIT_FAILURE);
    }
    
    south = (latOrigin < 0.0);
    
    if (!m_MissionReader.GetValue("LongOrigin", longOrigin))
    {
        std::cerr << "LongOrigin not set in *.moos file" << std::endl;
        exit(EXIT_FAILURE);
    }
    
    west = (longOrigin < 0.0);
    
    // initialize m_geodesy
    if (!m_geodesy.Initialise(latOrigin, longOrigin))
    {
        std::cerr << "Geodesy init failed." << std::endl;
        exit(EXIT_FAILURE);
    }
    
    return true;
}


bool CpREMUSCodec::hex_to_int_array(string h, vector<unsigned int>& c )
{
    boost::to_upper(h);
    
    for(unsigned int i = 0; i < h.size()/2; ++i)
    {
        char buff[5];
        buff[0] = '0';
        buff[1] = 'x';
        buff[2] = h[2*i];
        buff[3] = h[2*i+1];
        buff[4] = '\0';
        
        sscanf(buff,"%X",&c[i]);
		MOOSTrace("hex_to_int_array: c[%d] = %X; buff = %s\n", i, c[i], buff);
    }
    
    return true;
}

    
string CpREMUSCodec::int_array_to_hex(vector<unsigned int> c )
{
  char buff[5];
  string hex = "";
  
  for(unsigned int i = 0; i < c.size(); ++i)
    {
      c[i] = c[i] & 0x00ff;
      sprintf(buff,"%02X",c[i] & 0xff);
        MOOSTrace("int_array_to_hex: c[%d] = %X; buff = %s\n", i, c[i], buff);
      hex += MOOSFormat("%s",buff);    
    }

    
  return(hex);
}

string CpREMUSCodec::decode_mdat_ranger(vector<unsigned int> c, int node, string sender, string type)
{

  vector<unsigned char> cc;

  for (unsigned int i=0; i< c.size(); i++)
    cc.push_back( (unsigned char) c[i]);

  double dlat = DecodeRangerLL(c[1],c[2],c[3],c[4],c[5]);
  double dlon = DecodeRangerLL(c[6],c[7],c[8],c[9],c[10]);
  double dhdg = DecodeRangerBCD2(c[19],c[20]) * 0.1 ;
  double dspd = DecodeRangerBCD(c[18])*0.1;
  double ddepth = DecodeRangerBCD2(c[28],c[23]);
  
  double x,y;
  // convert lat, long into x, y. 60 nautical miles per minute
  m_geodesy.LatLong2LocalUTM(dlat, dlon, y, x);

  // Create human-readable version
  
  string ccl_status = "MessageType=CCL_STATUS" ;
  ccl_status += ",Node=" + as<string>(node);
  ccl_status += ",Timestamp=" + as<string>(MOOSTime());
  ccl_status += ",Latitude=" + as<string>(dlat);
  ccl_status += ",Longitude=" + as<string>(dlon);
  ccl_status += ",Speed=" + as<string>(dspd);
  ccl_status += ",Heading=" + as<string>(dhdg);
  ccl_status += ",Depth=" + as<string>(ddepth);
  
  m_Comms.Notify("STATUS_REPORT_IN",ccl_status);
  
  
  // Create contact variables for collision avoidance
  
  MOOSTrace("Creating Contact fields\n");
  
  string contact_name = boost::to_upper_copy(sender);
  string moosvar = contact_name + "_NAV_UTC";
  m_Comms.Notify(moosvar,MOOSTime());
  moosvar = contact_name+"_NAV_X";
  m_Comms.Notify(moosvar,x);
  moosvar = contact_name+"_NAV_Y";
  m_Comms.Notify(moosvar,y);
  moosvar = contact_name+"_NAV_LAT";
  m_Comms.Notify(moosvar,dlat);
  moosvar = contact_name+"_NAV_LONG";
  m_Comms.Notify(moosvar,dlon);
  moosvar = contact_name+"_NAV_SPEED";
  m_Comms.Notify(moosvar,dspd);
  moosvar = contact_name+"_NAV_DEPTH";
  m_Comms.Notify(moosvar,ddepth);
  moosvar = contact_name+"_NAV_HEADING";
  m_Comms.Notify(moosvar,dhdg);
  
  return assemble_NODE_REPORT(sender,
		     type,
		     as<string>(-1),
		     as<string>(MOOSTime()),
		     as<string>(x),
		     as<string>(y),
		     as<string>(dlat),
		     as<string>(dlon),
		     as<string>(dspd),
		     as<string>(dhdg),
		     as<string>(ddepth),as<string>(dhdg-360));
  
}

string CpREMUSCodec::decode_mdat_state(vector<unsigned int> c, int node, string sender, string type)
{
// typedef struct
// {
//     unsigned char     mode;
//     // MDAT_STATE
//     LATLON_COMPRESSED latitude;
//     // 3 bytes
//     LATLON_COMPRESSED longitude;
//       unsigned char     fix_age
//       TIME_DATE         time_date;
//     // 3 bytes;
//     unsigned char     heading;
//     // 1.5 degree resolution
//     unsigned short    mission_mode_depth;
//     //
//     unsigned long     faults;
//     unsigned char     faults_2;
//     unsigned char     mission_leg;
//     char              est_velocity;
//     char              objective_index;
//     unsigned char     watts_encoded;
//     LATLON_COMPRESSED lat_goal;
//     // 3 bytes
//     LATLON_COMPRESSED lon_goal;
//     // 3 bytes
//     unsigned char     battery_percent;
//     unsigned short    gfi_pitch_oil_encoded;
//     // 5 bits gfi,6 bits pitch,
//     // 5 bits oil
// }
// MODEM_MSG_DATA_STATE;
//    unsigned char mode = c[0];

    LATLON_COMPRESSED lat, lon;
    lat.compressed[0] = c[1];
    lat.compressed[1] = c[2];
    lat.compressed[2] = c[3];

    lon.compressed[0] = c[4];
    lon.compressed[1] = c[5];
    lon.compressed[2] = c[6];

    double dlat = Decode_latlon(lat);

    double dlon = Decode_latlon(lon);
    if (dlon > 180.0)
      dlon -= 360.0;

    //unsigned char fix_age = c[7];

    TIME_DATE td;
    td.compressed[0] = c[8];
    td.compressed[1] = c[9];
    td.compressed[2] = c[10];

    unsigned char hdg = c[11];

    unsigned short depth = c[13];
    depth = (((depth << 8) | c[12]) & 0x1fff);

    char spd = c[20];
    
    LATLON_COMPRESSED lat_goal, lon_goal;
    lat_goal.compressed[0] = c[23];
    lat_goal.compressed[1] = c[24];
    lat_goal.compressed[2] = c[25];

    lon_goal.compressed[0] = c[26];
    lon_goal.compressed[1] = c[27];
    lon_goal.compressed[2] = c[28];



    unsigned long ulArg = c[30] + 256*c[31];
    float fGFI,fPitch, fOil;
    Decode_gfi_pitch_oil(ulArg, &fGFI, &fPitch, &fOil);

    // rest of message is not really useful for us
    
    double glat = Decode_latlon(lat_goal);

    double glon = Decode_latlon(lon_goal);

    double dhdg = Decode_heading(hdg);
    double ddepth = Decode_depth(depth);
    double dspd = Decode_est_velocity(spd);
    
    short mon, day, hour, min, sec;
    
    Decode_time_date(td, &mon, &day, &hour, &min, &sec);

    cout << "mon: " << mon << endl;
    cout << "day: " << day << endl;
    cout << "hour: " << hour << endl;
    cout << "min: " << min << endl;
    cout << "sec: " << sec << endl;

    double x,y;
    // convert lat, long into x, y. 60 nautical miles per minute
    m_geodesy.LatLong2LocalUTM(dlat, dlon, y, x);


    // Create human-readable version

    string ccl_status = "MessageType=CCL_STATUS" ;
    ccl_status += ",Node=" + as<string>(node);
    ccl_status += ",Timestamp=" + as<string>(MOOSTime());
    ccl_status += ",Latitude=" + as<string>(dlat);
    ccl_status += ",Longitude=" + as<string>(dlon);
    ccl_status += ",Speed=" + as<string>(dspd);
    ccl_status += ",Heading=" + as<string>(dhdg);
    ccl_status += ",Depth=" + as<string>(ddepth);
    ccl_status += ",GoalLatitude=" + as<string>(glat);
    ccl_status += ",GoalLongitude=" + as<string>(glon);
    ccl_status += ",Batterypercent=" + as<string>((int) c[29]);
    ccl_status += ",GFI=" + as<string>((double) fGFI);
    ccl_status += ",Pitch=" + as<string>((double) fPitch);
    ccl_status += ",Oil=" + as<string>((double) fOil);

    m_Comms.Notify("STATUS_REPORT_IN",ccl_status);

   
    // Create contact variables for collision avoidance

    MOOSTrace("Creating Contact fields\n");
    
    string contact_name = boost::to_upper_copy(sender);
    string moosvar = contact_name + "_NAV_UTC";
    m_Comms.Notify(moosvar,MOOSTime());
    moosvar = contact_name+"_NAV_X";
    m_Comms.Notify(moosvar,x);
    moosvar = contact_name+"_NAV_Y";
    m_Comms.Notify(moosvar,y);
    moosvar = contact_name+"_NAV_LAT";
    m_Comms.Notify(moosvar,dlat);
    moosvar = contact_name+"_NAV_LONG";
    m_Comms.Notify(moosvar,dlon);
    moosvar = contact_name+"_NAV_SPEED";
    m_Comms.Notify(moosvar,dspd);
    moosvar = contact_name+"_NAV_DEPTH";
    m_Comms.Notify(moosvar,ddepth);
    moosvar = contact_name+"_NAV_HEADING";
    m_Comms.Notify(moosvar,dhdg);
  
    return assemble_NODE_REPORT(sender,
                       type,
                       as<string>(-1),
                       as<string>(MOOSTime()),
                       as<string>(x),
                       as<string>(y),
                       as<string>(dlat),
                       as<string>(dlon),
                       as<string>(dspd),
                       as<string>(dhdg),
                       as<string>(ddepth),as<string>(dhdg-360));
}

// replaces assembleAIS. mfallon dec2010
string CpREMUSCodec::assemble_NODE_REPORT(string name, string type, string db_time, string utc_time,
                                   string x, string y, string lat, string lon, string spd,
                                   string hdg, string depth, string yaw)
{
	  // Arbitary string ending: since this info is not in these REMUS messages
	  string rest_of_string = ",LENGTH=4.0,MODE=NOMOOS,ALLSTOP=NotUsingIvP";

    string summary = "NAME=" + name;
    summary += ",TYPE=" + type;
    summary += ",MOOSDB_TIME=" + db_time;
    summary += ",UTC_TIME=" + utc_time;
    summary += ",X="   + x;
    summary += ",Y="   + y;
    summary += ",LAT=" + lat;
    summary += ",LON=" + lon;
    summary += ",SPD=" + spd;
    summary += ",HDG=" + hdg;
    summary += ",YAW=" + yaw;
    summary += ",DEPTH=" + depth;
    summary += rest_of_string;

    return summary;
}

bool CpREMUSCodec::encode_mdat_state(vector<unsigned int>& c)
{
// typedef struct
// {
//     unsigned char     mode;
//     // MDAT_STATE
//     LATLON_COMPRESSED latitude;
//     // 3 bytes
//     LATLON_COMPRESSED longitude;
//       unsigned char     fix_age
//       TIME_DATE         time_date;
//     // 3 bytes;
//     unsigned char     heading;
//     // 1.5 degree resolution
//     unsigned short    mission_mode_depth;
//     //
//     unsigned long     faults;
//     unsigned char     faults_2;
//     unsigned char     mission_leg;
//     char              est_velocity;
//     char              objective_index;
//     unsigned char     watts_encoded;
//     LATLON_COMPRESSED lat_goal;
//     // 3 bytes
//     LATLON_COMPRESSED lon_goal;
//     // 3 bytes
//     unsigned char     battery_percent;
//     unsigned short    gfi_pitch_oil_encoded;
//     // 5 bits gfi,6 bits pitch,
//     // 5 bits oil
// }
// MODEM_MSG_DATA_STATE;

  for (unsigned int i=0; i<c.size(); i++)
    c[i] = 0x0000;
 
  c[0] = 0x000E;

  LATLON_COMPRESSED lat, lon;

  lat = Encode_latlon( nav_lat );

  lon = Encode_latlon( nav_lon );

  c[1] = lat.compressed[0];
  c[2] = lat.compressed[1];
  c[3] = lat.compressed[2];
  
  c[4] = lon.compressed[0];
  c[5] = lon.compressed[1];
  c[6] = lon.compressed[2];
  
  //unsigned char fix_age = c[7];
  
  TIME_DATE td;

  td = Encode_time_date((long) MOOSTime());
  
  c[8] = td.compressed[0];
  c[9] = td.compressed[1];
  c[10] = td.compressed[2];
  
  c[11] = Encode_heading((float) nav_heading);

  
  unsigned short depth = Encode_depth((float) nav_depth);
  c[12] = depth & 0x00ff;
  c[13] = (depth & 0xff00) >> 8;
 
  c[20] = Encode_est_velocity((float) nav_speed);
  
  return true;
}

string CpREMUSCodec::decode_mdat_redirect(vector<unsigned int> c, int node, string sender, string type)
{
  //    unsigned char mode = c[0];

    // redirect latitude

    LATLON_COMPRESSED lat, lon;
    lat.compressed[0] = c[2];
    lat.compressed[1] = c[3];
    lat.compressed[2] = c[4];

    lon.compressed[0] = c[5];
    lon.compressed[1] = c[6];
    lon.compressed[2] = c[7];

    double dlat = Decode_latlon(lat);
    double dlon = Decode_latlon(lon);
    // convert lat, long into x, y. 60 nautical miles per minute
    double x,y;
    m_geodesy.LatLong2LocalUTM(dlat, dlon, y, x);

    char buff[5];
    sprintf(buff,"%02X",c[8] & 0xff);
    spd_dep_flags += MOOSFormat("%s",buff);    

    // Transit depth
    unsigned short depth = c[10];
    depth = ((depth << 8) | c[9]) & 0x1fff;
    double dtdepth = Decode_depth(depth);

    // Transit Speed
    char spd = c[11];
    double dtspd = Decode_speed(SPEED_MODE_MSEC,spd);
    
    // Transit Command
    unsigned short tcomm = c[12];

    // Survey depth
    unsigned short sdepth = c[14];
    double dsdepth = Decode_depth( (((sdepth << 8) | c[13]) & 0x1fff)) ;
    
    // Survey Speed
    spd = c[15];
    double dsspd = Decode_speed(SPEED_MODE_MSEC,spd);
    
    // Survey Command
    unsigned short scomm = c[16];

    // Number of Rows
    unsigned short nrows = c[17];
    // Survey row length
    unsigned short rowlen =  ( (c[19] << 8) | c[18] ) & 0x1fff ;
    // Row spacing 0
    unsigned short rowspace0 = c[20];
    // Row spacing 1
    unsigned short rowspace1 = c[21];
    // Row heading
    unsigned short hdg = c[22];
    double dshdg = Decode_heading(hdg);

    // Start Lat Long
    LATLON_COMPRESSED slat, slon;
    slat.compressed[0] = c[23];
    slat.compressed[1] = c[24];
    slat.compressed[2] = c[25];

    slon.compressed[0] = c[26];
    slon.compressed[1] = c[27];
    slon.compressed[2] = c[28];

    double dslat = Decode_latlon(slat);
    double dslon = Decode_latlon(slon);
    // convert lat, long into x, y. 60 nautical miles per minute
    double dsx,dsy;
    m_geodesy.LatLong2LocalUTM(dlat, dlon, dsy, dsx);

    // Create human-readable version

    string ccl_redirect = "MessageType=CCL_REDIRECT" ;
    ccl_redirect += ",DestinationPlatformId=" + as<string>(node);
    ccl_redirect += ",Timestamp=" + as<string>(MOOSTime());
    ccl_redirect += ",Latitude=" + as<string>(dlat);
    ccl_redirect += ",Longitude=" + as<string>(dlon);
    ccl_redirect += ",SpeedDepthFlags=" + spd_dep_flags;
    ccl_redirect += ",TransitSpeed=" + as<string>(dtspd);
    ccl_redirect += ",TransitDepth=" + as<string>(dtdepth);
    ccl_redirect += ",TransitCommand=" + as<string>((int) tcomm);
    ccl_redirect += ",SurveySpeed=" + as<string>(dsspd);
    ccl_redirect += ",SurveyDepth=" + as<string>(dsdepth);
    ccl_redirect += ",SurveyCommand=" + as<string>((int) scomm);
    ccl_redirect += ",NumberOfRows=" + as<string>((int) nrows);
    ccl_redirect += ",RowLength=" + as<string>((int) rowlen);
    ccl_redirect += ",RowSpacing0=" + as<string>((int) rowspace0);
    ccl_redirect += ",RowSpacing1=" + as<string>((int) rowspace1);
    ccl_redirect += ",RowHeading=" + as<string>(dshdg);
    ccl_redirect += ",StartLatitude=" + as<string>(dslat);
    ccl_redirect += ",StartLongitude=" + as<string>(dslon);


    m_Comms.Notify("CCL_REDIRECT_IN",ccl_redirect);

    return(ccl_redirect);
}

bool CpREMUSCodec::encode_mdat_redirect(vector<unsigned int>& c)
{
  for (unsigned int i=0; i<c.size(); i++)
    c[i] = 0x0000;
 
  c[0] = MDAT_REDIRECT;

  LATLON_COMPRESSED lat, lon;

  lat = Encode_latlon( transit_lat );
  
  lon = Encode_latlon( transit_lon );

  c[2] = lat.compressed[0];
  c[3] = lat.compressed[1];
  c[4] = lat.compressed[2];
  
  c[5] = lon.compressed[0];
  c[6] = lon.compressed[1];
  c[7] = lon.compressed[2];
  
  // SpeedDepthFlags
  c[8] = strtol(spd_dep_flags.c_str(),NULL,16);


    // Transit depth
  unsigned short depth = Encode_depth((float) transit_depth);
  c[9] = depth & 0x00ff;
  c[10] = (depth & 0xff00) >> 8;
 
    // Transit Speed
  c[11] = Encode_speed(SPEED_MODE_MSEC,(float) transit_speed);
  
  // Transit Command
  c[12] = transit_command;
  

  // Survey depth
  depth = Encode_depth((float) survey_depth);
  c[13] = depth & 0x00ff;
  c[14] = (depth & 0xff00) >> 8;
 
    // Survey Speed
  c[15] = Encode_speed(SPEED_MODE_MSEC,(float) survey_speed);
  
  // Survey Command
  c[16] = survey_command;
  
  // Number of Survey Rows
  c[17] = survey_rows;

  // Survey row length
  c[18] = int(row_length) & 0x00ff;
  c[19] = (int(row_length) & 0xff00) >> 8;
  
  // Row spacing 0
  c[20] = int(row_spacing_0);
  // Row spacing 1
  c[21] = int(row_spacing_1);

  // Survey Row Headings  
  c[22] = Encode_heading((float) row_heading);

  lat = Encode_latlon( start_lat );
  
  lon = Encode_latlon( start_lon );

  c[23] = lat.compressed[0];
  c[24] = lat.compressed[1];
  c[25] = lat.compressed[2];
  
  c[26] = lon.compressed[0];
  c[27] = lon.compressed[1];
  c[28] = lon.compressed[2];
  
  return true;
}

string CpREMUSCodec::decode_mdat_position(vector<unsigned int> c, int node, string sender, string type)
{

  string sType;

  switch (c[1]) {
  case PS_DISABLED:
    sType="Disabled";
    break;
  case PS_SHIPS_POLE:
    sType="Ship's pole";
    break;
  case PS_GATEWAY_BUOY:
    sType="Gateway Buoy";
    break;
  case PS_NAMED_TRANSPONDER:
    sType="Transponder";
    break;
  case PS_VEHICLE_POSITION:
    sType="Vehicle position";
    break;
  case PS_LAST:
    sType="Last";
    break;
  default:
    sType="Invalid";
  }

    LATLON_COMPRESSED lat, lon;
    lat.compressed[0] = c[2];
    lat.compressed[1] = c[3];
    lat.compressed[2] = c[4];

    lon.compressed[0] = c[5];
    lon.compressed[1] = c[6];
    lon.compressed[2] = c[7];

    double dlat = Decode_latlon(lat);

    double dlon = Decode_latlon(lon);

    double x,y;
    // convert lat, long into x, y. 60 nautical miles per minute
    m_geodesy.LatLong2LocalUTM(dlat, dlon, y, x);

    // No speed heading depth info
    double dspd = 0;
    double dhdg = 0;
    double ddepth = 0;
    // Create human-readable version

    string ccl_status = "MessageType=CCL_POSITION" ;
    ccl_status += ",Node=" + as<string>(node);
    ccl_status += ",Type=" + sType;
    ccl_status += ",Timestamp=" + as<string>(MOOSTime());
    ccl_status += ",Latitude=" + as<string>(dlat);
    ccl_status += ",Longitude=" + as<string>(dlon);
    // make sure label is terminated
    //c[28] = 0;
    ccl_status += ",Label=" ;
    for (unsigned int i=15; i<28; i++)
      {
	unsigned char cc = c[i];
	ccl_status.push_back(cc);
      }

     m_Comms.Notify("CCL_POSITION_IN",ccl_status);

   
    // Create contact variables for collision avoidance

    MOOSTrace("Creating Contact fields\n");
    
    string contact_name = boost::to_upper_copy(sender);
    string moosvar = contact_name + "_NAV_UTC";
    m_Comms.Notify(moosvar,MOOSTime());
    moosvar = contact_name+"_NAV_X";
    m_Comms.Notify(moosvar,x);
    moosvar = contact_name+"_NAV_Y";
    m_Comms.Notify(moosvar,y);
    moosvar = contact_name+"_NAV_LAT";
    m_Comms.Notify(moosvar,dlat);
    moosvar = contact_name+"_NAV_LONG";
    m_Comms.Notify(moosvar,dlon);
    
    return assemble_NODE_REPORT(sender,
                       type,
                       as<string>(-1),
                       as<string>(MOOSTime()),
                       as<string>(x),
                       as<string>(y),
                       as<string>(dlat),
                       as<string>(dlon),
                       as<string>(dspd),
                       as<string>(dhdg),
                       as<string>(ddepth),as<string>(dhdg-360));

}

bool CpREMUSCodec::encode_mdat_position(vector<unsigned int>& c)
{

  for (unsigned int i=0; i<c.size(); i++)
    c[i] = 0x0000;
 
  c[0] = MDAT_POSITION;
  c[1] = PS_VEHICLE_POSITION;

  LATLON_COMPRESSED lat, lon;

  lat = Encode_latlon( nav_lat );
  
  lon = Encode_latlon( nav_lon );
  
  c[2] = lat.compressed[0];
  c[3] = lat.compressed[1];
  c[4] = lat.compressed[2];
  
  c[5] = lon.compressed[0];
  c[6] = lon.compressed[1];
  c[7] = lon.compressed[2];
  
  return true;
}

string CpREMUSCodec::decode_mdat_alert(vector<unsigned int> c, int node, string sender, string type)
{
  // OASIS Timestamp
  unsigned long oasis_time = (c[1]) +
    (c[2]<<8) +
    (c[3]<<16) +
    (c[4]<<24) ;	
  
  LATLON_COMPRESSED lat, lon;

  lat.compressed[0]=c[5];
  lat.compressed[1]=c[6];
  lat.compressed[2]=c[7];
  double dlat = Decode_latlon(lat);

  lon.compressed[0]=c[8];
  lon.compressed[1]=c[9];
  lon.compressed[2]=c[10];
  double dlon = Decode_latlon(lon);

  unsigned char hdg = c[11];
  double dhdg = Decode_heading(hdg);

  string ccl_alert = "MessageType=CCL_ALERT" ;
  ccl_alert += ",Node=" + as<string>(node);
  ccl_alert += ",Timestamp=" + as<string>(double(oasis_time));
  ccl_alert += ",Latitude=" + as<string>(dlat);
  ccl_alert += ",Longitude=" + as<string>(dlon);
  ccl_alert += ",Heading=" + as<string>(dhdg);
  ccl_alert += ",Contact1Id=" + as<string>( c[12] + (c[13]*256) );
  ccl_alert += ",Contact1Age=" + as<string>( c[14] );
  ccl_alert += ",Contact1Bearing=" + as<string>( c[15] );
  ccl_alert += ",Contact1Signature1=" + as<string>( c[16] );
  ccl_alert += ",Contact1Signature2=" + as<string>( c[17] );
  ccl_alert += ",Contact2Id=" + as<string>( c[18] + (c[19]*256) );
  ccl_alert += ",Contact2Age=" + as<string>( c[20] );
  ccl_alert += ",Contact2Bearing=" + as<string>( c[21] );
  ccl_alert += ",Contact2Signature1=" + as<string>( c[22] );
  ccl_alert += ",Contact2Signature2=" + as<string>( c[23] );
  ccl_alert += ",Contact3Id=" + as<string>( c[24] + (c[25]*256) );
  ccl_alert += ",Contact3Age=" + as<string>( c[26] );
  ccl_alert += ",Contact3Bearing=" + as<string>( c[27] );
  ccl_alert += ",Contact3Signature1=" + as<string>( c[28] );
  ccl_alert += ",Contact3Signature2=" + as<string>( c[29] );
    
  m_Comms.Notify("CCL_ALERT_IN",ccl_alert);
  
  return(ccl_alert);
}

string CpREMUSCodec::decode_mdat_alert2(vector<unsigned int> c, int node, string sender, string type)
{
  // OASIS Timestamp
  unsigned long oasis_time = (c[1]) +
    (c[2]<<8) +
    (c[3]<<16) +
    (c[4]<<24) ;	
  
  LATLON_COMPRESSED lat, lon;

  lat.compressed[0]=c[5];
  lat.compressed[1]=c[6];
  lat.compressed[2]=c[7];
  double dlat = Decode_latlon(lat);

  lon.compressed[0]=c[8];
  lon.compressed[1]=c[9];
  lon.compressed[2]=c[10];
  double dlon = Decode_latlon(lon);

  unsigned char hdg1 = c[15];
  double dhdg1 = Decode_heading(hdg1);
  string hdgtyp1 = "Vehicle";
  if (c[13] & 0x80)
    hdgtyp1 = "Absolute";

  unsigned char hdg2 = c[22];
  double dhdg2 = Decode_heading(hdg2);
  string hdgtyp2 = "Vehicle";
  if (c[20] & 0x80)
    hdgtyp2 = "Absolute";

  unsigned char hdg3 = c[29];
  double dhdg3 = Decode_heading(hdg3);
  string hdgtyp3 = "Vehicle";
  if (c[27] & 0x80)
    hdgtyp3 = "Absolute";

  string ccl_alert2 = "MessageType=CCL_ALERT2" ;
  ccl_alert2 += ",Node=" + as<string>(node);
  ccl_alert2 += ",Timestamp=" + as<string>(double(oasis_time));
  ccl_alert2 += ",Latitude=" + as<string>(dlat);
  ccl_alert2 += ",Longitude=" + as<string>(dlon);
  ccl_alert2 += ",Contact1Id=" + as<string>( c[11] + (c[12]*256) );
  ccl_alert2 += ",Contact1Age=" + as<string>( c[13] );
  ccl_alert2 += ",Contact1Bearing=" + as<string>( c[14] );
  ccl_alert2 += ",Contact1HeadingType=" + hdgtyp1;
  ccl_alert2 += ",Contact1Heading=" + as<string>(dhdg1);
  ccl_alert2 += ",Contact1Signature1=" + as<string>( c[16] );
  ccl_alert2 += ",Contact1Signature2=" + as<string>( c[17] );
  ccl_alert2 += ",Contact2Id=" + as<string>( c[18] + (c[19]*256) );
  ccl_alert2 += ",Contact2Age=" + as<string>( c[20] );
  ccl_alert2 += ",Contact2Bearing=" + as<string>( c[21] );
  ccl_alert2 += ",Contact2HeadingType=" + hdgtyp2;
  ccl_alert2 += ",Contact2Heading=" + as<string>(dhdg2);
  ccl_alert2 += ",Contact2Signature1=" + as<string>( c[23] );
  ccl_alert2 += ",Contact2Signature2=" + as<string>( c[24] );
  ccl_alert2 += ",Contact3Id=" + as<string>( c[25] + (c[26]*256) );
  ccl_alert2 += ",Contact3Age=" + as<string>( c[27] );
  ccl_alert2 += ",Contact3Bearing=" + as<string>( c[28] );
  ccl_alert2 += ",Contact3HeadingType=" + hdgtyp3;
  ccl_alert2 += ",Contact3Heading=" + as<string>(dhdg3);
  ccl_alert2 += ",Contact3Signature1=" + as<string>( c[30] );
  ccl_alert2 += ",Contact3Signature2=" + as<string>( c[31] );
    
  m_Comms.Notify("CCL_ALERT2_IN",ccl_alert2);
  
  return(ccl_alert2);
}

bool CpREMUSCodec::parseRedirect(string msg, int& node)
{

  string bite;
  double dummy;

  MOOSValFromString(bite,msg,"MessageType");
  if (!MOOSStrCmp(bite,"CCL_REDIRECT"))
    return(false);

  MOOSValFromString(dummy,msg,"DestinationPlatformId");
  node = int(dummy);
  MOOSTrace("parseRedirect: node= %d\n", node);

  MOOSValFromString(dummy,msg,"Timestamp");

  MOOSValFromString(transit_lat,msg,"Latitude");
  MOOSValFromString(transit_lon,msg,"Longitude");
  
  MOOSValFromString(spd_dep_flags,msg,"SpeedDepthFlags");
  MOOSValFromString(transit_depth,msg,"TransitDepth");
  MOOSValFromString(transit_speed,msg,"TransitSpeed");
  MOOSValFromString(dummy,msg,"TransitCommand");
  transit_command = int(dummy);

  MOOSValFromString(survey_depth,msg,"SurveyDepth");
  MOOSValFromString(survey_speed,msg,"SurveySpeed");
  MOOSValFromString(dummy,msg,"SurveyCommand");
  survey_command = int(dummy);
  MOOSValFromString(dummy,msg,"NumberOfRows");
  survey_rows = int(dummy);
  MOOSValFromString(row_length,msg,"RowLength");
  MOOSValFromString(row_spacing_0,msg,"RowSpacing0");
  MOOSValFromString(row_spacing_1,msg,"RowSpacing1");
  MOOSValFromString(row_heading,msg,"RowHeading");

  MOOSValFromString(start_lat,msg,"StartLatitude");
  MOOSValFromString(start_lon,msg,"StartLongitude");

  return(true);
}


