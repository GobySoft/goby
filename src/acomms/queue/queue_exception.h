
#ifndef QueueException20100812H
#define QueueException20100812H
// simple exception class

#include "goby/common/exception.h"

namespace goby
{
    namespace acomms
    {
        /// \brief Exception class for libdccl
        class QueueException : public goby::Exception
        {
          public:
          QueueException(const std::string& s)
              : Exception(s)
            { }
        };
    }
}

#endif
