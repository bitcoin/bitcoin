#!/usr/bin/env python3
# Copyright (c) 2014-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
'''
Script to generate list of seed nodes for kernel/chainparams.cpp.

This script expects three text files in the directory that is passed as an
argument:

    nodes_main.txt
    nodes_signet.txt
    nodes_test.txt
    nodes_testnet4.txt

These files must consist of lines in the format

    <ip>:<port>
    [<ipv6>]:<port>
    <onion>.onion:<port>
    <i2p>.b32.i2p:<port>

The output will be several data structures with the peers in binary format:

   static const uint8_t chainparams_seed_{main,signet,test,testnet4}[]={
   ...
   }

These should be pasted into `src/chainparamsseeds.h`.
'''

from base64 import b32decode
from enum import Enum
import sys
import os
import re

class BIP155Network(Enum):
    IPV4 = 1
    IPV6 = 2
    TORV2 = 3  # no longer supported
    TORV3 = 4
    I2P = 5
    CJDNS = 6

def name_to_bip155(addr):
    '''Convert address string to BIP155 (networkID, addr) tuple.'''
    if addr.endswith('.onion'):
        vchAddr = b32decode(addr[0:-6], True)
        if len(vchAddr) == 35:
            assert vchAddr[34] == 3
            return (BIP155Network.TORV3, vchAddr[:32])
        elif len(vchAddr) == 10:
            return (BIP155Network.TORV2, vchAddr)
        else:
            raise ValueError('Invalid onion %s' % vchAddr)
    elif addr.endswith('.b32.i2p'):
        vchAddr = b32decode(addr[0:-8] + '====', True)
        if len(vchAddr) == 32:
            return (BIP155Network.I2P, vchAddr)
        else:
            raise ValueError(f'Invalid I2P {vchAddr}')
    elif '.' in addr: # IPv4
        return (BIP155Network.IPV4, bytes((int(x) for x in addr.split('.'))))
    elif ':' in addr: # IPv6 or CJDNS
        sub = [[], []] # prefix, suffix
        x = 0
        addr = addr.split(':')
        for i,comp in enumerate(addr):
            if comp == '':
                if i == 0 or i == (len(addr)-1): # skip empty component at beginning or end
                    continue
                x += 1 # :: skips to suffix
                assert x < 2
            else: # two bytes per component
                val = int(comp, 16)
                sub[x].append(val >> 8)
                sub[x].append(val & 0xff)
        nullbytes = 16 - len(sub[0]) - len(sub[1])
        assert (x == 0 and nullbytes == 0) or (x == 1 and nullbytes > 0)
        addr_bytes = bytes(sub[0] + ([0] * nullbytes) + sub[1])
        if addr_bytes[0] == 0xfc:
            # Assume that seeds with fc00::/8 addresses belong to CJDNS,
            # not to the publicly unroutable "Unique Local Unicast" network, see
            # RFC4193: https://datatracker.ietf.org/doc/html/rfc4193#section-8
            return (BIP155Network.CJDNS, addr_bytes)
        else:
            return (BIP155Network.IPV6, addr_bytes)
    else:
        raise ValueError('Could not parse address %s' % addr)

def parse_spec(s):
    '''Convert endpoint string to BIP155 (networkID, addr, port) tuple.'''
    match = re.match(r'\[([0-9a-fA-F:]+)\](?::([0-9]+))?$', s)
    if match: # ipv6
        host = match.group(1)
        port = match.group(2)
    elif s.count(':') > 1: # ipv6, no port
        host = s
        port = ''
    else:
        (host,_,port) = s.partition(':')

    if not port:
        port = 0
    else:
        port = int(port)

    host = name_to_bip155(host)

    if host[0] == BIP155Network.TORV2:
        return None  # TORV2 is no longer supported, so we ignore it
    else:
        return host + (port, )

def ser_compact_size(l):
    r = b""
    if l < 253:
        r = l.to_bytes(1, "little")
    elif l < 0x10000:
        r = (253).to_bytes(1, "little") + l.to_bytes(2, "little")
    elif l < 0x100000000:
        r = (254).to_bytes(1, "little") + l.to_bytes(4, "little")
    else:
        r = (255).to_bytes(1, "little") + l.to_bytes(8, "little")
    return r

def bip155_serialize(spec):
    '''
    Serialize (networkID, addr, port) tuple to BIP155 binary format.
    '''
    r = b""
    r += spec[0].value.to_bytes(1, "little")
    r += ser_compact_size(len(spec[1]))
    r += spec[1]
    r += spec[2].to_bytes(2, "big")
    return r

def process_nodes(g, f, structname):
    g.write('static const uint8_t %s[] = {\n' % structname)
    for line in f:
        comment = line.find('#')
        if comment != -1:
            line = line[0:comment]
        line = line.strip()
        if not line:
            continue

        spec = parse_spec(line)
        if spec is None:  # ignore this entry (e.g. no longer supported addresses like TORV2)
            continue
        blob = bip155_serialize(spec)
        hoststr = ','.join(('0x%02x' % b) for b in blob)
        g.write(f'    {hoststr},\n')
    g.write('};\n')

def main():
    if len(sys.argv)<2:
        print(('Usage: %s <path_to_nodes_txt>' % sys.argv[0]), file=sys.stderr)
        sys.exit(1)
    g = sys.stdout
    indir = sys.argv[1]
    g.write('#ifndef BITCOIN_CHAINPARAMSSEEDS_H\n')
    g.write('#define BITCOIN_CHAINPARAMSSEEDS_H\n')
    g.write('/**\n')
    g.write(' * List of fixed seed nodes for the bitcoin network\n')
    g.write(' * AUTOGENERATED by contrib/seeds/generate-seeds.py\n')
    g.write(' *\n')
    g.write(' * Each line contains a BIP155 serialized (networkID, addr, port) tuple.\n')
    g.write(' */\n')
    with open(os.path.join(indir,'nodes_main.txt'), 'r') as f:
        process_nodes(g, f, 'chainparams_seed_main')
    g.write('\n')
    with open(os.path.join(indir,'nodes_signet.txt'), 'r') as f:
        process_nodes(g, f, 'chainparams_seed_signet')
    g.write('\n')
    with open(os.path.join(indir,'nodes_test.txt'), 'r') as f:
        process_nodes(g, f, 'chainparams_seed_test')
    g.write('\n')
    with open(os.path.join(indir,'nodes_testnet4.txt'), 'r') as f:
        process_nodes(g, f, 'chainparams_seed_testnet4')
    g.write('#endif // BITCOIN_CHAINPARAMSSEEDS_H\n')

if __name__ == '__main__':
    main()
