
#ifndef Exception20101005H
#define Exception20101005H

#include <exception>
#include <stdexcept>

namespace goby
{
    
    /// \brief simple exception class for goby applications
    class Exception : public std::runtime_error
    {
      public:
      Exception(const std::string& s)
          : std::runtime_error(s)
        { }
        
    };
    
    
}


#endif

