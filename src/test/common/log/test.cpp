// Copyright 2009-2013 Toby Schneider (https://launchpad.net/~tes)
//                     Massachusetts Institute of Technology (2007-)
//                     Woods Hole Oceanographic Institution (2007-)
//                     Goby Developers Team (https://launchpad.net/~goby-dev)
// 
//
// This file is part of the Goby Underwater Autonomy Project Binaries
// ("The Goby Binaries").
//
// The Goby Binaries are free software: you can redistribute them and/or modify
// them under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// The Goby Binaries are distributed in the hope that they will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Goby.  If not, see <http://www.gnu.org/licenses/>.
#include "goby/common/logger.h"
#include <cassert>

#include <sstream>

using goby::glog;

/// asserts false if called - used for testing proper short-circuiting of logger calls
inline std::ostream& stream_assert(std::ostream & os)
{ assert(false); }

using namespace goby::common::logger;

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

    
#if THREAD_SAFE_LOGGER
    std::cout << "checking locking ... " << std::endl;
    glog.is(VERBOSE, goby::common::logger_lock::lock) && glog << "lock ok" << std::endl << unlock;
    glog.is(VERBOSE, goby::common::logger_lock::lock) && glog << "unlock ok" << std::endl << unlock;
    glog.is(DEBUG3, goby::common::logger_lock::lock) && glog << stream_assert << std::endl << unlock;
#endif
    
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


    glog.add_group("test1", goby::common::Colors::lt_green, "Test 1");
    glog.add_group("test2", goby::common::Colors::lt_green, "Test 2");

    glog << group("test1") << "test1 group ok" << std::endl;
    glog.is(WARN) && glog << group("test2") << "test2 group warning ok" << std::endl;
    
    std::cout << "All tests passed." << std::endl;
    return 0;
}


