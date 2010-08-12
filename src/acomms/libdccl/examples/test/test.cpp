// t. schneider tes@mit.edu 11.20.09
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This software is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this software.  If not, see <http://www.gnu.org/licenses/>.


#include "goby/acomms/dccl.h"
#include <iostream>
#include <cassert>

using namespace goby;
using goby::acomms::operator<<;

void plus1(acomms::DCCLMessageVal& mv)
{
    long l = mv;
    ++l;
    mv = l;
}

void times2(acomms::DCCLMessageVal& mv)
{
    double d = mv;
    d *= 2;
    mv = d;
}

void prepend_fat(acomms::DCCLMessageVal& mv)
{
    std::string s = mv;
    s = "fat_" + s;
    mv = s;
}

void invert(acomms::DCCLMessageVal& mv)
{
    bool b = mv;
    b ^= 1;
    mv = b;
}

void algsum(acomms::DCCLMessageVal& mv, const std::vector<acomms::DCCLMessageVal>& ref_vals)
{
    double d = 0;
    // index 0 is the name ("sum"), so start at 1
    for(size_t i = 0, n = ref_vals.size(); i < n; ++i)
    {
        d += double(ref_vals[i]);
    }
    mv = d;
}


int main()
{
    std::cout << "loading xml file: test.xml" << std::endl;

    // instantiate the parser with a single xml file
    acomms::DCCLCodec dccl(DCCL_EXAMPLES_DIR "/test/test.xml", "../../message_schema.xsd");

    std::cout << dccl << std::endl;
    
    // load up the algorithms    
    dccl.add_algorithm("prepend_fat", &prepend_fat);
    dccl.add_algorithm("+1", &plus1);
    dccl.add_algorithm("*2", &times2);
    dccl.add_algorithm("invert", &invert);
    dccl.add_adv_algorithm("sum", &algsum);

    // must be kept secret!
    dccl.set_crypto_passphrase("my_passphrase!");
    
    std::map<std::string, std::vector<acomms::DCCLMessageVal> > in;
    
    bool b = true; 
    std::vector<acomms::DCCLMessageVal> e;
    e.push_back("dog");
    e.push_back("cat");
    e.push_back("emu");
    
    std::string s = "raccoon";  
    std::vector<acomms::DCCLMessageVal> i;
    i.push_back(30);
    i.push_back(40);
    std::vector<acomms::DCCLMessageVal> f;
    f.push_back(-12.5);
    f.push_back(1);
    
    std::string h = "abcd1234"; 
    std::vector<acomms::DCCLMessageVal> sum(2,0);
    
    
    in["B"] = std::vector<acomms::DCCLMessageVal>(1,b);
    in["E"] = e;
    in["S"] = std::vector<acomms::DCCLMessageVal>(1,s);
    in["I"] = i;
    in["F"] = f;
    in["H"] = std::vector<acomms::DCCLMessageVal>(1,h);
    in["SUM"] = sum;

    std::string hex;
    std::cout << "sent values:" << std::endl 
              << in;

    dccl.encode(4, hex, in);

    std::cout << "hex out: " << hex << std::endl;
    hex.resize(hex.length() + 20,'0');
    std::cout << "hex in: " << hex << std::endl;    
    
    std::map<std::string, std::vector<acomms::DCCLMessageVal> > out;
    
    dccl.decode(hex, out);
    
    std::cout << "received values:" << std::endl 
              << out;    

    sum[0] = double(i[0]) + double(f[0]);
    sum[1] = double(i[1]) + double(f[1]);
    i[0] = int(i[0]) + 1;
    i[1] = int(i[1]) + 1;
    
    acomms::DCCLMessageVal tmp = b;
    invert(tmp);
    b = tmp;
    
    tmp = s;
    prepend_fat(tmp);
    tmp.get(s);

    tmp = f[0];
    times2(tmp);
    f[0] = tmp;

    tmp = f[1];
    times2(tmp);
    f[1] = tmp;

    assert(out["B"][0] == b);
    assert(out["E"][0] == e[0]);
    assert(out["E"][1] == e[1]);
    assert(out["E"][2] == e[2]);
    assert(out["S"][0] == s);
    assert(out["F"][0] == f[0]);
    assert(out["F"][1] == f[1]);
    assert(out["SUM"][0] == sum[0]);
    assert(out["SUM"][1] == sum[1]);
    assert(out["I"][0] == i[0]);
    assert(out["I"][1] == i[1]);
    //assert(out["H"][0] == h);
    
    std::cout << "all tests passed" << std::endl;
}
