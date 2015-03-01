#!/bin/sh

# Script to bring up AX25 interface. Assumes interface will receive 'ax0' identifier
# i.e. any existing interface has been brought down using kissdown.sh

# Argument #1: IP address ex: 10.0.0.7
# Argument #2: Netmask    ex: 255.255.255.0

IPADDR=${1}
NETMASK=${2}

sudo modprobe ax25
sudo modprobe mkiss
sudo socat -d -d pty,link=/var/ax25/axp1,raw,echo=0 pty,link=/var/ax25/axp2,raw,echo=0 &
sleep 2
sudo kissattach /var/ax25/axp1 radio1 ${IPADDR}
sudo ifconfig ax0 netmask ${NETMASK}
