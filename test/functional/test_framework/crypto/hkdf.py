#!/usr/bin/env python3
# Copyright (c) 2023 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

"""Test-only HKDF-SHA256 implementation

It is designed for ease of understanding, not performance.

WARNING: This code is slow and trivially vulnerable to side channel attacks. Do not use for
anything but tests.
"""

import hashlib
import hmac


def hmac_sha256(key, data):
    """Compute HMAC-SHA256 from specified byte arrays key and data."""
    return hmac.new(key, data, hashlib.sha256).digest()


def hkdf_sha256(length, ikm, salt, info):
    """Derive a key using HKDF-SHA256."""
    if len(salt) == 0:
        salt = bytes([0] * 32)
    prk = hmac_sha256(salt, ikm)
    t = b""
    okm = b""
    for i in range((length + 32 - 1) // 32):
        t = hmac_sha256(prk, t + info + bytes([i + 1]))
        okm += t
    return okm[:length]
