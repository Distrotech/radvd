#!/bin/bash

IFACE_NAME=test1

ip tuntap add mode tun $IFACE_NAME
