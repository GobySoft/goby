// Copyright 2009-2012 Toby Schneider (https://launchpad.net/~tes)
//                     Massachusetts Institute of Technology (2007-)
//                     Woods Hole Oceanographic Institution (2007-)
//                     Goby Developers Team (https://launchpad.net/~goby-dev)
// 
//
// This file is part of the Goby Underwater Autonomy Project Libraries
// ("The Goby Libraries").
//
// The Goby Libraries are free software: you can redistribute them and/or modify
// them under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// The Goby Libraries are distributed in the hope that they will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with Goby.  If not, see <http://www.gnu.org/licenses/>.

#include "goby/pb/core.h"
#include "goby/common/time.h"
#include "test.pb.h"
#include <ctime>
#include <google/protobuf/text_format.h>
#include <google/protobuf/io/zero_copy_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <boost/unordered_map.hpp>
#include <Wt/Dbo/Query>
#include <Wt/Dbo/backend/Sqlite3>
#include <Wt/Dbo/Exception>
#include <iostream>
#include <fstream>

using goby::util::goby_time;
using goby::core::proto::Filter;
using goby::core::operator<<;


namespace dbo = Wt::Dbo;

// class User {
// public:
//   enum Role {
//     Visitor = 0,
//     Admin = 1,
//     Alien = 42
//   };

//   std::string name;
//   std::string password;
//   Role        role;
//   int         karma;

//   template<class Action>
//   void persist(Action& a)
//   {
//     dbo::field(a, name,     "name");
//     dbo::field(a, password, "password");
//     dbo::field(a, role,     "role");
//     dbo::field(a, karma,    "karma");
//   }
// };

class TestG1 : public goby::core::ApplicationBase
{
public:
    TestG1() //: goby::core::ApplicationBase(&cfg_)
        {
            glogger().add_group("foo", Colors::blue);
            subscribe<TestMessage>();
        }
    
private:
    void loop()
        {
            glogger() << group("foo") << "in: " << newest<TestMessage>() << std::endl;
        }
    
    // needs to be static so it gets constructed before TestG1
    static TestGConfig cfg_;
};    

TestGConfig TestG1::cfg_;

int main(int argc, char* argv[])
{   
    return goby::run<TestG1>(argc, argv);
}
