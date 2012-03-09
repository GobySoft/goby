
// name of Class, initial date (when file was first created) in YYYYMMDD, followed by capital 'H'
#ifndef Class20080605H
#define Class20080605H

// std
#include <string>

// third party
#include <boost/algorithm/string.hpp>

// local to goby
#include "acomms/modem_message.h"

// local to folder (part of same application / library)
#include "support_class.h"

// typedef
typedef std::list<micromodem::Message>::iterator messages_it;

// incomplete declarations: use whenever possible!
class FlexCout;

namespace nspace
{
    class Class
    {
      public:
        // "FlexCout* os", not "FlexCout *os:
        // in C++ we emphasize objects
        // (read: "os" is a FlexCout pointer)
        Class(FlexCout* os = NULL,
              unsigned int modem_id = 0,
              bool is_ccl = false);
        
        // definitions
        bool push_message(micromodem::Message& new_message);
            
        // inlines
        void clear_ack_queue() { waiting_for_ack_.clear(); }
        
        // only if class is intended to be derived
      protected: //methods    
        
      private: // methods
        messages_it next_message_it();    
        
      private: // data, no public data
        std::string name_;
        FlexCout* os_;
    };

    // related non class functions
    std::ostream & operator<< (std::ostream& os, const Class& oq);
}

#endif
