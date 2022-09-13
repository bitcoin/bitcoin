#!/usr/bin/env python3
# Copyright (c) 2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test-only Elligator Swift implementation

WARNING: This code is slow and uses bad randomness.
Do not use for anything but tests."""

import random

from test_framework.secp256k1 import FE, G, GE

# Precomputed constant square root of -3 (mod p).
MINUS_3_SQRT = FE(-3).sqrt()

def xswiftec(u, t):
    """Decode field elements (u, t) to an X coordinate on the curve."""
    if u == 0:
        u = FE(1)
    if t == 0:
        t = FE(1)
    if u**3 + t**2 + 7 == 0:
        t = 2 * t
    X = (u**3 + 7 - t**2) / (2 * t)
    Y = (X + t) / (MINUS_3_SQRT * u)
    for x in (u + 4 * Y**2, (-X / Y - u) / 2, (X / Y - u) / 2):
        if GE.is_valid_x(x):
            return x
    assert False

def xswiftec_inv(x, u, case):
    """Given x and u, find t such that xswiftec(u, t) = x, or return None.

    Case selects which of the up to 8 results to return."""

    if case & 2 == 0:
        if GE.is_valid_x(-x - u):
            return None
        v = x
        s = -(u**3 + 7) / (u**2 + u*v + v**2)
    else:
        s = x - u
        if s == 0:
            return None
        r = (-s * (4 * (u**3 + 7) + 3 * s * u**2)).sqrt()
        if r is None:
            return None
        if case & 1 and r == 0:
            return None
        v = (-u + r / s) / 2
    w = s.sqrt()
    if w is None:
        return None
    if case & 5 == 0:
        return -w * (u * (1 - MINUS_3_SQRT) / 2 + v)
    if case & 5 == 1:
        return w * (u * (1 + MINUS_3_SQRT) / 2 + v)
    if case & 5 == 4:
        return w * (u * (1 - MINUS_3_SQRT) / 2 + v)
    if case & 5 == 5:
        return -w * (u * (1 + MINUS_3_SQRT) / 2 + v)

def xelligatorswift(x):
    """Given a field element X on the curve, find (u, t) that encode them."""
    assert GE.is_valid_x(x)
    while True:
        u = FE(random.randrange(1, FE.SIZE))
        case = random.randrange(0, 8)
        t = xswiftec_inv(x, u, case)
        if t is not None:
            return u, t

def ellswift_create():
    """Generate a (privkey, ellswift_pubkey) pair."""
    priv = random.randrange(1, GE.ORDER)
    u, t = xelligatorswift((priv * G).x)
    return priv.to_bytes(32, 'big'), u.to_bytes() + t.to_bytes()

def ellswift_ecdh_xonly(pubkey_theirs, privkey):
    """Compute X coordinate of shared ECDH point between ellswift pubkey and privkey."""
    u = FE(int.from_bytes(pubkey_theirs[:32], 'big'))
    t = FE(int.from_bytes(pubkey_theirs[32:], 'big'))
    d = int.from_bytes(privkey, 'big')
    return (d * GE.lift_x(xswiftec(u, t))).x.to_bytes()
