# Copyright (c) 2020 Pieter Wuille
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Native Python MuHash3072 implementation."""

import hashlib
import unittest

from .util import modinv
from .chacha20 import ChaCha20


def chacha20_32_to_384(key32):
    """Generate ChaCha20 keystream with 32-byte key, 0 IV, 384-byte output."""
    c = ChaCha20(key32, 0)
    return c.keystream(384)

def data_to_num3072(data):
    """Hash a 32-byte array data to a 3072-bit number using 6 Chacha20 operations."""
    bytes384 = chacha20_32_to_384(data)
    return int.from_bytes(bytes384, 'little')

class MuHash3072:
    """Class representing the MuHash3072 computation of a set.

    See https://cseweb.ucsd.edu/~mihir/papers/inchash.pdf and https://lists.linuxfoundation.org/pipermail/bitcoin-dev/2017-May/014337.html
    """

    MODULUS = 2**3072 - 1103717

    def __init__(self):
        """Initialize for an empty set."""
        self.numerator = 1
        self.denominator = 1

    def insert(self, data):
        """Insert a byte array data in the set."""
        self.numerator = (self.numerator * data_to_num3072(data)) % self.MODULUS

    def remove(self, data):
        """Remove a byte array from the set."""
        self.denominator = (self.denominator * data_to_num3072(data)) % self.MODULUS

    def digest(self):
        """Extract the final hash. Does not modify this object."""
        val = (self.numerator * modinv(self.denominator, self.MODULUS)) % self.MODULUS
        bytes384 = val.to_bytes(384, 'little')
        return hashlib.sha256(bytes384).digest()

class TestFrameworkMuhash(unittest.TestCase):
    def test_muhash(self):
        muhash = MuHash3072()
        muhash.insert([0]*32)
        muhash.insert([1] + [0]*31)
        muhash.remove([2] + [0]*31)
        finalized = muhash.digest()
        # This mirrors the result in the C++ MuHash3072 unit test
        self.assertEqual(finalized[::-1].hex(), "a44e16d5e34d259b349af21c06e65d653915d2e208e4e03f389af750dc0bfdc3")

    def test_chacha20(self):
        def chacha_check(key, result):
            self.assertEqual(chacha20_32_to_384(key)[:64].hex(), result)

        # Test vectors from https://tools.ietf.org/html/draft-agl-tls-chacha20poly1305-04#section-7
        # Since the nonce is hardcoded to 0 in our function we only use those vectors.
        chacha_check([0]*32, "76b8e0ada0f13d90405d6ae55386bd28bdd219b8a08ded1aa836efcc8b770dc7da41597c5157488d7724e03fb8d84a376a43b8f41518a11cc387b669b2ee6586")
        chacha_check([0]*31 + [1], "4540f05a9f1fb296d7736e7b208e3c96eb4fe1834688d2604f450952ed432d41bbe2a0b6ea7566d2a5d1e7e20d42af2c53d792b1c43fea817e9ad275ae546963")
