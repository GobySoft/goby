// Copyright 2009-2014 Toby Schneider (https://launchpad.net/~tes)
//                     GobySoft, LLC (2013-)
//                     Massachusetts Institute of Technology (2007-2014)
//                     Goby Developers Team (https://launchpad.net/~goby-dev)
// 
//
// This file is part of the Goby Underwater Autonomy Project Binaries
// ("The Goby Binaries").
//
// The Goby Binaries are free software: you can redistribute them and/or modify
// them under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// The Goby Binaries are distributed in the hope that they will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Goby.  If not, see <http://www.gnu.org/licenses/>.

#include "pGobyMOOSAppTemplate.h"


using goby::glog;
using namespace goby::common::logger;
using goby::moos::operator<<;

GobyMOOSAppTemplateConfig GobyMOOSAppTemplate::cfg_;
GobyMOOSAppTemplate* GobyMOOSAppTemplate::inst_ = 0;


 
int main(int argc, char* argv[])
{
    return goby::moos::run<GobyMOOSAppTemplate>(argc, argv);
}


GobyMOOSAppTemplate* GobyMOOSAppTemplate::get_instance()
{
    if(!inst_)
        inst_ = new GobyMOOSAppTemplate();
    return inst_;
}

void GobyMOOSAppTemplate::delete_instance()
{
    delete inst_;
}


GobyMOOSAppTemplate::GobyMOOSAppTemplate()
    : GobyMOOSApp(&cfg_)
{
    // example subscription -
    //    handle_db_time called each time mail from DB_TIME is received
    subscribe("DB_TIME", &GobyMOOSAppTemplate::handle_db_time, this);
}


GobyMOOSAppTemplate::~GobyMOOSAppTemplate()
{
    
}



void GobyMOOSAppTemplate::loop()
{
    // example publication
    publish("TEST", MOOSTime());
}

void GobyMOOSAppTemplate::handle_db_time(const CMOOSMsg& msg)
{
    glog.is(VERBOSE) && glog << "Time is: " << std::setprecision(15) << msg.GetDouble() << std::endl;
}

    
