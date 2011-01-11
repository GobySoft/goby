// t. schneider tes@mit.edu 11.19.08
// ocean engineering graduate student - mit / whoi joint program
// massachusetts institute of technology (mit)
// laboratory for autonomous marine sensing systems (lamss)
// 
// this is pGeneralCodec.h, part of pGeneralCodec
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

#ifndef pGeneralCodecH
#define pGeneralCodecH

#include "MOOSLIB/MOOSApp.h"
#include "goby/moos/lib_nurc_moosapp/NurcMoosApp.h"

#include "goby/moos/lib_moos_dccl/moos_dccl_codec.h"
#include "goby/moos/lib_tes_util/dynamic_moos_vars.h"

class CpGeneralCodec : public TesMoosApp
{
  public:
    CpGeneralCodec();
    virtual ~CpGeneralCodec() {}
    
  private:
    void loop();
    void read_configuration(CProcessConfigReader& processConfigReader);    
    void do_subscriptions();

  private:
    // do all the real work
    MOOSDCCLCodec moos_dccl_;    
};

#endif

