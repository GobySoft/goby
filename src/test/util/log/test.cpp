#include "goby/common/logger.h"
#include <cassert>

#include <sstream>

using goby::glog;

/// asserts false if called - used for testing proper short-circuiting of logger calls
inline std::ostream& stream_assert(std::ostream & os)
{ assert(false); }

using namespace goby::util::logger;

int main()
{
    glog.set_name("test");

    std::cout << "attaching std::cout to QUIET" << std::endl;
    glog.add_stream(QUIET, &std::cout);
    glog.is(DEBUG3) && glog << stream_assert << std::endl;
    glog.is(DEBUG2) && glog << stream_assert << std::endl;
    glog.is(DEBUG1) && glog << stream_assert << std::endl;
    glog.is(VERBOSE) && glog << stream_assert << std::endl;
    glog.is(WARN) && glog << stream_assert << std::endl;

    std::cout << "attaching std::cout to WARN" << std::endl;
    glog.add_stream(WARN, &std::cout);
    glog.is(DEBUG3) && glog << stream_assert << std::endl;
    glog.is(DEBUG2) && glog << stream_assert << std::endl;
    glog.is(DEBUG1) && glog << stream_assert << std::endl;
    glog.is(VERBOSE) && glog << stream_assert << std::endl;
    glog.is(WARN) && glog << "warn ok" << std::endl;

    std::cout << "attaching std::cout to VERBOSE" << std::endl;
    glog.add_stream(VERBOSE, &std::cout);
    glog.is(DEBUG3) && glog << stream_assert << std::endl;
    glog.is(DEBUG2) && glog << stream_assert << std::endl;
    glog.is(DEBUG1) && glog << stream_assert << std::endl;
    glog.is(VERBOSE) && glog << "verbose ok" << std::endl;
    glog.is(WARN) && glog << "warn ok" << std::endl;

    
    std::cout << "checking locking ... " << std::endl;
    glog.is(VERBOSE, goby::util::logger_lock::lock) && glog << "lock ok" << std::endl << unlock;
    glog.is(VERBOSE, goby::util::logger_lock::lock) && glog << "unlock ok" << std::endl << unlock;
    glog.is(DEBUG3, goby::util::logger_lock::lock) && glog << stream_assert << std::endl << unlock;

    
    std::cout << "attaching std::cout to DEBUG1" << std::endl;
    glog.add_stream(DEBUG1, &std::cout);
    glog.is(DEBUG3) && glog << stream_assert << std::endl;
    glog.is(DEBUG2) && glog << stream_assert << std::endl;
    glog.is(DEBUG1) && glog << "debug1 ok" << std::endl;
    glog.is(VERBOSE) && glog << "verbose ok" << std::endl;
    glog.is(WARN) && glog << "warn ok" << std::endl;

    std::cout << "attaching std::cout to DEBUG2" << std::endl;
    glog.add_stream(DEBUG2, &std::cout);
    glog.is(DEBUG3) && glog << stream_assert << std::endl;
    glog.is(DEBUG2) && glog << "debug2 ok" << std::endl;
    glog.is(DEBUG1) && glog << "debug1 ok" << std::endl;
    glog.is(VERBOSE) && glog << "verbose ok" << std::endl;
    glog.is(WARN) && glog << "warn ok" << std::endl;

    std::cout << "attaching std::cout to DEBUG3" << std::endl;
    glog.add_stream(DEBUG3, &std::cout);
    glog.is(DEBUG3) && glog << "debug3 ok" << std::endl;
    glog.is(DEBUG2) && glog << "debug2 ok" << std::endl;
    glog.is(DEBUG1) && glog << "debug1 ok" << std::endl;
    glog.is(VERBOSE) && glog << "verbose ok" << std::endl;
    glog.is(WARN) && glog << "warn ok" << std::endl;

    std::stringstream ss1;
    std::cout << "attaching stringstream to VERBOSE" << std::endl;
    glog.add_stream(VERBOSE, &ss1);
    glog.is(DEBUG3) && glog << "debug3 ok" << std::endl;
    glog.is(DEBUG2) && glog << "debug2 ok" << std::endl;
    glog.is(DEBUG1) && glog << "debug1 ok" << std::endl;
    glog.is(VERBOSE) && glog << "verbose ok" << std::endl;
    glog.is(WARN) && glog << "warn ok" << std::endl;
    
    std::cout << "ss1: \n" << ss1.rdbuf();


    glog.add_group("test1", goby::util::Colors::lt_green, "Test 1");
    glog.add_group("test2", goby::util::Colors::lt_green, "Test 2");

    glog << group("test1") << "test1 group ok" << std::endl;
    glog.is(WARN) && glog << group("test2") << "test2 group warning ok" << std::endl;
    
    std::cout << "All tests passed." << std::endl;
    return 0;
}


