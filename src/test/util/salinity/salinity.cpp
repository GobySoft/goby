#include "goby/util/seawater/salinity.h"
#include <cassert>
#include <iostream>
#include <iomanip>
#include "goby/util/sci.h"

int main()
{
    using goby::util::unbiased_round;

    // from UNESCO 1983 test cases
    double test_conductivity_ratio = 1.888091;
    const double CONDUCTIVITY_AT_STANDARD = 42.914; // S = 35, T = 15 deg C, P = 0 dbar
    double test_conductivity_mSiemens_cm = test_conductivity_ratio * CONDUCTIVITY_AT_STANDARD;

    double test_temperature_deg_C = 40;
    double test_pressure_dbar = 10000;
    
    double calculated_salinity = SalinityCalculator::salinity(test_conductivity_mSiemens_cm, test_temperature_deg_C, test_pressure_dbar);

    std::cout << "calculated salinity: "
              << std::fixed <<  std::setprecision(5) << calculated_salinity
              << " for T = " << test_temperature_deg_C << " deg C,"
              << " P = " << test_pressure_dbar << " dbar," 
              << " C = " << test_conductivity_mSiemens_cm << " mSiemens / cm" << std::endl;
    
    assert(goby::util::unbiased_round(calculated_salinity, 5) == 40.00000);    
}
