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

#include "goby/pb/goby_app_base.h"
#include "goby/common/time.h"
#include "test.pb.h"
#include <ctime>
#include <google/protobuf/text_format.h>

using goby::util::goby_time;

TestGConfig cfg;

class TestG1 : public GobyAppBase
{
public:
    TestG1()
        : GobyAppBase("test_app1")
        {
            TestMessage a;

            std::cout << goby_time() << ": begin" << std::endl;

            for(int i = 0; i < 10000; ++i)
            {
                a.set_foo(i);
                publish(a);
                // std::string out;
                // google::protobuf::TextFormat::PrintToString(a, &out);
                // glogger << a.GetDescriptor()->full_name() << std::endl;
                // glogger << out << std::endl;
            }
        }
};    

class TestG2 : public GobyAppBase
{
public:
    TestG2()
        : GobyAppBase("test_app2")
        {
            subscribe(&TestG2::handler, this);
        }
    
    void handler(const TestMessage& msg)
        {
            if(msg.foo() == 9999)
                std::cout << goby_time() << ": end" << std::endl;
        }
};

int main()
{
    TestG1 g1;
    g1.run();
}
