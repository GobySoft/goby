// Copyright 2009-2013 Toby Schneider (https://launchpad.net/~tes)
//                     Massachusetts Institute of Technology (2007-)
//                     Woods Hole Oceanographic Institution (2007-)
//                     Goby Developers Team (https://launchpad.net/~goby-dev)
// 
//
// This file is part of the Goby Underwater Autonomy Project MOOS Interface Library
// ("The Goby MOOS Library").
//
// The Goby MOOS Library is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// The Goby MOOS Library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Goby.  If not, see <http://www.gnu.org/licenses/>.

#ifndef MODEM_ID_CONVERTH
#define MODEM_ID_CONVERTH

#include <iostream>
#include <fstream>
#include <sstream>
#include <map>

namespace goby
{
    namespace moos
    {
        
        class ModemIdConvert
        {
          public:
          ModemIdConvert() : max_name_length_(0),
                max_id_(0)
                {}
        
            std::string read_lookup_file(std::string path);
        
            std::string get_name_from_id(int id);
            std::string get_type_from_id(int id);
            std::string get_location_from_id(int id);
            int get_id_from_name(std::string name);

            size_t max_name_length() {return max_name_length_;}
            int max_id() {return max_id_;}        

            const std::map<int, std::string>& names() const
            { return names_; }
            
        
          private:
            std::map<int, std::string> names_;
            std::map<int, std::string> types_;
            std::map<int, std::string> locations_;

            size_t max_name_length_;
            int max_id_;
        };    
    }
}

#endif
