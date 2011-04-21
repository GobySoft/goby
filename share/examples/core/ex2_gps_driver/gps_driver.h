#ifndef GPSDRIVER20101014H
#define GPSDRIVER20101014H

#include "goby/core.h"
#include "goby/util/time.h"
// for serial driver
#include "goby/util/linebasedcomms.h"
#include "config.pb.h"

// forward declare (from gps_nmea.proto)
class NMEASentence;
class GPSSentenceGGA;

class GPSDriver : public goby::core::ApplicationBase
{
public:
    GPSDriver();
    ~GPSDriver();
    
    
private:
    void loop();
    boost::posix_time::ptime nmea_time2ptime(const std::string& nmea_time);
    void string2nmea_sentence(std::string in, NMEASentence* out);
    void set_gga_specific_fields(GPSSentenceGGA* gga);
    
    goby::util::SerialClient serial_;
    static GPSDriverConfig cfg_;    
};

// very simple exception classes
class bad_nmea_sentence : public std::runtime_error
{
  public:
  bad_nmea_sentence(const std::string& s)
      : std::runtime_error(s)
    { }
};

class bad_gga_sentence : public std::runtime_error
{
  public:
  bad_gga_sentence(const std::string& s)
      : std::runtime_error(s)
    { }
};


#endif
