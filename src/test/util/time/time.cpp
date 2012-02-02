#include "stdint.h"
#include "goby/common/time.h"

using goby::util::as;
using goby::uint64;
using goby::common::goby_time;
using namespace boost::posix_time;
using namespace boost::gregorian;

// 2011-08-16 19:36:57.523456 UTC
const double TEST_DOUBLE_TIME = 1313523417.523456;
const uint64 TEST_MICROSEC_TIME = TEST_DOUBLE_TIME * 1e6;
const boost::posix_time::ptime TEST_PTIME(date(2011,8,16),
                                          time_duration(19,36,57) +
                                          microseconds(523456));

bool double_cmp(double a, double b, int precision)
{
    int a_whole = a;
    int b_whole = b;

    int a_part = (a-a_whole)*pow(10.0, precision);
    int b_part = (b-b_whole)*pow(10.0, precision);
    
    return (a_whole == b_whole) && (a_part == b_part);
}

uint64 fake_time()
{
    return TEST_MICROSEC_TIME;
}


int main()
{
    std::cout << "current double: " << std::setprecision(16) << goby_time<double>() << std::endl;
    std::cout << "current uint64: " << goby_time<uint64>() << std::endl;
    std::cout << "current ptime: " << goby_time() << std::endl;


    std::cout << "double: " << std::setprecision(16) << TEST_DOUBLE_TIME << std::endl;
    std::cout << "uint64: " << TEST_MICROSEC_TIME << std::endl;
    std::cout << "ptime: " << TEST_PTIME << std::endl;

    assert(!double_cmp(5.4, 2.3, 1));
    
    
    assert(double_cmp(goby::common::ptime2unix_double(TEST_PTIME), TEST_DOUBLE_TIME, 6));
    assert(double_cmp(as<double>(TEST_PTIME), TEST_DOUBLE_TIME, 6)); // same as previous line
    
    assert(goby::common::unix_double2ptime(TEST_DOUBLE_TIME) == TEST_PTIME);
    assert(as<ptime>(TEST_DOUBLE_TIME) == TEST_PTIME);  // same as previous line
    
    assert(goby::common::ptime2unix_microsec(TEST_PTIME) == TEST_MICROSEC_TIME);
    assert(as<uint64>(TEST_PTIME) == TEST_MICROSEC_TIME);  // same as previous line

    assert(goby::common::unix_microsec2ptime(TEST_MICROSEC_TIME) == TEST_PTIME);
    assert(as<ptime>(TEST_MICROSEC_TIME) == TEST_PTIME); // same as previous line

    
    assert(as<ptime>(std::numeric_limits<uint64>::max()) == ptime(not_a_date_time));

    std::cout << goby_time() << std::endl;
    assert(goby_time() - boost::posix_time::microsec_clock::universal_time() < boost::posix_time::milliseconds(10));
    std::cout << goby_time<double>() << std::endl;
    assert(as<ptime>(goby_time<double>()) - boost::posix_time::microsec_clock::universal_time() < boost::posix_time::milliseconds(10));
    std::cout << goby_time<uint64>() << std::endl;
    assert(as<ptime>(goby_time<uint64>()) - boost::posix_time::microsec_clock::universal_time() < boost::posix_time::milliseconds(10));

    goby::common::goby_time_function = &fake_time;
    
    std::cout << goby_time() << std::endl;
    std::cout << goby_time<double>() << std::endl;
    std::cout << goby_time<uint64>() << std::endl;
    assert(goby_time<uint64>() == TEST_MICROSEC_TIME); 
    assert(goby_time<double>() == TEST_DOUBLE_TIME); 
    assert(goby_time<ptime>() == TEST_PTIME); 
    

           
    
    std::cout << "all tests passed" << std::endl;
    
    return 0;
}
