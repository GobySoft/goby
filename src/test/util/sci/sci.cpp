#include "goby/util/sci.h"
#include <cassert>
#include <iostream>

using namespace goby::util;

bool double_cmp(double a, double b, int precision)
{
    int a_whole = a;
    int b_whole = b;

    int a_part = (a-a_whole)*pow(10.0, precision);
    int b_part = (b-b_whole)*pow(10.0, precision);
    
    return (a_whole == b_whole) && (a_part == b_part);
}

int main()
{
    
    assert(ceil_log2(1023) == 10);
    assert(ceil_log2(1024) == 10);
    assert(ceil_log2(1025) == 11);

    assert(ceil_log2(15) == 4);
    assert(ceil_log2(16) == 4);
    assert(ceil_log2(17) == 5);

    assert(ceil_log2(328529398) == 29);

    assert(unbiased_round(5.5, 0) == 6);
    assert(unbiased_round(4.5, 0) == 4);

    assert(double_cmp(unbiased_round(4.123, 2), 4.12, 2));
    
    // check value for mackenzie
    assert(double_cmp(unbiased_round(mackenzie_soundspeed(25,35,1000),3), 1550.744, 3));
	   
    std::cout << "all tests passed" << std::endl;
    return 0;
}
