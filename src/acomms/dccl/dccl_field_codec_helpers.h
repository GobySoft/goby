
#ifndef DCCLFIELDCODECHELPERS20110825H
#define DCCLFIELDCODECHELPERS20110825H

#include "dccl_common.h"

#include <boost/signals.hpp>

namespace goby
{
    namespace acomms
    {        

        //RAII handler for the current Message recursion stack
        class MessageHandler
        {
          public:
            MessageHandler(const google::protobuf::FieldDescriptor* field = 0);
                
            ~MessageHandler()
            {
                for(int i = 0; i < descriptors_pushed_; ++i)
                    __pop_desc();
                    
                for(int i = 0; i < fields_pushed_; ++i)
                    __pop_field();
            }
            bool first() 
            { return desc_.empty(); }
            int count() 
            { return desc_.size(); }

            void push(const google::protobuf::Descriptor* desc);
            void push(const google::protobuf::FieldDescriptor* field);

            friend class DCCLFieldCodecBase;
          private:
            void __pop_desc();
            void __pop_field();
                
            static std::vector<const google::protobuf::Descriptor*> desc_;
            static std::vector<const google::protobuf::FieldDescriptor*> field_;
            int descriptors_pushed_;
            int fields_pushed_;
        };

        class BitsHandler
        {
          public:
            BitsHandler(Bitset* out_pool, Bitset* in_pool, bool lsb_first = true);
            
            ~BitsHandler()
            {
                connection_.disconnect();
            }
            void transfer_bits(unsigned size);

          private:
            bool lsb_first_;
            Bitset* in_pool_;
            Bitset* out_pool_;
            boost::signals::connection connection_;
        };
    }
}


#endif
