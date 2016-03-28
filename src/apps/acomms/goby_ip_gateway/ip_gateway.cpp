// Copyright 2009-2014 Toby Schneider (https://launchpad.net/~tes)
//                     GobySoft, LLC (2013-)
//                     Massachusetts Institute of Technology (2007-2014)
//                     Goby Developers Team (https://launchpad.net/~goby-dev)
// 
//
// This file is part of the Goby Underwater Autonomy Project Binaries
// ("The Goby Binaries").
//
// The Goby Binaries are free software: you can redistribute them and/or modify
// them under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// The Goby Binaries are distributed in the hope that they will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Goby.  If not, see <http://www.gnu.org/licenses/>.

#include <sys/socket.h>
#include <net/if.h>
#include <linux/if_tun.h>

#include "dccl.h"
#include "dccl/arithmetic/field_codec_arithmetic.h"

#include "ip_gateway_config.pb.h"
#include "goby/pb/application.h"
#include "goby/acomms/protobuf/network_header.pb.h"
#include "goby/util/binary.h"

int tun_alloc(char *dev);

using namespace goby::common::logger;

namespace goby
{
    namespace acomms
    {
    
        class IPGateway : public goby::pb::Application
        {
        public:
            IPGateway(goby::acomms::protobuf::IPGatewayConfig* cfg);
            ~IPGateway();
        private:
            void loop();
            
        private:
            goby::acomms::protobuf::IPGatewayConfig& cfg_;
            dccl::Codec dccl_;
            int tun_fd_;
            unsigned mtu_;
        };

        // no-op identifier codec
        class IPGatewayEmptyIdentifierCodec : public dccl::TypedFixedFieldCodec<dccl::uint32>
        {
        private:
            dccl::Bitset encode()
                { return dccl::Bitset(0, 0); }
        
            dccl::Bitset encode(const goby::uint32& wire_value)
                { return dccl::Bitset(0, 0); }
    
            goby::uint32 decode(dccl::Bitset* bits)
                {
                    return goby::acomms::protobuf::NetworkHeader::descriptor()->options().GetExtension(dccl::msg).id();
                }
            unsigned size() 
                { return 0; }
        };        
    }
}

goby::acomms::IPGateway::IPGateway(goby::acomms::protobuf::IPGatewayConfig* cfg)
    : Application(cfg),
      cfg_(*cfg),
      dccl_("ip_gateway_id_codec"),
      tun_fd_(-1),
      mtu_(1500)
{
    char tun_name[IFNAMSIZ];
    strcpy(tun_name, "\0");
    tun_fd_ = tun_alloc(tun_name);

    if(tun_fd_ < 0)
        glog.is(DIE) && glog << "Could not allocate tun interface. Check permissions?" << std::endl;
    
    std::cout << tun_name << std::endl;    
    
    dccl::dlog.connect(dccl::logger::INFO, &std::cout);
    
    dccl_arithmetic_load(&dccl_);
    
    dccl::arith::protobuf::ArithmeticModel addr_model;
    addr_model.set_name("goby.acomms.NetworkHeader.AddrModel");
    for(int i = 0, n = 32; i <= n; ++i)
    {
        if(i != n)
        {
            int freq = 10;
            if(i == 0 || i == 1)
                freq = 100;
            addr_model.add_value_bound(i);
            addr_model.add_frequency(freq);
        }
        else
        {
            addr_model.add_value_bound(n);
        }
    }
    addr_model.set_eof_frequency(0); 
    addr_model.set_out_of_range_frequency(0);
    dccl::arith::ModelManager::set_model(addr_model);        

    dccl::arith::protobuf::ArithmeticModel port_model;
    port_model.set_name("goby.acomms.NetworkHeader.PortModel");
    for(int i = 0, n = 1; i <= n; ++i)
    {
        if(i != n)
        {
            int freq = 10;
            port_model.add_value_bound(i);
            port_model.add_frequency(freq);
        }
        else
        {
            port_model.add_value_bound(n);
        }
    }
    port_model.set_eof_frequency(0); 
    port_model.set_out_of_range_frequency(0);
    dccl::arith::ModelManager::set_model(port_model);

    dccl_.load<goby::acomms::protobuf::NetworkHeader>();
    
    dccl_.info_all(&std::cout);
    
    goby::acomms::protobuf::NetworkHeader hdr, hdr_out;
    hdr.set_protocol(goby::acomms::protobuf::NetworkHeader::UDP);
    
    hdr.add_srcdest_addr(4);
    hdr.add_srcdest_addr(1);

    hdr.add_srcdest_port(0);
    hdr.add_srcdest_port(0);

    std::string encoded;
    dccl_.encode(&encoded, hdr);
    std::cout << goby::util::hex_encode(encoded) << std::endl;
    dccl_.decode(encoded, &hdr_out);
    std::cout << hdr_out.DebugString() << std::endl;
}

goby::acomms::IPGateway::~IPGateway()
{
    dccl_arithmetic_unload(&dccl_);
}



void goby::acomms::IPGateway::loop()
{
    glog.is(DEBUG1) && glog << "loop()" << std::endl;

    fd_set rd_set;
    FD_ZERO(&rd_set);
    FD_SET(tun_fd_, &rd_set);

    timeval tout = {0};
    int ret = select(tun_fd_+1, &rd_set, 0, 0, &tout);
    glog.is(DEBUG1) && glog << ret << std::endl;

    if (ret < 0 && errno != EINTR) {
        glog.is(WARN) && glog << "Could not select on tun fd." << std::endl;
        return;
    }
    else if(ret > 0)
    {
        char buffer[mtu_];
        int len = read(tun_fd_, buffer, mtu_);
        if ( len < 0 )
        {
            glog.is(WARN) && glog << "tun read error." << std::endl;
        }
        else if ( len == 0 )
        {
            glog.is(DIE) && glog << "tun reached EOF." << std::endl;
        }
        else
        {
            std::string data(buffer, len);
            glog.is(VERBOSE) && glog << "Received " << data.size() << " bytes. " << goby::util::hex_encode(data) << std::endl;            
        }
    }
}


int main(int argc, char* argv[])
{
    dccl::FieldCodecManager::add<goby::acomms::IPGatewayEmptyIdentifierCodec>("ip_gateway_id_codec");
    
    goby::acomms::protobuf::IPGatewayConfig cfg;
    goby::run<goby::acomms::IPGateway>(argc, argv, &cfg);
}
                            
    

// https://www.kernel.org/doc/Documentation/networking/tuntap.txt
//  char *dev should be the name of the device with a format string (e.g.
//  "tun%d"), but (as far as I can see) this can be any valid network device name.
//  Note that the character pointer becomes overwritten with the real device name
//  (e.g. "tun0")
int tun_alloc(char *dev)
{
    struct ifreq ifr;
    int fd, err;

    if( (fd = open("/dev/net/tun", O_RDWR)) < 0 )
        return fd;

    memset(&ifr, 0, sizeof(ifr));

    /* Flags: IFF_TUN   - TUN device (no Ethernet headers) 
     *        IFF_TAP   - TAP device  
     *
     *        IFF_NO_PI - Do not provide packet information  
     */ 
    ifr.ifr_flags = IFF_TUN; 
    if(*dev)
        strncpy(ifr.ifr_name, dev, IFNAMSIZ);

    if((err = ioctl(fd, TUNSETIFF, (void *) &ifr)) < 0)
    {
        close(fd);
        return err;
    }

    strcpy(dev, ifr.ifr_name);
    return fd;
}              
 
