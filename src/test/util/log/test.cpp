#include "goby/util/logger.h"
#include <cassert>

#include <sstream>

using goby::glog;

/// asserts false if called - used for testing proper short-circuiting of logger calls
inline std::ostream& stream_assert(std::ostream & os)
{ assert(false); }



int main()
{
    glog.set_name("test");

    std::cout << "attaching std::cout to QUIET" << std::endl;
    glog.add_stream(goby::util::Logger::QUIET, &std::cout);
    glog.is(debug3) && glog << stream_assert << std::endl;
    glog.is(debug2) && glog << stream_assert << std::endl;
    glog.is(debug1) && glog << stream_assert << std::endl;
    glog.is(verbose) && glog << stream_assert << std::endl;
    glog.is(warn) && glog << stream_assert << std::endl;

    std::cout << "attaching std::cout to WARN" << std::endl;
    glog.add_stream(goby::util::Logger::WARN, &std::cout);
    glog.is(debug3) && glog << stream_assert << std::endl;
    glog.is(debug2) && glog << stream_assert << std::endl;
    glog.is(debug1) && glog << stream_assert << std::endl;
    glog.is(verbose) && glog << stream_assert << std::endl;
    glog.is(warn) && glog << "warn ok" << std::endl;

    std::cout << "attaching std::cout to VERBOSE" << std::endl;
    glog.add_stream(goby::util::Logger::VERBOSE, &std::cout);
    glog.is(debug3) && glog << stream_assert << std::endl;
    glog.is(debug2) && glog << stream_assert << std::endl;
    glog.is(debug1) && glog << stream_assert << std::endl;
    glog.is(verbose) && glog << "verbose ok" << std::endl;
    glog.is(warn) && glog << "warn ok" << std::endl;

    
    std::cout << "checking locking ... " << std::endl;
    glog.is(verbose, goby::util::logger_lock::lock) && glog << "lock ok" << std::endl << unlock;
    glog.is(verbose, goby::util::logger_lock::lock) && glog << "unlock ok" << std::endl << unlock;
    glog.is(debug3, goby::util::logger_lock::lock) && glog << stream_assert << std::endl << unlock;

    
    std::cout << "attaching std::cout to DEBUG1" << std::endl;
    glog.add_stream(goby::util::Logger::DEBUG1, &std::cout);
    glog.is(debug3) && glog << stream_assert << std::endl;
    glog.is(debug2) && glog << stream_assert << std::endl;
    glog.is(debug1) && glog << "debug1 ok" << std::endl;
    glog.is(verbose) && glog << "verbose ok" << std::endl;
    glog.is(warn) && glog << "warn ok" << std::endl;

    std::cout << "attaching std::cout to DEBUG2" << std::endl;
    glog.add_stream(goby::util::Logger::DEBUG2, &std::cout);
    glog.is(debug3) && glog << stream_assert << std::endl;
    glog.is(debug2) && glog << "debug2 ok" << std::endl;
    glog.is(debug1) && glog << "debug1 ok" << std::endl;
    glog.is(verbose) && glog << "verbose ok" << std::endl;
    glog.is(warn) && glog << "warn ok" << std::endl;

    std::cout << "attaching std::cout to DEBUG3" << std::endl;
    glog.add_stream(goby::util::Logger::DEBUG3, &std::cout);
    glog.is(debug3) && glog << "debug3 ok" << std::endl;
    glog.is(debug2) && glog << "debug2 ok" << std::endl;
    glog.is(debug1) && glog << "debug1 ok" << std::endl;
    glog.is(verbose) && glog << "verbose ok" << std::endl;
    glog.is(warn) && glog << "warn ok" << std::endl;

    std::stringstream ss1;
    std::cout << "attaching stringstream to VERBOSE" << std::endl;
    glog.add_stream(goby::util::Logger::VERBOSE, &ss1);
    glog.is(debug3) && glog << "debug3 ok" << std::endl;
    glog.is(debug2) && glog << "debug2 ok" << std::endl;
    glog.is(debug1) && glog << "debug1 ok" << std::endl;
    glog.is(verbose) && glog << "verbose ok" << std::endl;
    glog.is(warn) && glog << "warn ok" << std::endl;
    
    std::cout << "ss1: \n" << ss1.rdbuf();


    glog.add_group("test1", goby::util::Colors::lt_green, "Test 1");
    glog.add_group("test2", goby::util::Colors::lt_green, "Test 2");

    glog << group("test1") << "test1 group ok" << std::endl;
    glog.is(warn) && glog << group("test2") << "test2 group warning ok" << std::endl;
    
    std::cout << "All tests passed." << std::endl;
    return 0;
}


