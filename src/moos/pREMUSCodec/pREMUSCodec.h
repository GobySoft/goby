// t. schneider tes@mit.edu 07.25.08
// ocean engineering graduate student - mit / whoi joint program
// massachusetts institute of technology (mit)
// laboratory for autonomous marine sensing systems (lamss)
// 
// this is pREMUSCodec.h 
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

#ifndef pREMUSCODECH
#define pREMUSCODECH

#include<string>
#include<math.h>
#include<vector>

#include "MOOSLIB/MOOSLib.h"
#include "WhoiUtil.h"
#include "MOOSUtilityLib/MOOSGeodesy.h"

// CCL frame types

#define MDAT_REDIRECT       (7)     /* VIP */
#define MDAT_POSITION       (8)     /* VIP */
#define MDAT_STATE          (14)    /* REMUS */
#define MDAT_RANGER         (16)    /* REMUS */
#define MDAT_OASIS          (31)    /* OASIS Array data */

class CpREMUSCodec : public CMOOSApp  
{
  public:
    CpREMUSCodec();
    virtual ~CpREMUSCodec();

  private:
    bool OnNewMail(MOOSMSG_LIST &NewMail);
    bool Iterate();
    bool OnConnectToServer();
    bool OnStartUp();

    void RegisterVariables();
    bool ReadConfiguration();
    bool hex_to_int_array(std::string h, std::vector<unsigned int>& c );
    std::string int_array_to_hex(std::vector<unsigned int> c );
    std::string decode_mdat_state(std::vector<unsigned int>  c , int node, std::string name, std::string type);
    std::string decode_mdat_ranger( std::vector<unsigned int>  c , int node, std::string name, std::string type);
    std::string decode_mdat_redirect(std::vector<unsigned int>  c , int node, std::string name, std::string type);
    std::string decode_mdat_position(std::vector<unsigned int>  c , int node, std::string name, std::string type);
    std::string decode_mdat_alert(std::vector<unsigned int>  c , int node, std::string name, std::string type);
    std::string decode_mdat_alert2(std::vector<unsigned int>  c , int node, std::string name, std::string type);
    bool encode_mdat_state(std::vector<unsigned int>&  c);
    bool encode_mdat_redirect(std::vector<unsigned int>&  c);
    bool encode_mdat_position(std::vector<unsigned int>&  c);
    bool parseRedirect(std::string msg, int& node);

    std::string mdat_state_var;
    std::string mdat_state_out;
    std::string mdat_ranger_var;
    std::string mdat_ranger_out;
    std::string mdat_redirect_var;
    std::string mdat_redirect_out;
    std::string mdat_alert_var;
    std::string mdat_alert_out;
    std::string mdat_alert2_var;
    std::string mdat_alert2_out;

		// Replaces assembleAIS mfallon
    std::string assemble_NODE_REPORT(std::string,std::string,std::string, \
														std::string,std::string,std::string,std::string, \
														std::string,std::string,std::string,std::string, \
														std::string);

    struct vehicle_nametype 
    {
        std::string name;
        std::string type;
    };
    // lookup read from a file for modem id -> name and type
    std::map<int, vehicle_nametype> modem_lookup;
    unsigned int my_id;
    // for lat long conversion
    CMOOSGeodesy m_geodesy;

    // HS 090828 Added creation of CCL status report for testing
    double nav_x;
    double nav_y;
    double nav_lat;
    double nav_lon;
    double nav_depth;
    double nav_speed;
    double nav_heading;

    bool got_x;
    bool got_y;
    bool got_depth;
    bool got_speed;
    bool got_heading;
    bool remus_status;
    bool west;
    bool south;

    double status_interval;
    double status_time;

    double transit_lat;
    double transit_lon;
    std::string spd_dep_flags;
    double transit_speed;
    double transit_depth;
    unsigned short transit_command;

    double start_lat;
    double start_lon;
    double survey_speed;
    double survey_depth;
    double row_heading;
    double row_length;
    double row_spacing_0;
    double row_spacing_1;

    unsigned short survey_rows;
    unsigned short survey_command;




};

#endif 
