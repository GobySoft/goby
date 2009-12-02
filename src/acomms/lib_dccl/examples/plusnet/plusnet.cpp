// t. schneider tes@mit.edu 3.31.09
// ocean engineering graduate student - mit / whoi joint program
// massachusetts institute of technology (mit)
// laboratory for autonomous marine sensing systems (lamss)
// 
// this is two_message_example.cpp
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


// encodes/decodes the message given in the pGeneralCodec documentation
// also includes the simple.xml file to show example of DCCLCodec instantiation
// with multiple files

// this is an example showing some of the "MOOS" related features of libdccl that can be used (if desired) in the absence of MOOS

#include "dccl.h"
#include <exception>
#include <iostream>

using dccl::operator<<;

int main()
{
    std::cout << "loading nafcon xml files" << std::endl;

    dccl::DCCLCodec dccl;
    dccl.add_xml_message_file("../src/acomms/lib_dccl/examples/plusnet/nafcon_command.xml", "../../message_schema.xsd");
    dccl.add_xml_message_file("../src/acomms/lib_dccl/examples/plusnet/nafcon_report.xml", "../../message_schema.xsd");
    
    std::cout << std::string(30, '#') << std::endl
              << "detailed message summary:" << std::endl
              << std::string(30, '#') << std::endl
              << dccl;

    std::cout << std::string(30, '#') << std::endl
              << "ENCODE / DECODE example:" << std::endl
              << std::string(30, '#') << std::endl;

    // initialize input contents to encoder
    std::map<std::string, std::string> strings;
    std::map<std::string, double> doubles;

    // initialize output hexadecimal
    std::string hex;

    strings["PLUSNET_MESSAGES"] = "MessageType=SENSOR_STATUS,SensorReportType=0,SourcePlatformId=3,DestinationPlatformId=0,Timestamp=1191947446.91117,NodeLatitude=47.7448,NodeLongitude=-122.845,NodeDepth=0.26,NodeCEP=0,NodeHeading=169.06,NodeSpeed=0,MissionState=2,MissionType=2,LastGPSTimestamp=1191947440,PowerLife=6,SensorHealth=0,RecorderState=1,RecorderLife=0,NodeSpecificInfo0=0,NodeSpecificInfo1=0,NodeSpecificInfo2=23,NodeSpecificInfo3=0,NodeSpecificInfo4=3,NodeSpecificInfo5=0";

    std::cout << "passing values to encoder:" << std::endl  
              << strings
              << doubles;
    
    dccl.encode_from_moos("SENSOR_STATUS", hex, &strings, &doubles);
    
    std::cout << "received hexadecimal string: "
              << hex
              << std::endl;

    
    strings.clear();
    doubles.clear();
    
    std::cout << "passed hexadecimal string to decoder: " 
              << hex
              << std::endl;

    dccl.decode_to_publish("SENSOR_STATUS", hex, &strings, &doubles);
    
    std::cout << "received values:" << std::endl 
              << strings
              << doubles;
    
    return 0;
}

