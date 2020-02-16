# Copyright (C) 2015-2016 The bitcoin-blockchain-parser developers
#
# This file is part of bitcoin-blockchain-parser.
#
# It is subject to the license terms in the LICENSE file found in the top-level
# directory of this distribution.
#
# No part of bitcoin-blockchain-parser, including this file, may be copied,
# modified, propagated, or distributed except according to the terms contained
# in the LICENSE file.

from bitcoin import base58
from .utils import btc_ripemd160, double_sha256


class Address(object):
    """Represents a bitcoin address"""

    def __init__(self, hash, public_key, address, type):
        self._hash = hash
        self.public_key = public_key
        self._address = address
        self.type = type

    def __repr__(self):
        return "Address(addr=%s)" % self.address

    @classmethod
    def from_public_key(cls, public_key):
        """Constructs an Address object from a public key"""
        return cls(None, public_key, None, "normal")

    @classmethod
    def from_ripemd160(cls, hash, type="normal"):
        """Constructs an Address object from a RIPEMD-160 hash, it may be a
        normal address or a P2SH address, the latter is indicated by setting
        type to 'p2sh'"""
        return cls(hash, None, None, type)

    @property
    def hash(self):
        """Returns the RIPEMD-160 hash corresponding to this address"""
        if self.public_key is not None and self._hash is None:
            self._hash = btc_ripemd160(self.public_key)

        return self._hash

    @property
    def address(self):
        """Returns the base58 encoded representation of this address"""
        if self._address is None:
            version = b'\x00' if self.type == "normal" else b'\x05'
            checksum = double_sha256(version + self.hash)

            self._address = base58.encode(version + self.hash + checksum[:4])
        return self._address

    def is_p2sh(self):
        return self.type == "p2sh"
