// t. schneider tes@mit.edu 11.22.09
// ocean engineering graudate student - mit / whoi joint program
// massachusetts institute of technology (mit)
// laboratory for autonomous marine sensing systems (lamss)
// 
// this is acomms_constants.h
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

#ifndef AcommsConstants20091122H
#define AcommsConstants20091122H

namespace acomms_util
{

   
    const unsigned int BITS_IN_BYTE = 8;
    // one hex char is a nibble (4 bits), two nibbles per byte
    const unsigned int NIBS_IN_BYTE = 2;
    
    const unsigned int BROADCAST_ID = 0;

    // largest allowed id 
    const unsigned int MAX_ID = 63;

    const std::string DCCL_CCL_HEADER_STR = "20";
    const std::bitset<BITS_IN_BYTE> DCCL_CCL_HEADER(32);

    // 00000111
    const unsigned int BYTE_MASK = 7; 

    const double NaN = std::numeric_limits<double>::quiet_NaN();

    // number of frames for a given packet type
    const unsigned PACKET_FRAME_COUNT [] = { 1, 3, 3, 2, 2, 8 };

    // data size
    const unsigned PACKET_SIZE [] = { 32, 32, 64, 256, 256, 256 };


    // probably can put in lib_q at some point
    const unsigned int NUM_HEADER_BYTES = 2;
    const unsigned int MULTIMESSAGE_MASK = 1 << 7;
    const unsigned int BROADCAST_MASK = 1 << 6;
    const unsigned int VAR_ID_MASK = 0xFF ^ MULTIMESSAGE_MASK ^ BROADCAST_MASK;
    
}



#endif
