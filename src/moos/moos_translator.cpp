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


#include <boost/algorithm/string.hpp>
#include <boost/math/special_functions/fpclassify.hpp>


#include "goby/util/sci.h"

#include "moos_translator.h"

const double NaN = std::numeric_limits<double>::quiet_NaN();

void goby::moos::MOOSTranslator::initialize(double lat_origin, double lon_origin)
{
    transitional::DCCLAlgorithmPerformer* ap = transitional::DCCLAlgorithmPerformer::getInstance();
                
    // set up algorithms
    ap->add_algorithm("power_to_dB", &alg_power_to_dB);
    ap->add_algorithm("dB_to_power", &alg_dB_to_power);
    ap->add_adv_algorithm("TSD_to_soundspeed", &alg_TSD_to_soundspeed);
    ap->add_algorithm("to_lower", &alg_to_lower);
    ap->add_algorithm("to_upper", &alg_to_upper);
    ap->add_algorithm("angle_0_360", &alg_angle_0_360);
    ap->add_algorithm("angle_-180_180", &alg_angle_n180_180);

    if(!(boost::math::isnan)(lat_origin) && !(boost::math::isnan)(lon_origin))
    {
        if(geodesy_.Initialise(lat_origin, lon_origin))
        {
            ap->add_adv_algorithm("lat2utm_y", boost::bind(&MOOSTranslator::alg_lat2utm_y, this, _1, _2));
            ap->add_adv_algorithm("lon2utm_x", boost::bind(&MOOSTranslator::alg_lon2utm_x, this, _1, _2));
            ap->add_adv_algorithm("utm_x2lon", boost::bind(&MOOSTranslator::alg_utm_x2lon, this, _1, _2));
            ap->add_adv_algorithm("utm_y2lat", boost::bind(&MOOSTranslator::alg_utm_y2lat, this, _1, _2));
        }
    }
                
}


void goby::moos::alg_power_to_dB(transitional::DCCLMessageVal& val_to_mod)
{
    val_to_mod = 10*log10(double(val_to_mod));
}

void goby::moos::alg_dB_to_power(transitional::DCCLMessageVal& val_to_mod)
{
    val_to_mod = pow(10.0, double(val_to_mod)/10.0);
}

// applied to "T" (temperature), references are "S" (salinity), then "D" (depth)
void goby::moos::alg_TSD_to_soundspeed(transitional::DCCLMessageVal& val,
                                       const std::vector<transitional::DCCLMessageVal>& ref_vals)
{
    val.set(goby::util::mackenzie_soundspeed(val, ref_vals[0], ref_vals[1]), 3);
}


void goby::moos::alg_angle_0_360(transitional::DCCLMessageVal& angle)
{
    double a = angle;
    while (a < 0)
        a += 360;
    while (a >=360)
        a -= 360;
    angle = a;
}

void goby::moos::alg_angle_n180_180(transitional::DCCLMessageVal& angle)
{
    double a = angle;
    while (a < -180)
        a += 360;
    while (a >=180)
        a -= 360;
    angle = a;
}

void goby::moos::MOOSTranslator::alg_lat2utm_y(transitional::DCCLMessageVal& mv,
                               const std::vector<transitional::DCCLMessageVal>& ref_vals)
{
    double lat = mv;
    double lon = ref_vals[0];
    double x = NaN;
    double y = NaN;
        
    if(!(boost::math::isnan)(lat) && !(boost::math::isnan)(lon)) geodesy_.LatLong2LocalUTM(lat, lon, y, x);        
    mv = y;
}

void goby::moos::MOOSTranslator::alg_lon2utm_x(transitional::DCCLMessageVal& mv,
                               const std::vector<transitional::DCCLMessageVal>& ref_vals)
{
    double lon = mv;
    double lat = ref_vals[0];
    double x = NaN;
    double y = NaN;

    if(!(boost::math::isnan)(lat) && !(boost::math::isnan)(lon)) geodesy_.LatLong2LocalUTM(lat, lon, y, x);
    mv = x;
}


void goby::moos::MOOSTranslator::alg_utm_x2lon(transitional::DCCLMessageVal& mv,
                               const std::vector<transitional::DCCLMessageVal>& ref_vals)
{
    double x = mv;
    double y = ref_vals[0];

    double lat = NaN;
    double lon = NaN;
    if(!(boost::math::isnan)(y) && !(boost::math::isnan)(x)) geodesy_.UTM2LatLong(x, y, lat, lon);    
    mv = lon;
}

void goby::moos::MOOSTranslator::alg_utm_y2lat(transitional::DCCLMessageVal& mv,
                                               const std::vector<transitional::DCCLMessageVal>& ref_vals)
{
    double y = mv;
    double x = ref_vals[0];
    
    double lat = NaN;
    double lon = NaN;
    if(!(boost::math::isnan)(x) && !(boost::math::isnan)(y)) geodesy_.UTM2LatLong(x, y, lat, lon);
    
    mv = lat;
}

void goby::moos::alg_to_upper(transitional::DCCLMessageVal& val_to_mod)
{
    val_to_mod = boost::algorithm::to_upper_copy(std::string(val_to_mod));
}

void goby::moos::alg_to_lower(transitional::DCCLMessageVal& val_to_mod)
{
    val_to_mod = boost::algorithm::to_lower_copy(std::string(val_to_mod));
}
