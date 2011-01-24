// t. schneider tes@mit.edu 06.05.08
// ocean engineering graudate student - mit / whoi joint program
// massachusetts institute of technology (mit)
// laboratory for autonomous marine sensing systems (lamss)
// 
// this is pAcommsHandlerMain.cpp, part of pAcommsHandler
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

#include "pAcommsHandler.h"
 
int main(int argc, char* argv[])
{
    // default parameters file
    const char* sMissionFile = "pAcommsHandler.moos";
        
    // under what name should the application register with the MOOSDB?
    const char* sMOOSName = "pAcommsHandler";
  
    switch(argc)
    {
        case 3:
            // command line says don't register with default name
            sMOOSName = argv[2];
        case 2:
            // command line says don't use default config file
            sMissionFile = argv[1];
    }

    
    // make an application
    CpAcommsHandler app;
    
    // run it
    try
    {
        app.Run(sMOOSName,sMissionFile);
    }
    catch(std::exception& e)
    {
        app.logger() << die << "Uncaught exception! what(): " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
