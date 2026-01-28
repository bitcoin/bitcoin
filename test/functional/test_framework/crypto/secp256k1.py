# Copyright (c) 2022-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

"""Test-only implementation of low-level secp256k1 field and group arithmetic

It is designed for ease of understanding, not performance.

WARNING: This code is slow and trivially vulnerable to side channel attacks. Do not use for
anything but tests.

Exports:
* FE: class for secp256k1 field elements
* GE: class for secp256k1 group elements
* G: the secp256k1 generator point
"""

import unittest
from hashlib import sha256
from pathlib import Path
import sys

# Prefer the vendored copy of secp256k1lab.
sys.path.insert(0, str(Path(__file__).resolve().parent / "../../../secp256k1lab/src"))
from secp256k1lab.secp256k1 import FE, G, GE, Scalar  # type: ignore[import]

__all__ = ["FE", "G", "GE", "Scalar"]


class TestFrameworkSecp256k1(unittest.TestCase):
    def test_H(self):
        H = sha256(G.to_bytes_uncompressed()).digest()
        assert GE.lift_x(FE.from_bytes_checked(H)) is not None
        self.assertEqual(H.hex(), "50929b74c1a04954b78b4b6035e97a5e078a5a0f28ec96d547bfee9ace803ac0")
