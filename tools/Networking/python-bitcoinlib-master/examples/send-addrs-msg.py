#!/usr/bin/env python3

import socket, time, bitcoin
from bitcoin.messages import msg_version, msg_verack, msg_addr
from bitcoin.net import CAddress


PORT = 18333

bitcoin.SelectParams('testnet') 

def version_pkt(client_ip, server_ip):
    msg = msg_version()
    msg.nVersion = 70002
    msg.addrTo.ip = server_ip
    msg.addrTo.port = PORT
    msg.addrFrom.ip = client_ip
    msg.addrFrom.port = PORT

    return msg

def addr_pkt( str_addrs ):
    msg = msg_addr()
    addrs = []
    for i in str_addrs:
        addr = CAddress()
        addr.port = 18333
        addr.nTime = int(time.time())
        addr.ip = i

        addrs.append( addr )
    msg.addrs = addrs
    return msg

s = socket.socket()

server_ip = "192.168.0.149"
client_ip = "192.168.0.13"

s.connect( (server_ip,PORT) )

# Send Version packet
s.send( version_pkt(client_ip, server_ip).to_bytes() )

# Get Version reply
print(s.recv(1924))

# Send Verack
s.send( msg_verack().to_bytes() )
# Get Verack
print(s.recv(1024))

# Send Addrs
s.send( addr_pkt(["252.11.1.2", "EEEE:7777:8888:AAAA::1"]).to_bytes() )

time.sleep(1) 
s.close()

# debug log on the server should look like:
# accepted connection 192.168.0.13:39979
# send version message: version 70002, blocks=317947, us=****, them=0.0.0.0:0, peer=192.168.0.13:39979
# receive version message: /pythonbitcoin0.0.1/: version 70002, blocks=-1, us=192.168.0.149:18333, them=192.168.0.13:18333, peer=192.168.0.13:39979
# Added 2 addresses from 192.168.0.13: 3 tried, 1706 new
# disconnecting node 192.168.0.13:39979


