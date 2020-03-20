# Copyright (C) 2017 The python-bitcoinlib developers
#
# This file is part of python-bitcoinlib.
#
# It is subject to the license terms in the LICENSE file found in the top-level
# directory of this distribution.
#
# No part of python-bitcoinlib, including this file, may be copied, modified,
# propagated, or distributed except according to the terms contained in the
# LICENSE file.

"""Bech32 encoding and decoding"""

import sys
_bchr = chr
_bord = ord
if sys.version > '3':
    long = int
    _bchr = lambda x: bytes([x])
    _bord = lambda x: x

from bitcoin.segwit_addr import encode, decode
import bitcoin

class Bech32Error(Exception):
    pass

class Bech32ChecksumError(Bech32Error):
    pass

class CBech32Data(bytes):
    """Bech32-encoded data

    Includes a witver and checksum.
    """
    def __new__(cls, s):
        """from bech32 addr to """
        witver, data = decode(bitcoin.params.BECH32_HRP, s)
        if witver is None and data is None:
            raise Bech32Error('Bech32 decoding error')

        return cls.from_bytes(witver, data)

    def __init__(self, s):
        """Initialize from bech32-encoded string

        Note: subclasses put your initialization routines here, but ignore the
        argument - that's handled by __new__(), and .from_bytes() will call
        __init__() with None in place of the string.
        """

    @classmethod
    def from_bytes(cls, witver, witprog):
        """Instantiate from witver and data"""
        if not (0 <= witver <= 16):
            raise ValueError('witver must be in range 0 to 16 inclusive; got %d' % witver)
        self = bytes.__new__(cls, witprog)
        self.witver = witver

        return self

    def to_bytes(self):
        """Convert to bytes instance

        Note that it's the data represented that is converted; the checkum and
        witver is not included.
        """
        return b'' + self

    def __str__(self):
        """Convert to string"""
        return encode(bitcoin.params.BECH32_HRP, self.witver, self)

    def __repr__(self):
        return '%s(%r)' % (self.__class__.__name__, str(self))

__all__ = (
    'Bech32Error',
    'Bech32ChecksumError',
    'CBech32Data',
)
