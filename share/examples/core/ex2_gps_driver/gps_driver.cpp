#include "gps_driver.h"

#include "gps_nmea.pb.h"

#include "goby/util/binary.h" // for goby::util::hex_string2number
#include "goby/util/string.h" // for goby::util::as

using goby::core::operator<<;

GPSDriverConfig GPSDriver::cfg_;

int main(int argc, char* argv[])
{
    return goby::run<GPSDriver>(argc, argv);
}

GPSDriver::GPSDriver()
    : goby::core::Application(&cfg_),
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
        goby::glog << "raw NMEA: " << in << std::flush;
        
        // parse
        NMEASentence nmea;
        try
        {
            string2nmea_sentence(in, &nmea);        
        }
        catch (bad_nmea_sentence& e)
        {
            goby::glog << warn << "bad NMEA sentence: " << e.what()
                       << std::endl;
        }

        if(nmea.sentence_id() == "GGA")
        {
            goby::glog << "This is a GGA type message." << std::endl;

            // create the message we send on the wire
            GPSSentenceGGA gga;
            // copy the raw message (in case later users want to do their
            // own parsing)
            gga.mutable_nmea()->CopyFrom(nmea);
            
            try
            {
                set_gga_specific_fields(&gga);

                // parse the time stamp
                using goby::util::as;
                gga.mutable_header()->set_time(as<std::string>(nmea_time2ptime(nmea.part(1))));
                goby::glog << gga << std::flush;
                
                publish(gga);
            }
            catch(bad_gga_sentence& e)
            {
                goby::glog << warn << "bad GGA sentence: " << e.what()
                           << std::endl;
            }
            
        }
        
    }
}


// from http://www.gpsinformation.org/dale/nmea.htm#GGA
// GGA - essential fix data which provide 3D location and accuracy data.
// $GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47
// Where:
//      GGA          Global Positioning System Fix Data
//      123519       Fix taken at 12:35:19 UTC
//      4807.038,N   Latitude 48 deg 07.038' N
//      01131.000,E  Longitude 11 deg 31.000' E
//      1            Fix quality: 0 = invalid
//                                1 = GPS fix (SPS)
//                                2 = DGPS fix
//                                3 = PPS fix
// 			          4 = Real Time Kinematic
// 			          5 = Float RTK
//                                6 = estimated (dead reckoning)
//                                    (2.3 feature)
// 			          7 = Manual input mode
// 			          8 = Simulation mode
//      08           Number of satellites being tracked
//      0.9          Horizontal dilution of position
//      545.4,M      Altitude, Meters, above mean sea level
//      46.9,M       Height of geoid (mean sea level) above WGS84
//                       ellipsoid
//      (empty field) time in seconds since last DGPS update
//      (empty field) DGPS station ID number
//      *47          the checksum data, always begins with *
// If the height of geoid is missing then the altitude should be suspect.
// Some non-standard implementations report altitude with respect to the
// ellipsoid rather than geoid altitude. Some units do not report negative
// altitudes at all. This is the only sentence that reports altitude.

void GPSDriver::set_gga_specific_fields(GPSSentenceGGA* gga)
{
    using goby::util::as;
    const NMEASentence& nmea = gga->nmea();
    
    const std::string& lat_string = nmea.part(2);
                
    if(lat_string.length() > 2)
    {
        double lat_deg = as<double>(lat_string.substr(0, 2));
        double lat_min = as<double>(lat_string.substr(2, lat_string.size()));
        double lat = lat_deg + lat_min / 60;
        gga->set_lat((nmea.part(3) == "S") ? -lat : lat);
    }
    else
    {
        throw(bad_gga_sentence("invalid latitude field"));
    }
                
    const std::string& lon_string = nmea.part(4);
    if(lon_string.length() > 2)
    {
        double lon_deg = as<double>(lon_string.substr(0, 3));
        double lon_min = as<double>(lon_string.substr(3, nmea.part(4).size()));
        double lon = lon_deg + lon_min / 60;
        gga->set_lon((nmea.part(5) == "W") ? -lon : lon);
    }
    else
        throw(bad_gga_sentence("invalid longitude field: " + nmea.part(4)));

    switch(goby::util::as<int>(nmea.part(6)))
    {
        default:
        case 0: gga->set_fix_quality(GPSSentenceGGA::INVALID); break;
        case 1: gga->set_fix_quality(GPSSentenceGGA::GPS_FIX); break;
        case 2: gga->set_fix_quality(GPSSentenceGGA::DGPS_FIX); break;
        case 3: gga->set_fix_quality(GPSSentenceGGA::PPS_FIX); break;
        case 4: gga->set_fix_quality(GPSSentenceGGA::REAL_TIME_KINEMATIC);
            break;
        case 5: gga->set_fix_quality(GPSSentenceGGA::FLOAT_RTK); break;
        case 6: gga->set_fix_quality(GPSSentenceGGA::ESTIMATED); break;
        case 7: gga->set_fix_quality(GPSSentenceGGA::MANUAL_MODE); break;
        case 8: gga->set_fix_quality(GPSSentenceGGA::SIMULATION_MODE); break;
    }

    gga->set_num_satellites(goby::util::as<int>(nmea.part(7)));
    gga->set_horiz_dilution(goby::util::as<float>(nmea.part(8)));
    gga->set_altitude(goby::util::as<double>(nmea.part(9)));
    gga->set_geoid_height(goby::util::as<double>(nmea.part(11)));
}

// converts a NMEA0183 sentence string into a class representation
void GPSDriver::string2nmea_sentence(std::string in, NMEASentence* out)
{
    
    // Silently drop leading/trailing whitespace if present.
    boost::trim(in);

    // Basic error checks ($, empty)
    if (in.empty())
      throw bad_nmea_sentence("message provided.");
    if (in[0] != '$')
        throw bad_nmea_sentence("no $: '" + in + "'.");
    // Check if the checksum exists and is correctly placed, and strip it.
    // If it's not correctly placed, we'll interpret it as part of message.
    // NMEA spec doesn't seem to say that * is forbidden elsewhere?
    // (should be)
    if (in.size() > 3 && in.at(in.size()-3) == '*') {
      std::string hex_csum = in.substr(in.size()-2);
      int cs;
      if(goby::util::hex_string2number(hex_csum, cs))
          out->set_checksum(cs);
      in = in.substr(0, in.size()-3);
    }
    
    // Split string into parts.
    size_t comma_pos = 0, last_comma_pos = 0;
    while((comma_pos = in.find(",", last_comma_pos)) != std::string::npos)
    {
        out->add_part(in.substr(last_comma_pos, comma_pos-last_comma_pos));

        // +1 moves us past the comma
        last_comma_pos = comma_pos + 1;
    }    
    out->add_part(in.substr(last_comma_pos));
    
    // Validate talker size.
    if (out->part(0).size() != 6)
        throw bad_nmea_sentence("bad talker length '" + in + "'.");

    // GP
    out->set_talker_id(out->part(0).substr(1, 2));
    // GGA
    out->set_sentence_id(out->part(0).substr(3));

}


// converts the time stamp used by GPS messages of the format HHMMSS.SSS
// for arbitrary precision fractional
// seconds into a boost::ptime object (much more usable class
// representation of for dates and times)
// *CAVEAT* this assumes that the message was received "today" for the
// date part of the returned ptime.
boost::posix_time::ptime GPSDriver::nmea_time2ptime(const std::string& mt)
{   
    using namespace boost::posix_time;
    using namespace boost::gregorian;

    // must be at least HHMMSS
    if(mt.length() < 6)
        return ptime(not_a_date_time);  
    else
    {
        std::string s_hour = mt.substr(0,2), s_min = mt.substr(2,2),
            s_sec = mt.substr(4,2), s_fs = "0";

        // has some fractional seconds
        if(mt.length() > 7)
            s_fs = mt.substr(7); // everything after the "."
	        
        try
        {
            int hour = boost::lexical_cast<int>(s_hour);
            int min = boost::lexical_cast<int>(s_min);
            int sec = boost::lexical_cast<int>(s_sec);
            int micro_sec = boost::lexical_cast<int>(s_fs)*
                pow(10, 6-s_fs.size());
            
            return (ptime(date(day_clock::universal_day()),
                          time_duration(hour, min, sec, 0)) +
                    microseconds(micro_sec));
        }
        catch (boost::bad_lexical_cast&)
        {
            return ptime(not_a_date_time);
        }        
    }
}

