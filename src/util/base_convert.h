
#include <gmp.h>

#include <string>

namespace goby
{
    namespace util
    {
        inline void base_convert(const std::string& source, std::string* sink, int source_base, int sink_base)
        {
            
            mpz_t base10;
            mpz_init(base10);
            
            for (int i = source.size()-1; i >= 0; --i)
            {
                mpz_add_ui(base10, base10, 0xFF & source[i]);
                if(i)
                    mpz_mul_ui(base10, base10, source_base);
            }
            
            sink->clear();

            while(mpz_cmp_ui(base10, 0) != 0)
            {
                unsigned long int remainder =  mpz_fdiv_q_ui (base10, base10, sink_base);
                sink->push_back(0xFF & remainder);
            }

            mpz_clear(base10);
        }
    }
}
