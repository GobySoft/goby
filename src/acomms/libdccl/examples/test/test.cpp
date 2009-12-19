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


#include "acomms/dccl.h"
#include <iostream>
#include <cassert>

using dccl::operator<<;

void plus1(dccl::MessageVal& mv)
{
    // this is silly but it demonstrates taking in one value and returning another
    long l;
    mv.val(l);
    ++l;

    double d = l;
    mv.set(d);
}

void times2(double& d)
{ d *= 2; }

void prepend_fat(std::string& s)
{ s = "fat_" + s; }

void invert(bool& b)
{ b ^= 1; }

void algsum(dccl::MessageVal& mv, const std::vector<std::string>& params,
         const std::map<std::string,dccl::MessageVal>& vals)
{
    double d;
    // index 0 is the name ("sum"), so start at 1
    for(size_t i = 1, n = params.size(); i < n; ++i)
    {
        std::map<std::string, dccl::MessageVal>::const_iterator it = vals.find(params[i]);
        double v;
        if(it != vals.end() && it->second.val(v))
            d += v;
    }
    mv.set(d);
}


int main()
{
    std::cout << "loading xml file: test.xml" << std::endl;

    // instantiate the parser with a single xml file
    dccl::DCCLCodec dccl(DCCL_EXAMPLES_DIR "/test/test.xml", "../../message_schema.xsd");

    // load up the algorithms    
    dccl.add_str_algorithm("prepend_fat", &prepend_fat);
    dccl.add_generic_algorithm("+1", &plus1);
    dccl.add_dbl_algorithm("*2", &times2);
    dccl.add_bool_algorithm("invert", &invert);
    dccl.add_adv_algorithm("sum", &algsum);
    
    std::vector< std::map<std::string, std::string> > s_in;
    s_in.resize(2);
    std::vector< std::map<std::string, double> > d_in;
    d_in.resize(2);
    std::vector< std::map<std::string, long> > l_in;
    l_in.resize(2);
    std::vector< std::map<std::string, bool> > b_in;
    b_in.resize(2);
    
    // first try everything in as a string
    bool b_i = true;             dccl::MessageVal B(b_i);
    std::string e_i = "dog";     dccl::MessageVal E(e_i);
    std::string s_i = "raccoon"; dccl::MessageVal S(s_i);
    long i_i = 42;               dccl::MessageVal I(i_i);
    double f_i = -12.5;          dccl::MessageVal F(f_i, 1);
    double sum_i = 0;            dccl::MessageVal SUM(sum_i, 0);

    // strings for everything
    B.val(s_in[0]["B"]);
    E.val(s_in[0]["E"]);
    S.val(s_in[0]["S"]);
    I.val(s_in[0]["I"]);
    F.val(s_in[0]["F"]);
    SUM.val(s_in[0]["SUM"]);

    // more sane, cast free approach
    B.val(b_in[1]["B"]);
    E.val(s_in[1]["E"]);
    S.val(s_in[1]["S"]);
    I.val(l_in[1]["I"]);
    F.val(d_in[1]["F"]);
    SUM.val(d_in[1]["SUM"]);    

    for(size_t j= 0, n = s_in.size(); j< n; ++j)
    {
        std::string hex;
        std::cout << "sent values:" << std::endl 
                  << s_in[j]
                  << d_in[j]
                  << l_in[j]
                  << b_in[j];

        dccl.encode(4, hex, &s_in[j], &d_in[j], &l_in[j], &b_in[j]);
    
        for(size_t k=0, m=4; k<m; ++k)
        {
            
            std::map<std::string, std::string> s_out;
            std::map<std::string, double> d_out;
            std::map<std::string, long> l_out;
            std::map<std::string, bool> b_out;
            
            dccl.decode(4, hex,
                        &s_out,
                        (k>0) ? &d_out : 0,
                        (k>1) ? &l_out : 0,
                        (k>2) ? &b_out : 0);
            
            std::cout << "received values:" << std::endl 
                      << s_out
                      << d_out
                      << l_out
                      << b_out;
        
            
            bool b; B.val(b);
            std::string e; E.val(e);
            std::string s; S.val(s);
            long i; I.val(i);
            double f; F.val(f);
            double sum; SUM.val(sum);

            ++i;
            invert(b);
            prepend_fat(s);
            times2(f);
            sum += i_i + f_i;
            
            dccl::MessageVal Bo(b);
            dccl::MessageVal Eo(e);
            dccl::MessageVal So(s);
            dccl::MessageVal Fo(f, 2);
            dccl::MessageVal SUMo(sum, 2);
            dccl::MessageVal Io(i);            

            switch(k)
            {
                case 3:
                    assert(Bo == b_out["B"]);
                    assert(Eo == s_out["E"]);
                    assert(So == s_out["S"]);
                    assert(Fo == d_out["F"]);
                    assert(SUMo == d_out["SUM"]);
                    assert(Io == l_out["I"]);
                    break;
                    
                case 2:
                    assert(Bo == l_out["B"]);
                    assert(Eo == s_out["E"]);
                    assert(So == s_out["S"]);
                    assert(Fo == d_out["F"]);
                    assert(SUMo == d_out["SUM"]);
                    assert(Io == l_out["I"]);
                    break;

                case 1:
                    assert(Bo == d_out["B"]);
                    assert(Eo == s_out["E"]);
                    assert(So == s_out["S"]);
                    assert(Fo == d_out["F"]);
                    assert(SUMo == d_out["SUM"]);
                    assert(Io == d_out["I"]);
                    break;

                case 0:
                    assert(Bo == s_out["B"]);
                    assert(Eo == s_out["E"]);
                    assert(So == s_out["S"]);
                    assert(Fo == s_out["F"]);
                    assert(SUMo == s_out["SUM"]);
                    assert(Io == s_out["I"]);
                    break;
            }
        }
    }
    

    std::cout << "all tests passed" << std::endl;
}
