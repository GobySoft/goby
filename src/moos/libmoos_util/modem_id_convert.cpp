// t. schneider tes@mit.edu 12.22.08
// ocean engineering graudate student - mit / whoi joint program
// massachusetts institute of technology (mit)
// laboratory for autonomous marine sensing systems (lamss)      
// 
// this is modem_id_convert.h 
//
// this does lookups from modem to vehicle name and vice versa
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


#include "modem_id_convert.h"
#include <string>
#include <vector>
#include "goby/util/string.h"
#include <boost/algorithm/string.hpp>

using namespace std;
using goby::util::as;

namespace tes
{
    std::string ModemIdConvert::read_lookup_file(string path)
    {
        
        stringstream ss;
        ss << "***********************************" << endl;
        ss << "modem_id_convert trying to read modem id lookup file" << endl;
        

        ifstream fin;

        fin.open(path.c_str(), ifstream::in);

        if (fin.fail())
        {
            string message = "cannot open " + path + " for reading!";
            ss << message << endl;
            return ss.str();
        }

        ss << "reading in modem id lookup table:" << endl;
      
        while(fin)
        {
            string sline;
            getline(fin, sline);
            
            // strip the spaces and comments out
            string::size_type pos = sline.find(' ');
            while(pos != string::npos)
            {
                sline.erase(pos, 1);
                pos = sline.find(' ');
            }
            pos = sline.find('#');
            while(pos != string::npos)
            {
                sline.erase(pos);
                pos = sline.find('#');
            }
            pos = sline.find("//");
            while(pos != string::npos)
            {
                sline.erase(pos);
                pos = sline.find("//");
            }

            // ignore blank lines
            if(sline != "")
            {
                vector<string> line_parsed;
                boost::algorithm::split(line_parsed, sline, boost::is_any_of(","));
                
                if(line_parsed.size() < 3)
                    ss << "invalid line: " <<  sline << endl;
                else
                {
                    string location_name;
                    if(line_parsed.size() < 4)
                        location_name = line_parsed[1];
                    else
                        location_name = line_parsed[3];
                    
                    
                    ss << "modem id [" << line_parsed[0] << "], name [" << line_parsed[1] << "], type [" << line_parsed[2] << "]" << ", location name [" << location_name << "]" << endl;

                    int id = atoi(line_parsed[0].c_str());

                    //add the entry to our lookup vectors

                    names[id] =  line_parsed[1];

                    max_name_length_ = max(names[id].length(), max_name_length_);
                    max_id_ = max(id, max_id_);
                    
                    
                    types[id] =  line_parsed[2];
                    locations[id] = location_name;
                }
            }
        }

        fin.close();
        
        ss << "***********************************" << endl;
        return ss.str();
    }    
        
    string ModemIdConvert::get_name_from_id(int id)
    {
        if(names.count(id))
            return names[id]; // return the found name
        else
            return as<string>(id); // if not in the lookup table, just give the number back as a string
    }
    
    
    string ModemIdConvert::get_type_from_id(int id)
    {
        if(types.count(id))
            return types[id];
        else
            return "unknown_type";    
    }
    
    string ModemIdConvert::get_location_from_id(int id)
    {
        if(locations.count(id))
            return locations[id];
        else
            return as<string>(id);
    }
    
    int ModemIdConvert::get_id_from_name(string name)
    {
        for (map<int,string>::iterator it = names.begin(); it != names.end(); ++it)
        {
            if(boost::iequals(it->second, name))
                return it->first;
        }
        
        
        return int(as<double>(name));
    }
}
