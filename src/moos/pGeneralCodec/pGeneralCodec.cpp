// t. schneider tes@mit.edu 11.19.08
// ocean engineering graduate student - mit / whoi joint program
// massachusetts institute of technology (mit)
// laboratory for autonomous marine sensing systems (lamss)
// 
// this is pGeneralCodec.cpp, part of pGeneralCodec
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
#include "goby/moos/lib_moos_dccl/moos_dccl_codec.h"

CpGeneralCodec::CpGeneralCodec()
    : moos_dccl_(this)
{ }

void CpGeneralCodec::loop()
{ moos_dccl_.iterate(); }


// readMissionParameters: reads in configuration values from the .moos file
// parameter keys are case insensitive
void CpGeneralCodec::read_configuration(CProcessConfigReader& processConfigReader)
{
    moos_dccl_.add_flex_groups();
    moos_dccl_.read_parameters(processConfigReader);
}

// registerMoosVariables: register for variables we want to get mail for
void CpGeneralCodec::do_subscriptions()
{    
    moos_dccl_.register_variables();
}

