#include "stdint.h"
#include "goby/util/time.h"

using goby::util::as;
using goby::uint64;
using goby::util::goby_time;
using namespace boost::posix_time;
using namespace boost::gregorian;

int main()
{
    // 2011-08-16 19:36:57.523456 UTC
    const double TEST_DOUBLE_TIME = 1313523417.523456;
    const uint64 TEST_MICROSEC_TIME = TEST_DOUBLE_TIME * 1e6;
    const boost::posix_time::ptime TEST_PTIME(date(2011,8,16),
                                              time_duration(19,36,57) +
                                              microseconds(523456));

    std::cout << "double: " << std::setprecision(16) << TEST_DOUBLE_TIME << std::endl;
    std::cout << "uint64: " << TEST_MICROSEC_TIME << std::endl;
    std::cout << "ptime: " << TEST_PTIME << std::endl;

    assert(goby::util::ptime2unix_double(TEST_PTIME) == TEST_DOUBLE_TIME);
    assert(as<double>(TEST_PTIME) == TEST_DOUBLE_TIME); // same as previous line

    assert(goby::util::unix_double2ptime(TEST_DOUBLE_TIME) == TEST_PTIME);
    assert(as<ptime>(TEST_DOUBLE_TIME) == TEST_PTIME);  // same as previous line
    
    assert(goby::util::ptime2unix_microsec(TEST_PTIME) == TEST_MICROSEC_TIME);
    assert(as<uint64>(TEST_PTIME) == TEST_MICROSEC_TIME);  // same as previous line

    assert(goby::util::unix_microsec2ptime(TEST_MICROSEC_TIME) == TEST_PTIME);
    assert(as<ptime>(TEST_MICROSEC_TIME) == TEST_PTIME); // same as previous line

    
    assert(as<ptime>(std::numeric_limits<uint64>::max()) == ptime(not_a_date_time));

    std::cout << goby_time() << std::endl;
    std::cout << goby_time<double>() << std::endl;
    std::cout << goby_time<uint64>() << std::endl;

    std::cout << "all tests passed" << std::endl;
    
    return 0;
}
