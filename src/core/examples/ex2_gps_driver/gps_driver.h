#ifndef GPSDRIVER20101014H
#define GPSDRIVER20101014H

#include "goby/core/core.h"
#include "goby/util/time.h"
// for serial driver
#include "goby/util/linebasedcomms.h"
#include "config.pb.h"

class GPSDriver : public goby::core::ApplicationBase
{
public:
    GPSDriver();
    ~GPSDriver();
    
    
private:
    void loop();
    static boost::posix_time::ptime nmea_time2ptime(const std::string& nmea_time);
    
    goby::util::SerialClient serial_;
    static GPSDriverConfig cfg_;    
};

#endif
