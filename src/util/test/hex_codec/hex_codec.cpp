#include "goby/util/binary.h"

#include <iostream>
#include <sstream>
#include <iomanip>

int main()
{
    using goby::util::hex_encode;
    using goby::util::hex_decode;
    
    {
        std::cout << "testing lowercase even" << std::endl;

        std::stringstream ss;
        for(int i = 0; i <= 255; ++i)
            ss << std::setfill('0') << std::setw(2) << std::hex << i;
            
        std::string hex1 = ss.str();
        std::string hex2;
        
        std::string bytes;
        hex_decode(hex1, &bytes);
        hex_encode(bytes, &hex2);
        
        std::cout << "hex1: " << hex1 << std::endl;
        std::cout << "hex2: " << hex2 << std::endl;
        assert(hex1 == hex2.substr(hex2.size() - hex1.size()));
    }
    {
        std::cout << "testing uppercase even" << std::endl;

        std::stringstream ss;
        for(int i = 0; i <= 255; ++i)
            ss << std::uppercase << std::setfill('0') << std::setw(2) << std::hex << i;
            
        std::string hex1 = ss.str();
        std::string hex2;
        
        std::string bytes;
        hex_decode(hex1, &bytes);
        hex_encode(bytes, &hex2, true);
        
        std::cout << "hex1: " << hex1 << std::endl;
        std::cout << "hex2: " << hex2 << std::endl;
        assert(hex1 == hex2.substr(hex2.size() - hex1.size()));
    }
    

    {
        std::cout << "testing lowercase odd" << std::endl;

        std::stringstream ss;
        for(int i = 0; i <= 255; ++i)
            ss << std::setfill('0') << std::setw(2) << std::hex << i;

        ss << "d";
        
        std::string hex1 = ss.str();
        std::string hex2;
        
        std::string bytes;
        hex_decode(hex1, &bytes);
        hex_encode(bytes, &hex2);
        
        std::cout << "hex1: " << hex1 << std::endl;
        std::cout << "hex2: " << hex2 << std::endl;
        assert(hex1 == hex2.substr(hex2.size() - hex1.size()));
    }
    {
        std::cout << "testing uppercase odd" << std::endl;

        std::stringstream ss;
        for(int i = 0; i <= 255; ++i)
            ss << std::uppercase << std::setfill('0') << std::setw(2) << std::hex << i;
        ss << "D";

        
        std::string hex1 = ss.str();
        std::string hex2;
        
        std::string bytes;
        hex_decode(hex1, &bytes);
        hex_encode(bytes, &hex2, true);
        
        std::cout << "hex1: " << hex1 << std::endl;
        std::cout << "hex2: " << hex2 << std::endl;
        assert(hex1 == hex2.substr(hex2.size() - hex1.size()));
    }

    
    std::cout << "all tests passed" << std::endl;
    
    return 0;
}
