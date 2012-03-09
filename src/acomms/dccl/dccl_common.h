
#ifndef DCCLConstants20091211H
#define DCCLConstants20091211H

#include <string>
#include <bitset>
#include <limits>
#include <vector>

#include <google/protobuf/dynamic_message.h>
#include <boost/dynamic_bitset.hpp>

#include "goby/util/primitive_types.h"
#include "goby/acomms/acomms_constants.h"
#include "goby/common/logger.h"

namespace goby
{

    namespace acomms
    {
        /// \brief Used for all cases in DCCL when an arbitrary length set of raw bits is needed. boost::dynamic_bitset is similar to std::bitset but allows dynamic length (runtime set) bitsets.
        typedef boost::dynamic_bitset<unsigned char> Bitset;


        inline Bitset operator+(const Bitset& a, const Bitset& b)
        {
            Bitset out(a);
            for(int i = 0, n = b.size(); i < n; ++i)
                out.push_back(b[i]);
            return out;
        }

        
        inline void bitset2string(const Bitset& bits, std::string* str)
        {
            str->resize(bits.num_blocks()); // resize the string to fit the bitset
            to_block_range(bits, str->rbegin());
        }
            
        inline void string2bitset(Bitset* bits, const std::string& str)
        {
            bits->resize(str.size() * BITS_IN_BYTE);
            from_block_range(str.rbegin(), str.rend(), *bits);
        }
        
        // more efficient way to do ceil(total_bits / 8)
        // to get the number of bytes rounded up.
        enum { BYTE_MASK = 7 }; // 00000111
        inline unsigned floor_bits2bytes(unsigned bits)
        { return bits >> 3; }
        inline unsigned ceil_bits2bytes(unsigned bits)
        {
            return (bits& BYTE_MASK) ?
                floor_bits2bytes(bits) + 1 :
                floor_bits2bytes(bits);
        }
        
        
    }
}
#endif
