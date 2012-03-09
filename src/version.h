

#ifndef VERSION20110304H
#define VERSION20110304H

#include <string>

#define GOBY_VERSION_MAJOR @GOBY_VERSION_MAJOR@
#define GOBY_VERSION_MINOR @GOBY_VERSION_MINOR@
#define GOBY_VERSION_PATCH @GOBY_VERSION_PATCH@

namespace goby
{
    const std::string VERSION_STRING = "@GOBY_VERSION@";
    const std::string VERSION_DATE = "@GOBY_VERSION_DATE@";
    const std::string COMPILE_DATE = "@GOBY_COMPILE_DATE@";
}

#endif
