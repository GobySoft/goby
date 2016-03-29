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

#include "dccl/arithmetic/field_codec_arithmetic.h"

#include "ip_gateway_config.pb.h"
#include "goby/pb/application.h"
#include "goby/util/binary.h"
#include "goby/acomms/ip_codecs.h"

int tun_alloc(char *dev);
int tun_config(const char* dev, const char* host, unsigned cidr_prefix);

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
            void handle_udp_packet(const goby::acomms::protobuf::IPv4Header& ip_hdr,
                                   const goby::acomms::protobuf::UDPHeader& udp_hdr,
                                   std::string payload);
            void write_udp_packet(goby::acomms::protobuf::IPv4Header& ip_hdr,
                                  goby::acomms::protobuf::UDPHeader& udp_hdr,
                                  std::string payload);

        private:
            goby::acomms::protobuf::IPGatewayConfig& cfg_;
            dccl::Codec dccl_goby_nh_, dccl_ip_, dccl_udp_;
            int tun_fd_;
            unsigned mtu_;
        };        
    }
}

goby::acomms::IPGateway::IPGateway(goby::acomms::protobuf::IPGatewayConfig* cfg)
    : Application(cfg),
      cfg_(*cfg),
      dccl_goby_nh_("ip_gateway_id_codec_0"),
      dccl_ip_("ip_gateway_id_codec_1"),
      dccl_udp_("ip_gateway_id_codec_2"),
      tun_fd_(-1),
      mtu_(1500)
{
    char tun_name[IFNAMSIZ];
    strcpy(tun_name, "\0");
    tun_fd_ = tun_alloc(tun_name);    
    if(tun_fd_ < 0)
        glog.is(DIE) && glog << "Could not allocate tun interface. Check permissions?" << std::endl;

    int ret = tun_config(tun_name, cfg_.local_ip_address().c_str(), cfg_.cidr_netmask_prefix());
    if(ret < 0)
        glog.is(DIE) && glog << "Could not configure tun interface. Check IP address: " << cfg_.local_ip_address() << " and netmask prefix: " << cfg_.cidr_netmask_prefix() << std::endl;
    
    
    
    dccl::dlog.connect(dccl::logger::INFO, &std::cout);
    
    dccl_arithmetic_load(&dccl_goby_nh_);
    
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

    dccl_goby_nh_.load<goby::acomms::protobuf::NetworkHeader>();
    dccl_ip_.load<goby::acomms::protobuf::IPv4Header>();
    dccl_udp_.load<goby::acomms::protobuf::UDPHeader>();
    
    dccl_goby_nh_.info_all(&std::cout);
    
    goby::acomms::protobuf::NetworkHeader hdr, hdr_out;
    hdr.set_protocol(goby::acomms::protobuf::NetworkHeader::UDP);
    
    hdr.add_srcdest_addr(4);
    hdr.add_srcdest_addr(1);

    hdr.add_srcdest_port(0);
    hdr.add_srcdest_port(0);

    std::string encoded;
    dccl_goby_nh_.encode(&encoded, hdr);
    std::cout << goby::util::hex_encode(encoded) << std::endl;
    dccl_goby_nh_.decode(encoded, &hdr_out);
    std::cout << hdr_out.DebugString() << std::endl;
}

goby::acomms::IPGateway::~IPGateway()
{
    dccl_arithmetic_unload(&dccl_goby_nh_);
}



void goby::acomms::IPGateway::loop()
{
    fd_set rd_set;
    FD_ZERO(&rd_set);
    FD_SET(tun_fd_, &rd_set);

    timeval tout = {0};
    int ret = select(tun_fd_+1, &rd_set, 0, 0, &tout);

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
            goby::acomms::protobuf::IPv4Header ip_hdr;
            unsigned short ip_header_size = (buffer[0] & 0xF) * 4;
            unsigned short version = ((buffer[0] >> 4) & 0xF);
            if(version == 4)
            {
                std::string header_data(buffer, ip_header_size);
                dccl_ip_.decode(header_data, &ip_hdr);
                glog.is(DEBUG2) && glog << "Received " << len << " bytes. " << std::endl;
                switch(ip_hdr.protocol())
                {
                    default:
                        glog.is(DEBUG1) && glog << "IPv4 Protocol " << ip_hdr.protocol() << " is not supported." << std::endl;
                        break;
                    case IPPROTO_UDP:
                    {
                        const int udp_header_size = 8;
                        goby::acomms::protobuf::UDPHeader udp_hdr;
                        std::string udp_header_data(&buffer[ip_header_size], udp_header_size); 
                        dccl_udp_.decode(udp_header_data, &udp_hdr);
                        handle_udp_packet(ip_hdr, udp_hdr, std::string(&buffer[ip_header_size+udp_header_size], ip_hdr.total_length()-ip_header_size-udp_header_size));
                        break;
                    }
                }
            }
            
        }
    }
}

void goby::acomms::IPGateway::handle_udp_packet(
    const goby::acomms::protobuf::IPv4Header& ip_hdr,
    const goby::acomms::protobuf::UDPHeader& udp_hdr,
    std::string payload)
{
    
    glog.is(VERBOSE) && glog << "Received UDP Packet. IPv4 Header: " << ip_hdr.DebugString() << "UDP Header: " << udp_hdr <<  "Payload (" << payload.size() << " bytes): " << goby::util::hex_encode(payload) << std::endl;

    // echo for now
    goby::acomms::protobuf::IPv4Header new_ip_hdr = ip_hdr;
    goby::acomms::protobuf::UDPHeader new_udp_hdr = udp_hdr;
    new_ip_hdr.mutable_source_ip_address()->swap(*new_ip_hdr.mutable_dest_ip_address());
    
    const unsigned new_source_port = udp_hdr.dest_port();
    const unsigned new_dest_port = udp_hdr.source_port();
    new_udp_hdr.set_source_port(new_source_port);
    new_udp_hdr.set_dest_port(new_dest_port);
    
    write_udp_packet(new_ip_hdr, new_udp_hdr, payload);
}


void goby::acomms::IPGateway::write_udp_packet(goby::acomms::protobuf::IPv4Header& ip_hdr, goby::acomms::protobuf::UDPHeader& udp_hdr, std::string payload)
{
    // set checksum 0 for calculation
    ip_hdr.set_header_checksum(0);
    udp_hdr.set_checksum(0);
    
    std::string ip_hdr_data, udp_hdr_data;
    dccl_udp_.encode(&udp_hdr_data, udp_hdr);
    dccl_ip_.encode(&ip_hdr_data, ip_hdr);

    enum { NET_SHORT_BYTES = 2, NET_LONG_BYTES = 4 };
    enum { IPV4_SOURCE_ADDR_OFFSET = 12, IPV4_DEST_ADDR_OFFSET = 16, IPV4_CS_OFFSET = 10 };
    enum { UDP_LENGTH_OFFSET = 4, UDP_CS_OFFSET = 6};
    
    std::string udp_pseudo_header = ip_hdr_data.substr(IPV4_SOURCE_ADDR_OFFSET, NET_LONG_BYTES) + ip_hdr_data.substr(IPV4_DEST_ADDR_OFFSET, NET_LONG_BYTES) + char(0) + char(IPPROTO_UDP) + udp_hdr_data.substr(UDP_LENGTH_OFFSET, NET_SHORT_BYTES);
    assert(udp_pseudo_header.size() == 12);

    uint16_t ip_checksum = net_checksum(ip_hdr_data);
    uint16_t udp_checksum = net_checksum(udp_pseudo_header + udp_hdr_data + payload);

    ip_hdr_data[IPV4_CS_OFFSET] = (ip_checksum >> 8) & 0xFF;
    ip_hdr_data[IPV4_CS_OFFSET+1] = ip_checksum & 0xFF;

    udp_hdr_data[UDP_CS_OFFSET] = (udp_checksum >> 8) & 0xFF;
    udp_hdr_data[UDP_CS_OFFSET+1] = udp_checksum & 0xFF;

    
    std::string packet(ip_hdr_data + udp_hdr_data + payload);
    unsigned len = write(tun_fd_, packet.c_str(), packet.size());
    if(len < packet.size())
        glog.is(WARN) && glog << "Failed to write all " << packet.size() << " bytes." << std::endl;
}

int main(int argc, char* argv[])
{
    dccl::FieldCodecManager::add<goby::acomms::IPGatewayEmptyIdentifierCodec<0xF000> >("ip_gateway_id_codec_0");
    dccl::FieldCodecManager::add<goby::acomms::IPGatewayEmptyIdentifierCodec<0xF001> >("ip_gateway_id_codec_1");
    dccl::FieldCodecManager::add<goby::acomms::IPGatewayEmptyIdentifierCodec<0xF002> >("ip_gateway_id_codec_2");
    dccl::FieldCodecManager::add<goby::acomms::NetShortCodec>("net.short");
    dccl::FieldCodecManager::add<goby::acomms::IPv4AddressCodec>("ip.v4.address");
    dccl::FieldCodecManager::add<goby::acomms::IPv4FlagsFragOffsetCodec>("ip.v4.flagsfragoffset");
    
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
    ifr.ifr_flags = IFF_TUN | IFF_NO_PI; 
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

// from https://stackoverflow.com/questions/5308090/set-ip-address-using-siocsifaddr-ioctl
int tun_config(const char* dev, const char* host, unsigned cidr_prefix)
{
    // set address 

    struct ifreq ifr;
    struct sockaddr_in* sai = (struct sockaddr_in *)&ifr.ifr_addr;
    int sockfd;
        
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    memset(&ifr, 0, sizeof(ifr));

    strncpy(ifr.ifr_name, dev, IFNAMSIZ);


    sai->sin_family = AF_INET;
    sai->sin_port = 0;

    int ret = inet_aton(host, &sai->sin_addr);
    if(ret == 0)
        return -1;

    if(ioctl(sockfd, SIOCSIFADDR, &ifr) == -1)
        return -1;
        
    if(ioctl(sockfd, SIOCGIFFLAGS, &ifr) == -1)
        return -1;
        
    ifr.ifr_flags |= IFF_UP | IFF_RUNNING;

    if(ioctl(sockfd, SIOCSIFFLAGS, &ifr) == -1)
        return -1;


    // set mask
        
    struct ifreq ifr_mask;
    struct sockaddr_in* sai_mask = (struct sockaddr_in *)&ifr_mask.ifr_addr;
    memset(&ifr_mask, 0, sizeof(ifr_mask));
    strncpy(ifr_mask.ifr_name, dev, IFNAMSIZ);

    sai_mask->sin_family = AF_INET;
    sai_mask->sin_port = 0;

    if(cidr_prefix > 32)
        return -1;
        
    sai_mask->sin_addr.s_addr = htonl(0xFFFFFFFF - ((1 << (32-cidr_prefix))-1));

    if(ioctl(sockfd, SIOCSIFNETMASK, &ifr_mask) == -1)
        return -1;
        
    close(sockfd);
    
    return 0;    
}

