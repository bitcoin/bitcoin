#!/usr/bin/env python3
# Copyright (c) 2016-2018 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Specialized SipHash-2-4 implementations.

This implements SipHash-2-4 for 256-bit integers.
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

def siphash256(k0, k1, h):
    n0 = h & ((1 << 64) - 1)
    n1 = (h >> 64) & ((1 << 64) - 1)
    n2 = (h >> 128) & ((1 << 64) - 1)
    n3 = (h >> 192) & ((1 << 64) - 1)
    v0 = 0x736f6d6570736575 ^ k0
    v1 = 0x646f72616e646f6d ^ k1
    v2 = 0x6c7967656e657261 ^ k0
    v3 = 0x7465646279746573 ^ k1 ^ n0
    v0, v1, v2, v3 = siphash_round(v0, v1, v2, v3)
    v0, v1, v2, v3 = siphash_round(v0, v1, v2, v3)
    v0 ^= n0
    v3 ^= n1
    v0, v1, v2, v3 = siphash_round(v0, v1, v2, v3)
    v0, v1, v2, v3 = siphash_round(v0, v1, v2, v3)
    v0 ^= n1
    v3 ^= n2
    v0, v1, v2, v3 = siphash_round(v0, v1, v2, v3)
    v0, v1, v2, v3 = siphash_round(v0, v1, v2, v3)
    v0 ^= n2
    v3 ^= n3
    v0, v1, v2, v3 = siphash_round(v0, v1, v2, v3)
    v0, v1, v2, v3 = siphash_round(v0, v1, v2, v3)
    v0 ^= n3
    v3 ^= 0x2000000000000000
    v0, v1, v2, v3 = siphash_round(v0, v1, v2, v3)
    v0, v1, v2, v3 = siphash_round(v0, v1, v2, v3)
    v0 ^= 0x2000000000000000
    v2 ^= 0xFF
    v0, v1, v2, v3 = siphash_round(v0, v1, v2, v3)
    v0, v1, v2, v3 = siphash_round(v0, v1, v2, v3)
    v0, v1, v2, v3 = siphash_round(v0, v1, v2, v3)
    v0, v1, v2, v3 = siphash_round(v0, v1, v2, v3)
    return v0 ^ v1 ^ v2 ^ v3
