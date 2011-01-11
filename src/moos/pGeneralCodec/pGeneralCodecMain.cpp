// t. schneider tes@mit.edu 11.19.08
// ocean engineering graduate student - mit / whoi joint program
// massachusetts institute of technology (mit)
// laboratory for autonomous marine sensing systems (lamss)
// 
// this is pGeneralCodecMain.cpp 
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

#include "pGeneralCodec.h"
 
int main(int argc, char * argv[])
{
    // default parameters file
    std::string sMissionFile = "pGeneralCodec.moos";
    // under what name should the application register with the MOOSDB?
    std::string sMOOSName = "pGeneralCodec";
    // possible flag
    std::string flag = "";

    switch(argc)
    {        
        case 4:
            if(argv[3][0] == '-')
                flag = argv[3];
            else
                sMOOSName = argv[3];
            
        case 3:
            if(argv[2][0] == '-')
                flag = argv[2];
            else if (argv[1][0] == '-')
                sMissionFile = argv[2];
            else
                sMOOSName = argv[2];
            
        case 2:
            if(argv[1][0] == '-')
                flag = argv[1];            
            else
                sMissionFile = argv[1];
    }

    
    // make an application
    CpGeneralCodec pGeneralCodecApp;
    
    // run it
    try
    {
        pGeneralCodecApp.Run(sMOOSName.c_str(),sMissionFile.c_str());
    }
    catch(std::exception& e)
    {
        pGeneralCodecApp.logger() << die << "Uncaught exception! what(): " << e.what() << std::endl;
        return 1;
    }

    return EXIT_SUCCESS;
}
