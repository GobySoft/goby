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
    '/translator_entry *{/+1' '/^  }/' \
   
echo "ProcessConfig = pGobyApp\n{\n  common {" | cat - xx01 > includes/common.pb.cfg
echo "  }\n}" >> includes/common.pb.cfg

echo "  ..." > /tmp/ldots
cat xx00 /tmp/ldots xx02 /tmp/ldots xx09 /tmp/ldots xx11 /tmp/ldots xx13 /tmp/ldots xx15 /tmp/ldots xx17 /tmp/ldots xx19 > includes/pAcommsHandler_reduced.moos


sed 's/^ \{4\}//' xx03 > includes/driver_config.pb.cfg
sed 's/^ \{4\}//' xx04 > includes/driver_mmdriver.pb.cfg
sed 's/^ \{4\}//' xx05 > includes/driver_abc_driver.pb.cfg
sed 's/^ \{4\}//' xx06 > includes/driver_ufield.pb.cfg
sed 's/^ \{4\}//' xx07 > includes/driver_pb.pb.cfg
sed 's/^ \{4\}//' xx08 > includes/driver_udp.pb.cfg
sed 's/^ \{4\}//' xx10 | sed '/frame:/d' | sed '/poll_/d' > includes/mac_config.pb.cfg
sed 's/^ \{4\}//' xx12 > includes/queue_config.pb.cfg
sed 's/^ \{4\}//' xx14 > includes/dccl_config.pb.cfg
sed 's/^ \{4\}//' xx16 > includes/route_config.pb.cfg

rm xx*

moos_goby_liaison -e | sed -e 's/#.*//' -e 's/[ ^I]*$//' -e '/^$/ d' | \
    csplit - \
    '/base *{/+1' '/^}/' \
    '/goby\.common\.protobuf\.moos_scope_config/' \
    '/goby\.common\.protobuf\.pb_commander_config/' \

echo "base {" | cat - xx01 > includes/base.pb.cfg
echo "}" >> includes/base.pb.cfg

cat xx00 /tmp/ldots xx02 > includes/liaison.pb.cfg

cat xx03 > includes/moos_scope_liaison.pb.cfg
cat xx04 > includes/moos_commander_liaison.pb.cfg


rm xx*

pTranslator -e | sed -e 's/#.*//' -e 's/[ ^I]*$//' -e '/^$/ d' | \
    csplit - \
    '/common *{/+1' '/^  }/' \
   
cat xx00 /tmp/ldots xx02 > includes/pTranslator.moos

rm xx*

moos_gateway_g -e | sed -e 's/#.*//' -e 's/[ ^I]*$//' -e '/^$/ d' | \
    csplit - \
    '/base *{/+1' '/^}/' \
   
cat xx00 /tmp/ldots xx02 > includes/moos_gateway_g.pb.cfg

rm xx*

rm /tmp/ldots

