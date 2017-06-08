# Copyright (c) 2013 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

#network interface on which to limit traffic
IF="eth0"
#limit of the network interface in question
LINKCEIL="1gbit"
#limit outbound Bitcoin protocol traffic to this rate
LIMIT="160kbit"
#defines the address space for which you wish to disable rate limiting
LOCALNET="192.168.0.0/16"

#delete existing rules
tc qdisc del dev ${IF} root

#add root class
tc qdisc add dev ${IF} root handle 1: htb default 10

#add parent class
tc class add dev ${IF} parent 1: classid 1:1 htb rate ${LINKCEIL} ceil ${LINKCEIL}

#add our two classes. one unlimited, another limited
tc class add dev ${IF} parent 1:1 classid 1:10 htb rate ${LINKCEIL} ceil ${LINKCEIL} prio 0
tc class add dev ${IF} parent 1:1 classid 1:11 htb rate ${LIMIT} ceil ${LIMIT} prio 1

#add handles to our classes so packets marked with <x> go into the class with "... handle <x> fw ..."
tc filter add dev ${IF} parent 1: protocol ip prio 1 handle 1 fw classid 1:10
tc filter add dev ${IF} parent 1: protocol ip prio 2 handle 2 fw classid 1:11

#delete any existing rules
#disable for now
#ret=0
#while [ $ret -eq 0 ]; do
#	iptables -t mangle -D OUTPUT 1
#	ret=$?
#done

#limit outgoing traffic to and from port 8333. but not when dealing with a host on the local network
#	(defined by $LOCALNET)
#	--set-mark marks packages matching these criteria with the number "2"
#	these packages are filtered by the tc filter with "handle 2"
#	this filter sends the packages into the 1:11 class, and this class is limited to ${LIMIT}
iptables -t mangle -A OUTPUT -p tcp -m tcp --dport 8333 ! -d ${LOCALNET} -j MARK --set-mark 0x2
iptables -t mangle -A OUTPUT -p tcp -m tcp --sport 8333 ! -d ${LOCALNET} -j MARK --set-mark 0x2
