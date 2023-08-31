# Copyright (c) 2020 Pieter Wuille
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Native Python MuHash3072 implementation."""

import hashlib
import unittest

from .chacha20 import chacha20_block

def data_to_num3072(data):
    """Hash a 32-byte array data to a 3072-bit number using 6 Chacha20 operations."""
    bytes384 = b""
    for counter in range(6):
        bytes384 += chacha20_block(data, bytes(12), counter)
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
        data_hash = hashlib.sha256(data).digest()
        self.numerator = (self.numerator * data_to_num3072(data_hash)) % self.MODULUS

    def remove(self, data):
        """Remove a byte array from the set."""
        data_hash = hashlib.sha256(data).digest()
        self.denominator = (self.denominator * data_to_num3072(data_hash)) % self.MODULUS

    def digest(self):
        """Extract the final hash. Does not modify this object."""
        val = (self.numerator * pow(self.denominator, -1, self.MODULUS)) % self.MODULUS
        bytes384 = val.to_bytes(384, 'little')
        return hashlib.sha256(bytes384).digest()

class TestFrameworkMuhash(unittest.TestCase):
    def test_muhash(self):
        muhash = MuHash3072()
        muhash.insert(b'\x00' * 32)
        muhash.insert((b'\x01' + b'\x00' * 31))
        muhash.remove((b'\x02' + b'\x00' * 31))
        finalized = muhash.digest()
        # This mirrors the result in the C++ MuHash3072 unit test
        self.assertEqual(finalized[::-1].hex(), "10d312b100cbd32ada024a6646e40d3482fcff103668d2625f10002a607d5863")
