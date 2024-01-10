#!/usr/bin/env python3
# Copyright (c) 2016-2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""SipHash-2-4 implementation.

This implements SipHash-2-4. For convenience, an interface taking 256-bit
integers is provided in addition to the one accepting generic data.
"""

def rotl64(n, b):
    return n >> (64 - b) | (n & ((1 << (64 - b)) - 1)) << b


def siphash_round(v0, v1, v2, v3):
    v0 = (v0 + v1) & ((1 << 64) - 1)
    v1 = rotl64(v1, 13)
    v1 ^= v0
    v0 = rotl64(v0, 32)
    v2 = (v2 + v3) & ((1 << 64) - 1)
    v3 = rotl64(v3, 16)
    v3 ^= v2
    v0 = (v0 + v3) & ((1 << 64) - 1)
    v3 = rotl64(v3, 21)
    v3 ^= v0
    v2 = (v2 + v1) & ((1 << 64) - 1)
    v1 = rotl64(v1, 17)
    v1 ^= v2
    v2 = rotl64(v2, 32)
    return (v0, v1, v2, v3)


def siphash(k0, k1, data):
    assert type(data) is bytes
    v0 = 0x736f6d6570736575 ^ k0
    v1 = 0x646f72616e646f6d ^ k1
    v2 = 0x6c7967656e657261 ^ k0
    v3 = 0x7465646279746573 ^ k1
    c = 0
    t = 0
    for d in data:
        t |= d << (8 * (c % 8))
        c = (c + 1) & 0xff
        if (c & 7) == 0:
            v3 ^= t
            v0, v1, v2, v3 = siphash_round(v0, v1, v2, v3)
            v0, v1, v2, v3 = siphash_round(v0, v1, v2, v3)
            v0 ^= t
            t = 0
    t = t | (c << 56)
    v3 ^= t
    v0, v1, v2, v3 = siphash_round(v0, v1, v2, v3)
    v0, v1, v2, v3 = siphash_round(v0, v1, v2, v3)
    v0 ^= t
    v2 ^= 0xff
    v0, v1, v2, v3 = siphash_round(v0, v1, v2, v3)
    v0, v1, v2, v3 = siphash_round(v0, v1, v2, v3)
    v0, v1, v2, v3 = siphash_round(v0, v1, v2, v3)
    v0, v1, v2, v3 = siphash_round(v0, v1, v2, v3)
    return v0 ^ v1 ^ v2 ^ v3


def siphash256(k0, k1, num):
    assert type(num) is int
    return siphash(k0, k1, num.to_bytes(32, 'little'))
