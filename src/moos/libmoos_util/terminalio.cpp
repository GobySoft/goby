// t. schneider tes@mit.edu 05.11.08
// ocean engineering graudate student - mit / whoi joint program
// massachusetts institute of technology (mit)
// laboratory for autonomous marine sensing systems (lamss)      
// 
// this is terminalio.cpp 
//
// this adds a class for output to a terminal for MOOS
// processes
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


// the following three functions add more functionality to the standard
// MOOSTrace() and allow for verbosity to be easily handled.

// EchoInform(std::string message) - used for messages intended to inform
// but not warn the user
// verbose: display message, terse: display heartbeat, quiet: display ""
#include "terminalio.h"

#define NOCOLOR "\33[1;0m"

using namespace std;
using tes::stricmp;

CMOOSTermIO::CMOOSTermIO()
{
    m_verbosity = "verbose";
    m_processname = "Unknown";

    m_colorchart["nocolor"] = "\33[0m";

    m_colorchart["light_red"] = "\33[91m";
    m_colorchart["red"] = "\33[31m";
    
    
    m_colorchart["light_green"] = "\33[92m";
    m_colorchart["green"] = "\33[32m";
    
    
    m_colorchart["light_yellow"] = "\33[93m";
    m_colorchart["yellow"] = "\33[33m";

        
    m_colorchart["light_blue"] = "\33[94m";
    m_colorchart["blue"] = "\33[34m";


    m_colorchart["light_magenta"] = "\33[95m";
    m_colorchart["magenta"] = "\33[35m";

    
    m_colorchart["light_cyan"] = "\33[96m";
    m_colorchart["cyan"] = "\33[36m";

    
    m_colorchart["light_white"] = "\33[97m";
    m_colorchart["white"] = "\33[37m";
}
    
void CMOOSTermIO::SetProcessName(string p_name)
{
    m_processname = p_name;
}


void CMOOSTermIO::SetVerbosity(string verbosity)
{
    m_verbosity = verbosity;
}


// added on 6.28.08 to streamline output to groups with a given color and heartbeat symbol
void CMOOSTermIO::add_output_group(string name, string heartbeat = "", string color = "nocolor", string explanation = "no description")
{
    output_group new_group =
        {heartbeat,
         color,
        explanation};
    
    m_output[name] = new_group;
}

void CMOOSTermIO::EchoInform(string message, string heartbeat, string color)
{
    string start_color = "";
    string end_color = "";
    if(color != "")
    {
        start_color = m_colorchart[color];
        end_color = NOCOLOR;
    }

    if(stricmp(m_verbosity, "verbose"))
    {
        cout <<  m_processname <<  ": " << start_color << message << end_color <<  endl;
    }
    else if(stricmp(m_verbosity, "terse"))
    {
	cout << start_color << heartbeat << end_color << flush;
    }    
}


void CMOOSTermIO::EchoInformC(const char * format, ...)
{
    char buffer[5000];
    va_list args;
    va_start (args, format);
    vsprintf (buffer, format, args);
    EchoInform((string) buffer, (string)"");
    va_end (args);
}


void CMOOSTermIO::EchoGroup(string group, string message)
{
    EchoInform(message, m_output[group].heartbeat, m_output[group].color);
}


void CMOOSTermIO::EchoGroupC(const char * group, const char * format, ...)
{
    char buffer[5000];
    va_list args;
    va_start (args, format);
    vsprintf (buffer,format, args);
    EchoInform((string) buffer, m_output[(string)group].heartbeat, m_output[(string)group].color);
    va_end (args);
}

// EchoWarn(string message) - used for messages intended to warn
// the user but not terminate the script
// verbose: display message, terse: display '!', quiet: display ""
void CMOOSTermIO::EchoWarn(string message)
{
    string message_out = "(Warning): " + message;
    EchoInform(message_out, (string)"!", "light_red");
}
       
void CMOOSTermIO::EchoWarnC(const char * format, ...)
{
    char buffer[5000];
    va_list args;
    
    va_start (args,format);
    vsprintf (buffer,format, args);

    string format2 = "(Warning): ";
    EchoInform(string(format2+buffer), "!", "light_red");
    va_end (args);

}

// EchoDie - used to display a message and then terminate the script
// verbose: display message, terse: display message, quiet: display message
void CMOOSTermIO::EchoDie(string message)
{
    cout << m_processname << ": (Error): " << m_colorchart["red"] << message << NOCOLOR << endl;
    exit(1);
}

void CMOOSTermIO::EchoDieC(const char * format, ...)
{
    char buffer[5000];
    va_list args;
    va_start (args, format);
    vsprintf (buffer,format, args);
    va_end (args);
    EchoDie((string) buffer);
    
}
