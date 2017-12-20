#!/usr/bin/env python3
# Copyright (c) 2017 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

# Script to test the current dns seeds in use

import socket
import os
import re
import sys

SOCKET_TIMEOUT = 5
MIN_IPV4 = 4
MIN_IPV6 = 5
MIN_CONNECTABLE = MIN_IPV4
CPP_FILE = "../../src/chainparams.cpp"
CHECK_CONNECTION = True

def parse_cpp(filename):
    chainparams_cpp_file = os.path.join(os.path.dirname(__file__), filename)
    with open(chainparams_cpp_file, 'r') as sourcefile:
        data=sourcefile.read()
        seeds = re.findall('vSeeds.emplace_back\(\"(.*)\"', data)
        return seeds

def hilite(string, status, bold):
    if os.name != 'posix':
        return string
    attr = []
    if status:
        # green
        attr.append('32')
    else:
        # red
        attr.append('31')
    if bold:
        attr.append('1')
    return '\x1b[%sm%s\x1b[0m' % (';'.join(attr), string)

def test_connectability(seed_response, ipv6 = False):
    print("  Testing port 8333",end="", flush=True)
    conectable_peers = 0
    for ip in seed_response:
        print(".",end="", flush=True)
        s = socket.socket( (socket.AF_INET6 if ipv6 else socket.AF_INET) , socket.SOCK_STREAM)
        s.settimeout(SOCKET_TIMEOUT)
        try:
            s.connect(ip[4])
            conectable_peers+=1
        except (socket.timeout, socket.error, ConnectionRefusedError):
            print("!",end="", flush=True)
        try:
            s.shutdown(2)
            s.close()
        except:
            pass
    print(" "+str(conectable_peers)+" connectable peers")
    return conectable_peers

def main():
    ret = 0
    seeds = []
    # check if we should overload a seed given as first parameter
    if len(sys.argv) == 2:
        seeds = [sys.argv[1]]
    else:
        #parse chainparams.cpp file
        seeds = parse_cpp(CPP_FILE)
    for seed in seeds:
        v4 = []
        v6 = []
        conectable_v4_peers = 0
        conectable_v6_peers = 0
        print("Testing "+seed+" ...")
        try:
            v4 = socket.getaddrinfo(seed, 8333, socket.AF_INET)
            print("  found IPv4: "+str(len(v4)))
        except:
            print("Failed to retrive DNS records!")
        if CHECK_CONNECTION:
            conectable_v4_peers = test_connectability(v4)
        try:
            v6 = socket.getaddrinfo(seed, 8333, socket.AF_INET6)
            print("  found IPv6: "+str(len(v6)))
        except:
            print("Failed to retrive IPv6 DNS records!")
        #disable IPv6 connection test due to missing widespread support
        #if CHECK_CONNECTION:
        #. conectable_v6_peers = test_connectability(v6, True)
        if len(v4) >= MIN_IPV4 and len(v6) >= MIN_IPV6 and (CHECK_CONNECTION == False or conectable_v4_peers >= MIN_IPV4):
            print("Status: "+hilite("OKAY", True, True)+"\n")
        else:
            print("Status: "+hilite("Failed", False, True)+"\n")
            ret = 1
    sys.exit(ret)

if __name__ == '__main__':
    main()