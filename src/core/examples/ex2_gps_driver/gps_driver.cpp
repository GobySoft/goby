#include "gps_driver.h"

#include "gps_nmea.pb.h"
#include "node_report.pb.h"

GPSDriverConfig GPSDriver::cfg_;

int main(int argc, char* argv[])
{
    return goby::run<GPSDriver>(argc, argv);
}

GPSDriver::GPSDriver()
    : goby::core::ApplicationBase(&cfg_),
      serial_(cfg_.serial_port(), cfg_.serial_baud(), cfg_.end_line())
{
    serial_.start();
}

GPSDriver::~GPSDriver()
{
    serial_.close();
}

void GPSDriver::loop()
{
    std::string in;
    while(serial_.readline(&in))
    {
        glogger() << "raw NMEA: " << in << std::flush;
        goby::util::NMEASentence nmea(in);

        if(nmea.sentence_id() == "GGA")
        {
            glogger() << "This is a GGA type message." << std::endl;
            GPSSentenceGGA gga;
            gga.mutable_nmea()->set_talker_id(nmea.talker_id());
            gga.mutable_nmea()->set_sentence_id(nmea.sentence_id());
            gga.mutable_nmea()->set_checksum(
                goby::util::NMEASentence::checksum(nmea.message()));
            for(int i = 0, n = nmea.size(); i < n; ++i)
                gga.mutable_nmea()->add_part(nmea[i]);

            boost::posix_time::ptime t = nmea_time2ptime(nmea.at(1));            
            gga.mutable_header()->set_unix_time(goby::util::ptime2unix_double(t));
            gga.mutable_header()->set_iso_time(boost::posix_time::to_iso_string(t));
            
            glogger() << gga << std::flush;
        }
    }
}


boost::posix_time::ptime GPSDriver::nmea_time2ptime(const std::string& nmea_time)
{   
    using namespace boost::posix_time;
    using namespace boost::gregorian;

    if(!(nmea_time.length() == 6))
        return ptime(not_a_date_time);  
    else
    {
        // HHMMSS
        std::string s_hour = nmea_time.substr(0,2),
            s_min = nmea_time.substr(2,2),
            s_sec = nmea_time.substr(4,2);
        
        try
        {
            int hour = boost::lexical_cast<int>(s_hour);
            int min = boost::lexical_cast<int>(s_min);
            int sec = boost::lexical_cast<int>(s_sec);
            
            return (ptime(date(day_clock::universal_day()), time_duration(hour, min, sec, 0)));
        }
        catch (boost::bad_lexical_cast&)
        {
            return ptime(not_a_date_time);
        }        
    }
}



