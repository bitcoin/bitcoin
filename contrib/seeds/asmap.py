#!/usr/bin/env python3
# Copyright (c) 2013-2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
import ipaddress

# Convert a byte array to a bit array
def DecodeBytes(byts):
    return [(byt >> i) & 1 for byt in byts for i in range(8)]

def DecodeBits(stream, bitpos, minval, bit_sizes):
    val = minval
    for pos in range(len(bit_sizes)):
        bit_size = bit_sizes[pos]
        if pos + 1 < len(bit_sizes):
            bit = stream[bitpos]
            bitpos += 1
        else:
            bit = 0
        if bit:
            val += (1 << bit_size)
        else:
            for b in range(bit_size):
                bit = stream[bitpos]
                bitpos += 1
                val += bit << (bit_size - 1 - b)
            return (val, bitpos)
    assert(False)

def DecodeType(stream, bitpos):
    return DecodeBits(stream, bitpos, 0, [0, 0, 1])

def DecodeASN(stream, bitpos):
    return DecodeBits(stream, bitpos, 1, [15, 16, 17, 18, 19, 20, 21, 22, 23, 24])

def DecodeMatch(stream, bitpos):
    return DecodeBits(stream, bitpos, 2, [1, 2, 3, 4, 5, 6, 7, 8])

def DecodeJump(stream, bitpos):
    return DecodeBits(stream, bitpos, 17, [5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30])

def Interpret(asmap, num, bits):
    pos = 0
    default = None
    while True:
        assert(len(asmap) >= pos + 1)
        (opcode, pos) = DecodeType(asmap, pos)
        if opcode == 0:
            (asn, pos) = DecodeASN(asmap, pos)
            return asn
        elif opcode == 1:
            (jump, pos) = DecodeJump(asmap, pos)
            if (num >> (bits - 1)) & 1:
                pos += jump
            bits -= 1
        elif opcode == 2:
            (match, pos) = DecodeMatch(asmap, pos)
            matchlen = match.bit_length() - 1
            for bit in range(matchlen):
                if ((num >> (bits - 1)) & 1) != ((match >> (matchlen - 1 - bit)) & 1):
                    return default
                bits -= 1
        elif opcode == 3:
            (default, pos) = DecodeASN(asmap, pos)
        else:
            assert(False)



def decode_ip(ip: str) -> int:
    addr = ipaddress.ip_address(ip)
    if isinstance(addr, ipaddress.IPv4Address):
        return int.from_bytes(addr.packed, 'big') + 0xffff00000000
    elif isinstance(addr, ipaddress.IPv6Address):
        return int.from_bytes(addr.packed, 'big')

class ASMap:
    def __init__(self, filename):
        '''
        Instantiate an ASMap from a file.
        '''
        with open(filename, "rb") as f:
            self.asmap = DecodeBytes(f.read())

    def lookup_asn(self, ip):
        '''
        Look up the ASN for an IP, returns an ASN id as integer or None if not
        known.
        '''
        return Interpret(self.asmap, decode_ip(ip), 128)
