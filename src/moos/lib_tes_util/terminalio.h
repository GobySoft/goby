// t. schneider tes@mit.edu 05.11.08
// ocean engineering graudate student - mit / whoi joint program
// massachusetts institute of technology (mit)
// laboratory for autonomous marine sensing systems (lamss)      
// 
// this is terminalio.h 
//
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
#include <iostream>
#include <stdio.h>
#include <stdarg.h>
#include "tes_string.h"
#include <map>

#ifndef TERMINALIOH
#define TERMINALIOH


class CMOOSTermIO 
{
  public:
    CMOOSTermIO();
    
    void SetProcessName(std::string);
    void SetVerbosity(std::string);    
    
    void EchoInform(std::string message, std::string heartbeat = "", std::string color = "");
    void EchoInformC(const char * format, ...);
    void EchoWarn(std::string message);
    void EchoWarnC(const char * format, ...);
    void EchoDie(std::string message);
    void EchoDieC(const char * format, ...);
    
    void add_output_group(std::string name, std::string heartbeat, std::string color, std::string explanation);
    void EchoGroupC(const char * group, const char * format, ...);
    void EchoGroup(std::string group, std::string message);
    
  private:    
    std::string m_verbosity;
    std::string m_processname;
    
    std::map<std::string,std::string> m_colorchart;

    struct output_group
    {
        std::string heartbeat;
        std::string color;
        std::string explanation;
    };
        
    std::map<std::string,output_group> m_output;
    
};



#endif
