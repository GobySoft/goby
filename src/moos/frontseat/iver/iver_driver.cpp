// Copyright 2009-2017 Toby Schneider (http://gobysoft.org/index.wt/people/toby)
//                     GobySoft, LLC (2013-)
//                     Massachusetts Institute of Technology (2007-2014)
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

#include <boost/units/base_units/metric/knot.hpp>
#include <boost/units/base_units/imperial/foot.hpp>

#include "goby/common/logger.h"
#include "goby/util/as.h"
#include "goby/util/linebasedcomms/nmea_sentence.h"

#include "iver_driver.h"

namespace gpb = goby::moos::protobuf;
using goby::common::goby_time;
using goby::glog;
using namespace goby::common::logger;

const int allowed_skew = 10;

extern "C"
{
    FrontSeatInterfaceBase* frontseat_driver_load(iFrontSeatConfig* cfg)
    {
        return new IverFrontSeat(*cfg);
    }
}

IverFrontSeat::IverFrontSeat(const iFrontSeatConfig& cfg)
    : FrontSeatInterfaceBase(cfg),
      iver_config_(cfg.GetExtension(iver_config)),
      serial_(iver_config_.serial_port(),
	      iver_config_.serial_baud(), "\r\n"),
      frontseat_providing_data_(false),
      last_frontseat_data_time_(0),
      frontseat_state_(gpb::FRONTSEAT_NOT_CONNECTED)
{
    goby::util::NMEASentence::enforce_talker_length = false;
    
    serial_.start();

}

void IverFrontSeat::loop()
{
    try_receive();

    goby::util::NMEASentence request_data("$OSD,G,C,S,P,,,,");
    serial_.write(request_data.message_cr_nl());
    
    if(goby_time<double>() > last_frontseat_data_time_ + allowed_skew)
        frontseat_providing_data_ = false;
} 


void IverFrontSeat::try_receive()
{
    std::string in;
    while(serial_.readline(&in))
    {
        boost::trim(in);
        try
        {          
            process_receive(in);
        }
        catch(std::exception& e)
        {
            glog.is(DEBUG1) && glog << warn << "Failed to handle message: " << e.what() << std::endl;
        }            
    }
}    


void IverFrontSeat::process_receive(const std::string& s)
{
    gpb::FrontSeatRaw raw_msg;
    raw_msg.set_raw(s);
    signal_raw_from_frontseat(raw_msg);

    // parse
    try
    {
	goby::util::NMEASentence nmea(s);

	if(nmea.sentence_id() == "RMC")
	{
	    enum RMCFields {
		TALKER = 0,
		UTC = 1,
		VALIDITY = 2,
		LAT = 3,
		LAT_HEMI = 4,
		LON = 5,
		LON_HEMI = 6,
		SOG_KNOTS = 7,
		TRACK_DEGREES = 8,
		UT_DATE = 9,
		MAG_VAR_DEG = 10,
		MAG_VAR_HEMI = 11,
		RMC_SIZE = 12};

	    if(nmea.size() < RMC_SIZE)
		throw(goby::util::bad_nmea_sentence("Message too short"));

	    status_.Clear(); // $GPRMC clears the message, $C sends it	   
	    
	    status_.set_time_with_units(nmea_time_to_seconds(nmea.as<double>(UTC), nmea.as<int>(UT_DATE)));
	    status_.mutable_global_fix()->set_lat_with_units(nmea_geo_to_degrees(nmea.as<double>(LAT), nmea.as<char>(LAT_HEMI)));
	    status_.mutable_global_fix()->set_lon_with_units(nmea_geo_to_degrees(nmea.as<double>(LON), nmea.as<char>(LON_HEMI)));

	    static const boost::units::metric::knot_base_unit::unit_type knots;
	    status_.set_speed_with_units(nmea.as<double>(SOG_KNOTS)*knots);
	    // I think OceanServer is using Track for True Heading - TODO: check this
	    status_.mutable_pose()->set_heading_with_units(nmea.as<double>(TRACK_DEGREES)*boost::units::degree::degrees);
	}
	else if(nmea.at(0).substr(0,2) == "$C")
	{
	    // $C82.8P-3.89R-2.63T20.3D3.2*78
	    enum CompassFields {
		TALKER = 0,
		HEADING = 1,
		PITCH = 2,
		ROLL = 3,
		TEMP = 4,
		DEPTH = 5 };

	    std::vector<std::string> cfields;
	    boost::split(cfields, nmea.at(0), boost::is_any_of("CPRTD"));

	    static const boost::units::imperial::foot_base_unit::unit_type feet;
	    status_.mutable_global_fix()->set_depth_with_units(goby::util::as<double>(cfields.at(DEPTH))*feet);
	    status_.mutable_pose()->set_roll_with_units(goby::util::as<double>(cfields.at(ROLL))*boost::units::degree::degrees);
	    status_.mutable_pose()->set_pitch_with_units(goby::util::as<double>(cfields.at(PITCH))*boost::units::degree::degrees);
    
	    compute_missing(&status_);	   
	    gpb::FrontSeatInterfaceData data;
	    data.mutable_node_status()->CopyFrom(status_);
	    signal_data_from_frontseat(data);
	}
	else
	{
	    glog.is(DEBUG1) && glog << "[Parser]: Ignoring sentence: " << s << std::flush;
	}
        
    }
    catch(goby::util::bad_nmea_sentence& e)
    {
	glog.is(WARN) && glog << "[Parser]: Invalid NMEA sentence: " << e.what() << std::endl;
    }
        
    
// TODO add in backseat control mode
    frontseat_state_ = gpb::FRONTSEAT_IN_CONTROL;
}

void IverFrontSeat::send_command_to_frontseat(const gpb::CommandRequest& command)
{    
    // not yet implemented
} 
    
void IverFrontSeat::send_data_to_frontseat(const gpb::FrontSeatInterfaceData& data)
{
    // no data yet to send
}

void IverFrontSeat::send_raw_to_frontseat(const gpb::FrontSeatRaw& data)
{
    write(data.raw());
} 
    
bool IverFrontSeat::frontseat_providing_data() const
{
    return frontseat_providing_data_;
} 

goby::moos::protobuf::FrontSeatState IverFrontSeat::frontseat_state() const
{
    return frontseat_state_;
} 

void IverFrontSeat::write(const std::string& s)
{
    gpb::FrontSeatRaw raw_msg;
    raw_msg.set_raw(s);
    signal_raw_to_frontseat(raw_msg);
    
    serial_.write(s + "\r\n");
}

boost::units::quantity<boost::units::si::time> IverFrontSeat::nmea_time_to_seconds(double nmea_time, int nmea_date)
{
    namespace si = boost::units::si;
    
    using namespace boost::posix_time;
    using namespace boost::gregorian;
    ptime unix_epoch(date(1970,1,1),time_duration(0,0,0));
    
    int hours = nmea_time / 10000;
    nmea_time -= hours*10000;
    int minutes = nmea_time / 100;
    nmea_time -= minutes*100;
    int seconds = nmea_time;
    long micro_s = (nmea_time - seconds)*1e6;
    

    int day = 0, month = 0, year = 0;
    // for time warp
    if(nmea_date > 999999)
    {
	day = nmea_date / 100000;
	nmea_date -= day*100000;
	month = nmea_date / 1000;
	nmea_date -= month*1000;
	year = nmea_date;
    }
    else
    {
	day = nmea_date / 10000;
	nmea_date -= day*10000;
	month = nmea_date / 100;
	nmea_date -= month*100;
	year = nmea_date;
    }
    
    try
    {
        ptime given_time(date(year + 2000, month, day),
                         time_duration(hours, minutes, seconds) + microseconds(micro_s));
        
        if (given_time == not_a_date_time)
        {
            return -1*si::seconds;
        }
        else
        {
            date_duration date_diff = given_time.date() - date(1970,1,1);
            time_duration time_diff = given_time.time_of_day();
        
            return (date_diff.days()*24*3600 + 
		    time_diff.total_seconds() +
		    static_cast<double>(time_diff.fractional_seconds()) / time_duration::ticks_per_second())*si::seconds;
        }
    }
    catch(std::exception& e)
    {
	glog.is(DEBUG1) && glog << "Invalid time: " << e.what() << std::endl;
        return -1*si::seconds;
    }
}


boost::units::quantity<boost::units::degree::plane_angle> IverFrontSeat::nmea_geo_to_degrees(double nmea_geo, char hemi)
{
    // DDMM.MMMM
    double deg_int = std::floor(nmea_geo / 1e2);
    double deg_frac = (nmea_geo - (deg_int * 1e2)) / 60;

    double deg = std::numeric_limits<double>::quiet_NaN();
    
    if(hemi == 'N' || hemi == 'E')
        deg = (deg_int + deg_frac);
    else if(hemi == 'S' || hemi == 'W')
        deg = -(deg_int + deg_frac);

    return deg*boost::units::degree::degrees;
}
