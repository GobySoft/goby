
#ifndef DCCLException20100812H
#define DCCLException20100812H

#include "goby/common/exception.h"

namespace goby
{
    namespace acomms
    {
        /// \brief Exception class for libdccl
        class DCCLException : public goby::Exception
        {
          public:
          DCCLException(const std::string& s)
              : Exception(s)
            { }

        };

        // used to signal null value in field codecs
        class DCCLNullValueException : public DCCLException
        {
          public:
          DCCLNullValueException()
              : DCCLException("NULL Value")
            { }    
        };
        
    }
}


#endif

