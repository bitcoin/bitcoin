#!/usr/bin/env python3
#
# bignum.py
#
# This file is copied from python-bitcoinlib.
#
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
#

"""Bignum routines"""


import struct


# generic big endian MPI format

def bn_bytes(v, have_ext=False):
    ext = 0
    if have_ext:
        ext = 1
    return ((v.bit_length()+7)//8) + ext

def bn2bin(v):
    s = bytearray()
    i = bn_bytes(v)
    while i > 0:
        s.append((v >> ((i-1) * 8)) & 0xff)
        i -= 1
    return s

def bin2bn(s):
    l = 0
    for ch in s:
        l = (l << 8) | ch
    return l

def bn2mpi(v):
    have_ext = False
    if v.bit_length() > 0:
        have_ext = (v.bit_length() & 0x07) == 0

    neg = False
    if v < 0:
        neg = True
        v = -v

    s = struct.pack(b">I", bn_bytes(v, have_ext))
    ext = bytearray()
    if have_ext:
        ext.append(0)
    v_bin = bn2bin(v)
    if neg:
        if have_ext:
            ext[0] |= 0x80
        else:
            v_bin[0] |= 0x80
    return s + ext + v_bin

def mpi2bn(s):
    if len(s) < 4:
        return None
    s_size = bytes(s[:4])
    v_len = struct.unpack(b">I", s_size)[0]
    if len(s) != (v_len + 4):
        return None
    if v_len == 0:
        return 0

    v_str = bytearray(s[4:])
    neg = False
    i = v_str[0]
    if i & 0x80:
        neg = True
        i &= ~0x80
        v_str[0] = i

    v = bin2bn(v_str)

    if neg:
        return -v
    return v

# bitcoin-specific little endian format, with implicit size
def mpi2vch(s):
    r = s[4:]           # strip size
    r = r[::-1]         # reverse string, converting BE->LE
    return r

def bn2vch(v):
    return bytes(mpi2vch(bn2mpi(v)))

def vch2mpi(s):
    r = struct.pack(b">I", len(s))   # size
    r += s[::-1]            # reverse string, converting LE->BE
    return r

def vch2bn(s):
    return mpi2bn(vch2mpi(s))

