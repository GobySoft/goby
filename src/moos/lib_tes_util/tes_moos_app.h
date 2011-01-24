// t. schneider tes@mit.edu 07.26.10
// ocean engineering graudate student - mit / whoi joint program
// massachusetts institute of technology (mit)
// laboratory for autonomous marine sensing systems (lamss)      
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

#ifndef TESMOOSAPP20100726H
#define TESMOOSAPP20100726H


#include <boost/date_time/posix_time/posix_time.hpp>
#include <map>
#include <boost/function.hpp>
#include <boost/bind.hpp>

#include "goby/moos/lib_nurc_moosapp/NurcMoosApp.h"
#include "dynamic_moos_vars.h"
#include "goby/util/logger.h"


class TesMoosApp : public NurcMoosApp
{   
  public:
    typedef boost::function<void (const CMOOSMsg& msg)> InboxFunc;

    TesMoosApp();
    virtual ~TesMoosApp() { }    
    
    void outbox(CMOOSMsg& msg)
    { m_Comms.Post(msg); }
    
    template<typename T>
        void outbox(const std::string& key, const T& value)
    { m_Comms.Notify(key, value); }
    
    
    goby::util::FlexOstream& logger() { return goby::util::glogger(); }
    tes::DynamicMOOSVars& dynamic_vars() { return dynamic_vars_; }
    double start_time() const { return start_time_; }

    void subscribe(const std::string& var,
                   InboxFunc handler = InboxFunc(),
                   int blackout = 0);
    
    void subscribe(const std::string& var,
                   int blackout)
    { subscribe(var, InboxFunc(), blackout); }
    
        
    template<typename V, typename A1>
        void subscribe(const std::string& var,
                       void(V::*mem_func)(A1),
                       V* obj,
                       int blackout = 0)
    { subscribe(var, boost::bind(mem_func, obj, _1), blackout); }    

    
    
  protected:
    virtual void loop() = 0;
    virtual void inbox(const CMOOSMsg& msg) { }    
    virtual void read_configuration(CProcessConfigReader& config) = 0;
    virtual void do_subscriptions() = 0;
    
    
  private:
// from NurcMoosApp
    // from NurcMoosApp
    bool registerMoosVariables ()
    { do_subscriptions(); return true; }
    
    bool readMissionParameters (CProcessConfigReader& processConfigReader);

    // from CMOOSApp
    bool Iterate();
    bool OnNewMail(MOOSMSG_LIST &NewMail);


  private:
    
    // when we started (seconds since UNIX)
    double start_time_;

    // have we read the configuration file fully?
    bool configuration_read_;
    bool cout_cleared_;

    // log file for internal driver
    std::ofstream fout_;

    // allows direct reading of newest publish to a given MOOS variable
    tes::DynamicMOOSVars dynamic_vars_;

    std::map<std::string, boost::function<void (const CMOOSMsg& msg)> > mail_handlers_;
    
};


#endif
