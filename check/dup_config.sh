#!/bin/bash

. check/common.sh

function baseline {
cat << EOF
duplicate interface definition for eth0
error parsing or activating the config file: $RADVD_CONF
Exiting, failed to read config file.
EOF
}

function output {
cat << EOF > $RADVD_CONF 

interface eth0 {
     AdvSendAdvert on;
     prefix 2002:0000:0000::/64;
};


interface eth0 {
     AdvSendAdvert on;
     prefix 2002:0000:0000::/64;
};

EOF

./radvd -C $RADVD_CONF -c 2>&1 | trim_log || die "radvd failed"
}

run

