#!/usr/bin/env python3
#
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Big number routines.

Functions for converting numbers to MPI format and Bitcoin-specific little
endian format.

This file is copied from python-bitcoinlib.
"""
def bn2bin(v):
    """Convert a number to a byte array."""
    s = bytearray()
    bytes_len = (v.bit_length() + 7) // 8
    for i in range(bytes_len, 0, -1):
        s.append((v >> ((i - 1) * 8)) & 0xff)
    return s

def bn2mpi(v):
    """Convert number to MPI format, without the sign byte."""
    have_ext = False
    if v.bit_length() > 0:
        have_ext = (v.bit_length() & 0x07) == 0

    neg = False
    if v < 0:
        neg = True
        v = -v

    ext = bytearray()
    if have_ext:
        ext.append(0)
    v_bin = bn2bin(v)
    if neg:
        if have_ext:
            ext[0] |= 0x80
        else:
            v_bin[0] |= 0x80
    return ext + v_bin

def bn2vch(v):
    """Convert number to bitcoin-specific little endian format."""
    return bytes(reversed(bn2mpi(v)))
