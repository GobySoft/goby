// Copyright 2009-2013 Toby Schneider (https://launchpad.net/~tes)
//                     Massachusetts Institute of Technology (2007-)
//                     Woods Hole Oceanographic Institution (2007-)
//                     Goby Developers Team (https://launchpad.net/~goby-dev)
// 
//
// This file is part of the Goby Underwater Autonomy Project Binaries
// ("The Goby Binaries").
//
// The Goby Binaries are free software: you can redistribute them and/or modify
// them under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// The Goby Binaries are distributed in the hope that they will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Goby.  If not, see <http://www.gnu.org/licenses/>.

#include <iostream>
#include <cassert>
#include <utility>

#include "goby/util/binary.h"
#include "goby/acomms/dccl/dccl_bitset.h"

using goby::acomms::Bitset;

int main()
{
    // construct
    unsigned long value = 23;
    Bitset bits(8, value);
    std::string s = bits.to_string();
    std::cout << bits << std::endl;
    assert(s == std::string("00010111"));

    // bitshift
    bits <<= 2;
    std::cout << bits << std::endl;
    s = bits.to_string();
    assert(s == std::string("01011100"));
    
    bits >>= 1;
    std::cout << bits << std::endl;
    s = bits.to_string();
    assert(s == std::string("00101110"));

    s = (bits << 3).to_string();
    assert(s == std::string("01110000"));

    s = (bits >> 2).to_string();
    assert(s == std::string("00001011"));

    // logic
    unsigned long v1 = 14, v2 = 679;
    Bitset bits1(15, v1), bits2(15, v2);
    
    assert((bits1 & bits2).to_ulong() == (v1 & v2));
    assert((bits1 | bits2).to_ulong() == (v1 | v2));
    assert((bits1 ^ bits2).to_ulong() == (v1 ^ v2));

    assert((bits < bits1) == (value < v1));
    assert((bits1 < bits2) == (v1 < v2));
    assert((bits2 < bits1) == (v2 < v1));

    using namespace std::rel_ops;
    
    assert((bits1 > bits2) == (v1 > v2));
    assert((bits1 >= bits2) == (v1 >= v2));
    assert((bits1 <= bits2) == (v1 <= v2));
    assert((bits1 != bits2) == (v1 != v2));

    assert((Bitset(8, 23)) == Bitset(8, 23));
    assert((Bitset(16, 23)) != Bitset(8, 23));
    assert((Bitset(16, 0x0001)) != Bitset(16, 0x1001));
    
    assert(goby::util::hex_encode(bits2.to_byte_string()) == "a702");
    bits2.from_byte_string(goby::util::hex_decode("12a502"));
    std::cout << bits2.size() << ": " << bits2 << std::endl;
    assert(bits2.to_ulong() == 0x02a512);

    // get_more_bits;
    {
        std::cout << std::endl;
        Bitset parent(8, 0xD1);
        Bitset child(4, 0, &parent);

        std::cout << "parent: " << parent << std::endl;
        std::cout << "child: " << child << std::endl;

        std::cout << "get more bits: 4" <<  std::endl;
        child.get_more_bits(4);
        
        std::cout << "parent: " << parent << std::endl;
        std::cout << "child: " << child << std::endl;

        assert(child.size() == 8);
        assert(parent.size() == 4);
        
        assert(child.to_ulong() == 0x10);
        assert(parent.to_ulong() == 0xD);
        
    }    

    {
        std::cout << std::endl;
        Bitset grandparent(8, 0xD1);
        Bitset parent(8, 0x02, &grandparent);
        Bitset child(4, 0, &parent);

        std::cout << "grandparent: " << grandparent << std::endl;
        std::cout << "parent: " << parent << std::endl;
        std::cout << "child: " << child << std::endl;

        std::cout << "get more bits: 4" <<  std::endl;
        child.get_more_bits(4);
        
        std::cout << "grandparent: " << grandparent << std::endl;
        std::cout << "parent: " << parent << std::endl;
        std::cout << "child: " << child << std::endl;

        assert(child.size() == 8);
        assert(parent.size() == 4);
        assert(grandparent.size() == 8);
        
        assert(child.to_ulong() == 0x20);
        assert(parent.to_ulong() == 0x0);
        assert(grandparent.to_ulong() == 0xD1);
    }


    {        
        std::cout << std::endl;
        Bitset grandparent(8, 0xD1);
        Bitset parent(&grandparent);
        Bitset child(4, 0, &parent);

        std::cout << "grandparent: " << grandparent << std::endl;
        std::cout << "parent: " << parent << std::endl;
        std::cout << "child: " << child << std::endl;

        std::cout << "get more bits: 4" <<  std::endl;
        child.get_more_bits(4);
        
        std::cout << "grandparent: " << grandparent << std::endl;
        std::cout << "parent: " << parent << std::endl;
        std::cout << "child: " << child << std::endl;

        assert(child.size() == 8);
        assert(parent.size() == 0);
        assert(grandparent.size() == 4);
        
        assert(child.to_ulong() == 0x10);
        assert(parent.to_ulong() == 0x0);
        assert(grandparent.to_ulong() == 0xD);
    }

    
    std::cout << "all tests passed" << std::endl;
    
    return 0;
}
