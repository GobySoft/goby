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

using goby::util::goby_time;
using goby::core::operator<<;


class TestG2 : public goby::core::ApplicationBase
{
public:
    TestG2() : goby::core::ApplicationBase()
        {
            // int i = 0;

            // google::protobuf::TextFormat::Printer printer;
            // printer.SetSingleLineMode(true);
            
            // TestMessage b;
            // b.set_foo(++i);
            // b.set_name("bob");
            // b.set_evalue(TestMessage::YES);
            // b.mutable_subfoo()->set_baz(i);
            // b.add_rstr("fe\n\ne");
            // b.add_rstr("fie");
            // b.add_rstr("foe");
            // //publish(b);
            // std::string str;
            // printer.PrintToString(b, &str);
            // std::cout << str << std::endl;

            // TestMessage c;
            // google::protobuf::TextFormat::ParseFromString(str, &c);
            // std::cout << c << std::endl;
            
            subscribe(&TestG2::handler, this);
        }
    
    void handler(const TestMessage& msg)
        {
            glogger() << "in: " << msg << std::endl;
        }
    void loop()
        {
            static int i = 0;
            TestMessage b;
            //b.mutable_header()->set_iso_time("20000101T000001");
            b.set_foo(++i);
            b.set_name("bob");
            b.set_evalue(TestMessage::YES);
                {
                    Sub* p = b.mutable_subfoo();
                    p->set_baz(i);
                    p->set_bar(2*i);
                    p->add_rint(1);
                    p->add_rint(2);
                    p->add_rint(3);
                    p->add_rint(i);

                }
                // {
                //     Sub* p = b.add_subfoo();
                //     p->set_baz(i+1);
                //     p->set_bar(2*(i+1));
                // }
                // {
                //     Sub* p = b.add_subfoo();
                //     p->set_baz(i);
                //     p->set_bar(2*(i+1));
                // }
                
            
            b.add_rstr("fee");
            b.add_rstr("fie");
            b.add_rstr("foe");
            publish(b);
            glogger() << "out: " << b << std::endl;
        }
    
};

int main(int argc, char* argv[])
{
    return goby::run<TestG2>(argc, argv);
}
