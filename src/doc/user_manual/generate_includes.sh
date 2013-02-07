pAcommsHandler -e | sed -e 's/#.*//' -e 's/[ ^I]*$//' -e '/^$/ d' | \
    csplit - \
    '/common *{/+1' '/^  }/' \
    '/driver_cfg *{/+1' \
    '/micromodem\.protobuf\.Config/' \
    '/ABCDriverConfig/' \
    '/goby\.moos\.protobuf\.Config/' \
    '/PBDriverConfig/' \
    '/UDPDriverConfig/' \
     '/^  }/' \
    '/mac_cfg *{/+1' '/^  }/' \
    '/queue_cfg *{/+1' '/^  }/' \
    '/dccl_cfg *{/+1' '/^  }/' \
    '/route_cfg *{/+1' '/^  }/' \
   
echo "ProcessConfig = pGobyApp\n{\n  common {" | cat - xx01 > includes/common.pb.cfg
echo "  }\n}" >> includes/common.pb.cfg

echo "  ..." > /tmp/ldots
cat xx00 /tmp/ldots xx02 /tmp/ldots xx09 /tmp/ldots xx11 /tmp/ldots xx13 /tmp/ldots xx15 /tmp/ldots xx17 > includes/pAcommsHandler_reduced.moos


sed 's/^ \{4\}//' xx03 > includes/driver_config.pb.cfg
sed 's/^ \{4\}//' xx04 > includes/driver_mmdriver.pb.cfg
sed 's/^ \{4\}//' xx05 > includes/driver_abc_driver.pb.cfg
sed 's/^ \{4\}//' xx06 > includes/driver_ufield.pb.cfg
sed 's/^ \{4\}//' xx07 > includes/driver_pb.pb.cfg
sed 's/^ \{4\}//' xx08 > includes/driver_udp.pb.cfg
sed 's/^ \{4\}//' xx10 > includes/mac_config.pb.cfg
sed 's/^ \{4\}//' xx12 > includes/queue_config.pb.cfg
sed 's/^ \{4\}//' xx14 > includes/dccl_config.pb.cfg
sed 's/^ \{4\}//' xx16 > includes/route_config.pb.cfg

rm xx*