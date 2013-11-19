#!/bin/bash

set -u

if (( $# != 1 )) ; then
	echo Usage: $0 ifname
	exit 1;
fi

IFACE_NAME=$1
REMOVE_IFACE=0;

function die {
	echo ERROR: $1
	if (( $REMOVE_IFACE != 0 )) ; then
		ip link delete $IFACE_NAME
	fi
	exit 1
}

ip link add $IFACE_NAME type dummy || die "unable to create dummy interface $IFACE_NAME"
REMOVE_IFACE=1
ifconfig $IFACE_NAME up || die "unable to bring up dummy interface $IFACE_NAME"

cat << EOF > radvd.conf || die "unable to write radvd.conf"

interface $IFACE_NAME {
     AdvSendAdvert on;
     MinRtrAdvInterval 20;
     MaxRtrAdvInterval 60;
     AdvLinkMTU 1472;
     prefix 2002:0000:0000::/64 {
             AdvOnLink off;
             AdvAutonomous on;
             AdvRouterAddr on; 
             AdvPreferredLifetime 90;
             AdvValidLifetime 120;
     };
     RDNSS 2001:470:20::2
     {
             AdvRDNSSLifetime 60;
     };
};

EOF

./radvd -m stderr -d 5 --config radvd.conf || die "unable to run radvd"
rm radvd.conf || die "unable to rm radvd.conf"

ip link delete $IFACE_NAME

