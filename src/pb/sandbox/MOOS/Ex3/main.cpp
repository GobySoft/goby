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

 //simple "main" file which serves to build and run a CMOOSApp-derived application


int main(int argc ,char * argv[])
{
    //set up some default application parameters

    //what's the name of the configuration file that the application
    //should look in if it needs to read parameters?
    const char * sMissionFile = "Mission.moos";

    //under what name shoud the application register with the MOOSDB?
    const char * sMOOSName = "Simulator";

    switch(argc)
    {
    case 3:
    //command line says don't register with default name
    sMOOSName = argv[2];
    case 2:
    //command line says don't use default "mission.moos" config file
    sMissionFile = argv[1];
    }

    //make an application
    CSimulator TheApp;

    //run forever pasing registration name and mission file parameters
    TheApp.Run(sMOOSName,sMissionFile);

    //probably will never get here..
    return 0;
}
