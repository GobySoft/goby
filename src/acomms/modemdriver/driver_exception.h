
#ifndef DriverException20108012H
#define DriverException20100812H

#include "goby/common/exception.h"

namespace goby
{
    namespace acomms
    {
        class ModemDriverException : public goby::Exception
        {
          public:
          ModemDriverException(const std::string& s)
              : Exception(s)
            { }

        };
    }
}

#endif
