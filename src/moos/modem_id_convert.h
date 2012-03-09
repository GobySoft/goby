#ifndef MODEM_ID_CONVERTH
#define MODEM_ID_CONVERTH

#include <iostream>
#include <fstream>
#include <sstream>
#include <map>

namespace tes
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
        
      private:
        std::map<int, std::string> names;
        std::map<int, std::string> types;
        std::map<int, std::string> locations;

        size_t max_name_length_;
        int max_id_;
    };    
}

#endif
