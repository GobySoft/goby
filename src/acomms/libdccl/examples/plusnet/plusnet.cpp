// t. schneider tes@mit.edu 3.31.09
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

#include "goby/acomms/dccl.h"

#include <exception>
#include <iostream>

using namespace goby;
using goby::acomms::operator<<;

int main()
{
    std::cout << "loading nafcon xml files" << std::endl;

    acomms::DCCLCodec dccl;
    dccl.add_xml_message_file(DCCL_EXAMPLES_DIR "/plusnet/nafcon_command.xml", "../../message_schema.xsd");
    dccl.add_xml_message_file(DCCL_EXAMPLES_DIR "/plusnet/nafcon_report.xml", "../../message_schema.xsd");
    
    std::cout << std::string(30, '#') << std::endl
              << "detailed message summary:" << std::endl
              << std::string(30, '#') << std::endl
              << dccl;

    std::cout << std::string(30, '#') << std::endl
              << "ENCODE / DECODE example:" << std::endl
              << std::string(30, '#') << std::endl;

    // initialize input contents to encoder
    std::map<std::string, acomms::DCCLMessageVal> in_vals;

    // initialize output message
    acomms::ModemMessage msg;

    in_vals["PLUSNET_MESSAGES"] = "MessageType=SENSOR_STATUS,SensorReportType=0,SourcePlatformId=1,DestinationPlatformId=3,Timestamp=1191947446.91117,NodeLatitude=47.7448,NodeLongitude=-122.845,NodeDepth=0.26,NodeCEP=0,NodeHeading=169.06,NodeSpeed=0,MissionState=2,MissionType=2,LastGPSTimestamp=1191947440,PowerLife=6,SensorHealth=0,RecorderState=1,RecorderLife=0,NodeSpecificInfo0=0,NodeSpecificInfo1=0,NodeSpecificInfo2=23,NodeSpecificInfo3=0,NodeSpecificInfo4=3,NodeSpecificInfo5=0";

    std::cout << "passing values to encoder:" << std::endl  
              << in_vals;
    
    dccl.pubsub_encode("SENSOR_STATUS", msg, in_vals);
    
    std::cout << "received acomms::DCCLMessage: "
              << msg.serialize()
              << std::endl;

    
    std::multimap<std::string, acomms::DCCLMessageVal> out_vals;

    
    std::cout << "passed acomms::DCCLMessage to decoder: " 
              << msg.serialize()
              << std::endl;

    dccl.pubsub_decode("SENSOR_STATUS", msg, out_vals);
    
    std::cout << "received values:" << std::endl 
              << out_vals;
    
    
    return 0;
}

