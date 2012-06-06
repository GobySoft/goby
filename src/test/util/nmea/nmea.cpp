// Copyright 2009-2012 Toby Schneider (https://launchpad.net/~tes)
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
#include "goby/util/linebasedcomms.h"
#include "goby/util/binary.h"

int main()
{
    std::string str("");
    std::vector<std::string> vec;
    boost::split(vec, str, boost::is_any_of(","));

    assert(vec.size() == 1 && vec.at(0) == "");
    
    {
        goby::util::NMEASentence nmea;
        nmea.push_back("$FOOBA");
        nmea.push_back(1);
        nmea.push_back(2);
        nmea.push_back(3);
        nmea.push_back("");
        std::cout << nmea.message() << std::endl;
        assert(nmea.message() == "$FOOBA,1,2,3,*75");
    }
    
    {
        goby::util::NMEASentence nmea;
        nmea.push_back("$CCTXD");
        nmea.push_back(2);
        nmea.push_back("1,1");
        nmea.push_back(goby::util::hex_encode(""));
        std::cout << nmea.message() << std::endl;
        assert(nmea.message() == "$CCTXD,2,1,1,*7A");
    }
    
    std::cout << "all tests passed" << std::endl;
    
    return 0;
}
