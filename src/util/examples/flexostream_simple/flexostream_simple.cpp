// copyright 2011 t. schneider tes@mit.edu
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

#include "goby/util/logger.h"
#include <boost/asio/deadline_timer.hpp> // for deadline_timer
#include <fstream>

using goby::util::glogger;
using goby::util::Colors;
using namespace goby::util::tcolor; // red, blue, etc.

void output();

int main(int argc, char* argv[])
{
    if(argc < 2)
    {
        std::cout << "usage: flexostream quiet|warn|verbose|debug|gui [file.txt]"
                  << std::endl;
        return 1;
    }

    // write our name with each log entry
    glogger().set_name(argv[0]);

    // set colors and descriptions for the groups
    glogger().add_group("a", Colors::green, "group a");
    glogger().add_group("b", Colors::magenta, "group b");
    glogger().add_group("c", Colors::blue, "group c");
    
    std::string verbosity = argv[1];

    std::ofstream fout;
    if(argc == 3)
    {
        fout.open(argv[2]);
        if(fout.is_open())
        {            
            glogger().add_stream(goby::util::Logger::debug, &fout);
        }
        else
        {
            std::cerr << "Could not open " << argv[2] << " for writing!" << std::endl;
            return 1;
        }
    }
        
    if(verbosity == "quiet")
    {
        std::cout << "--- testing quiet ---" << std::endl;
        // add a stream with the quiet setting
        glogger().add_stream(goby::util::Logger::quiet, &std::cout);
        output();
    }
    else if(verbosity == "warn")
    {
        std::cout << "--- testing warn ---" << std::endl;
        // add a stream with the quiet setting
        glogger().add_stream(goby::util::Logger::warn, &std::cout);
        output();
    }
    else if(verbosity == "verbose")
    {
        std::cout << "--- testing verbose ---" << std::endl;
        // add a stream with the quiet setting
        glogger().add_stream(goby::util::Logger::verbose, &std::cout);
        output();
    }
    else if(verbosity == "debug")
    {
        std::cout << "--- testing debug ---" << std::endl;
        // add a stream with the quiet setting
        glogger().add_stream(goby::util::Logger::debug, &std::cout);
        output();
    }
    else if(verbosity == "gui")
    {
        std::cout << "--- testing gui ---" << std::endl;
        // add a stream with the quiet setting
        glogger().add_stream(goby::util::Logger::gui, &std::cout);
        output();

        const int CLOSE_TIME = 60;
        glogger() << warn << "closing in " << CLOSE_TIME << " seconds!" << std::endl;

        // `sleep` is not thread safe, so we use a boost::asio::deadline_timer
        boost::asio::io_service io_service;
        boost::asio::deadline_timer timer(io_service);        
        timer.expires_from_now(boost::posix_time::seconds(CLOSE_TIME));
        timer.wait();        
    }
    else
    {
        std::cout << "invalid verbosity setting: " << verbosity << std::endl;
        return 1;
    }

    if(fout.is_open())
        fout.close();
    return 0;
}

void output()
{
    glogger() << warn << "this is warning text" << std::endl;
    glogger() << "this is normal text" << std::endl;
    glogger() << lt_blue << "this is light blue text (in color terminals)"
              << nocolor << std::endl;
    glogger() << debug << "this is debug text" << std::endl;

    glogger() << group("a") << "this text is related to a" << std::endl;
    glogger() << group("b") << "this text is related to b" << std::endl;
    glogger() << group("c") << warn << "this warning is related to c" << std::endl;

}
