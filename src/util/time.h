// copyright 2010 t. schneider tes@mit.edu
// 
// this file is a collection of time conversion utilities used in goby
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

#ifndef Time20100713H
#define Time20100713H

#include <ctime>

#include <boost/date_time.hpp>

/// All objects related to the Goby Underwater Autonomy Project
namespace goby
{
    /// Utility objects for performing functions such as logging, non-acoustic communication (ethernet / serial), time, scientific, string manipulation, etc.
    namespace util
    {
        ///\name Time
        //@{

        /// Always use for current time within the Goby project
        inline boost::posix_time::ptime goby_time()
        {
            using namespace boost::posix_time;
            return ptime(microsec_clock::universal_time());
        }

        /// Simple string representation of goby_time()
        inline std::string goby_time_as_string(const boost::posix_time::ptime& t = goby_time())
        { return boost::posix_time::to_simple_string(t); }

        /// ISO string representation of goby_time()
        inline std::string goby_file_timestamp()
        {
            using namespace boost::posix_time;
            return to_iso_string(second_clock::universal_time());
        }
        
        
        /// convert from boost date_time ptime to the number of seconds (including fractional) since 1/1/1970 0:00 UTC ("UNIX Time")
        inline double ptime2unix_double(boost::posix_time::ptime given_time)
        {
            using namespace boost::posix_time;
            using namespace boost::gregorian;
        
            if (given_time == not_a_date_time)
                return -1;
            else
            {
                ptime time_t_epoch(date(1970,1,1));        
                time_duration diff = given_time - time_t_epoch;
        
                return (double(diff.total_seconds()) + double(diff.fractional_seconds()) / double(time_duration::ticks_per_second()));
            }
        }
    
        /// convert to boost date_time ptime from the number of seconds (including fractional) since 1/1/1970 0:00 UTC ("UNIX Time"): good to the microsecond
        inline boost::posix_time::ptime unix_double2ptime(double given_time)
        {
            using namespace boost::posix_time;
            using namespace boost::gregorian;
    
            if (given_time == -1)
                return boost::posix_time::ptime(not_a_date_time);
            else
            {
                ptime time_t_epoch(date(1970,1,1));
            
                double s = floor(given_time);
                double micro_s = (given_time - s)*1e6;
                return time_t_epoch + seconds(s) + microseconds(micro_s);
            }
        }

    
        /// convert to ptime from time_t from time.h (whole seconds since UNIX)
        inline boost::posix_time::ptime time_t2ptime(std::time_t t)
        { return boost::posix_time::from_time_t(t); }
    
        /// convert from ptime to time_t from time.h (whole seconds since UNIX)
        inline std::time_t ptime2time_t(boost::posix_time::ptime t)
        {
            std::tm out = boost::posix_time::to_tm(t);
            return mktime(&out);
        }
        
        /// time duration to double number of seconds: good to the microsecond
        inline double time_duration2double(boost::posix_time::time_duration time_of_day)
        {
            using namespace boost::posix_time;
            return (double(time_of_day.total_seconds()) + double(time_of_day.fractional_seconds()) / double(time_duration::ticks_per_second()));
        }

        //@}

    }
}
#endif
