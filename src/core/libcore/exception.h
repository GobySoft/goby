#include "goby/util/exception.h"

namespace goby
{
    
    namespace core
    {
        
        ///  \brief  indicates a problem with the runtime command line
        /// or .cfg file configuration (or --help was given)
        class ConfigException : public Exception
        {
          public:
          ConfigException(const std::string& s)
              : Exception(s),
                error_(true)
                { }

            void set_error(bool b) { error_ = b; }
            bool error() { return error_; }
        
          private:
            bool error_;
        };
    }
}
