// Copyright 2009-2012 Toby Schneider (https://launchpad.net/~tes)
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


#include "Simulator.h"
#include <math.h>

//default constructor
CSimulator::CSimulator()
{
}

//default (virtual) destructor
CSimulator::~CSimulator()
{
}

/** 
Called by base class whenever new mail has arrived.
Place your code for handling mail (notifications that something
has changed in the MOOSDB in this function

Parameters:
    NewMail :  std::list<CMOOSMsg> reference

Return values:
    return true if everything went OK
    return false if there was problem
**/
bool CSimulator::OnNewMail(MOOSMSG_LIST &NewMail)
{
    return true;
}

/**
called by the base class when the application has made contact with
the MOOSDB and a channel has been opened. Place code to specify what
notifications you want to receive here.
**/
bool CSimulator::OnConnectToServer()
{
    return true;
}

/** Called by the base class periodically. This is where you place code
which does the work of the application **/
bool CSimulator::Iterate()
{ 
    static int k = 0;
    if(k++%10==0)
    {
    //simulate some brownian motion
    static double dfHeading = 0;
    dfHeading+=MOOSWhiteNoise(0.1); 

    //publish the data (2nd param is a double so it will be forever double data...)
    m_Comms.Notify("Heading",dfHeading,MOOSTime());
    }
    if(k%35==0)
    {
    
    static double dfVolts  = 100;
    dfVolts-=fabs(MOOSWhiteNoise(0.1));
    std::string sStatus = MOOSFormat("Status=%s,BatteryVoltage=%.2f,Bilge = %s",
                     dfVolts>50.0? "Good":"Bad",
                     dfVolts,
                     k%100>50?"On":"Off");

    //publish the data (2nd param is a std::string so it will be forever string data...)
    m_Comms.Notify("VehicleStatus",sStatus,MOOSTime());
    }
    return true;
}

/** called by the base class before the first ::Iterate is called. Place
startup code here - especially code which reads configuration data from the 
mission file **/
bool CSimulator::OnStartUp()
{       
    return true;
}
