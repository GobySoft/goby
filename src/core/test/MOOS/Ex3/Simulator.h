// Ex3/Simulator.h: interface for the  class.

#ifndef SIMULATORH
#define SIMULATORH

#include <MOOSLIB/MOOSApp.h>
#include "goby/core/libmoosmimic/moos_mimic.h"

class CSimulator : public GobyCMOOSApp
{
public:
    //standard construction and destruction
    CSimulator();
    virtual ~CSimulator();

protected:
    //where we handle new mail
    bool OnNewMail(MOOSMSG_LIST &NewMail);
    //where we do the work
    bool Iterate();
    //called when we connect to the server
    bool OnConnectToServer();
    //called when we are starting up..
    bool OnStartUp();

};

#endif 
