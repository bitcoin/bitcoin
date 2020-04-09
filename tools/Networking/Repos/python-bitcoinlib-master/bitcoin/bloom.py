# Copyright (C) 2013-2014 The python-bitcoinlib developers
#
# This file is part of python-bitcoinlib.
#
# It is subject to the license terms in the LICENSE file found in the top-level
# directory of this distribution.
#
# No part of python-bitcoinlib, including this file, may be copied, modified,
# propagated, or distributed except according to the terms contained in the
# LICENSE file.

"""Bloom filter support"""

from __future__ import absolute_import, division, print_function, unicode_literals

import struct
import sys
import math

import bitcoin.core
import bitcoin.core.serialize

def _ROTL32(x, r):
    assert x <= 0xFFFFFFFF
    return ((x << r) & 0xFFFFFFFF) | (x >> (32 - r))

def MurmurHash3(nHashSeed, vDataToHash):
    """MurmurHash3 (x86_32)

    Used for bloom filters. See http://code.google.com/p/smhasher/source/browse/trunk/MurmurHash3.cpp
    """

    assert nHashSeed <= 0xFFFFFFFF

    h1 = nHashSeed
    c1 = 0xcc9e2d51
    c2 = 0x1b873593

    # body
    i = 0
    while (i < len(vDataToHash) - len(vDataToHash) % 4
           and len(vDataToHash) - i >= 4):

        k1 = struct.unpack(b"<L", vDataToHash[i:i+4])[0]

        k1 = (k1 * c1) & 0xFFFFFFFF
        k1 = _ROTL32(k1, 15)
        k1 = (k1 * c2) & 0xFFFFFFFF

        h1 ^= k1
        h1 = _ROTL32(h1, 13)
        h1 = (((h1*5) & 0xFFFFFFFF) + 0xe6546b64) & 0xFFFFFFFF

        i += 4

    # tail
    k1 = 0
    j = (len(vDataToHash) // 4) * 4
    bord = ord
    if sys.version > '3':
        # In Py3 indexing bytes returns numbers, not characters
        bord = lambda x: x
    if len(vDataToHash) & 3 >= 3:
        k1 ^= bord(vDataToHash[j+2]) << 16
    if len(vDataToHash) & 3 >= 2:
        k1 ^= bord(vDataToHash[j+1]) << 8
    if len(vDataToHash) & 3 >= 1:
        k1 ^= bord(vDataToHash[j])

    k1 &= 0xFFFFFFFF
    k1 = (k1 * c1) & 0xFFFFFFFF
    k1 = _ROTL32(k1, 15)
    k1 = (k1 * c2) & 0xFFFFFFFF
    h1 ^= k1

    # finalization
    h1 ^= len(vDataToHash) & 0xFFFFFFFF
    h1 ^= (h1 & 0xFFFFFFFF) >> 16
    h1 *= 0x85ebca6b
    h1 ^= (h1 & 0xFFFFFFFF) >> 13
    h1 *= 0xc2b2ae35
    h1 ^= (h1 & 0xFFFFFFFF) >> 16

    return h1 & 0xFFFFFFFF


class CBloomFilter(bitcoin.core.serialize.Serializable):
    # 20,000 items with fp rate < 0.1% or 10,000 items and <0.0001%
    MAX_BLOOM_FILTER_SIZE = 36000
    MAX_HASH_FUNCS = 50

    UPDATE_NONE = 0
    UPDATE_ALL = 1
    UPDATE_P2PUBKEY_ONLY = 2
    UPDATE_MASK = 3

    def __init__(self, nElements, nFPRate, nTweak, nFlags):
        """Create a new bloom filter

        The filter will have a given false-positive rate when filled with the
        given number of elements.

        Note that if the given parameters will result in a filter outside the
        bounds of the protocol limits, the filter created will be as close to
        the given parameters as possible within the protocol limits. This will
        apply if nFPRate is very low or nElements is unreasonably high.

        nTweak is a constant which is added to the seed value passed to the
        hash function It should generally always be a random value (and is
        largely only exposed for unit testing)

        nFlags should be one of the UPDATE_* enums (but not _MASK)
        """
        LN2SQUARED = 0.4804530139182014246671025263266649717305529515945455
        LN2 = 0.6931471805599453094172321214581765680755001343602552
        self.vData = bytearray(int(min(-1  / LN2SQUARED * nElements * math.log(nFPRate), self.MAX_BLOOM_FILTER_SIZE * 8) / 8))
        self.nHashFuncs = int(min(len(self.vData) * 8 / nElements * LN2, self.MAX_HASH_FUNCS))
        self.nTweak = nTweak
        self.nFlags = nFlags

    def bloom_hash(self, nHashNum, vDataToHash):
        return MurmurHash3(((nHashNum * 0xFBA4C795) + self.nTweak) & 0xFFFFFFFF, vDataToHash) % (len(self.vData) * 8)

    __bit_mask = bytearray([0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80])

    def insert(self, elem):
        """Insert an element in the filter.

        elem may be a COutPoint or bytes
        """
        if isinstance(elem, bitcoin.core.COutPoint):
            elem = elem.serialize()

        if len(self.vData) == 1 and self.vData[0] == 0xff:
            return

        for i in range(0, self.nHashFuncs):
            nIndex = self.bloom_hash(i, elem)
            # Sets bit nIndex of vData
            self.vData[nIndex >> 3] |= self.__bit_mask[7 & nIndex]

    def contains(self, elem):
        """Test if the filter contains an element

        elem may be a COutPoint or bytes
        """
        if isinstance(elem, bitcoin.core.COutPoint):
            elem = elem.serialize()

        if len(self.vData) == 1 and self.vData[0] == 0xff:
            return True

        for i in range(0, self.nHashFuncs):
            nIndex = self.bloom_hash(i, elem)
            if not (self.vData[nIndex >> 3] & self.__bit_mask[7 & nIndex]):
                return False
        return True

    def IsWithinSizeConstraints(self):
        return len(self.vData) <= self.MAX_BLOOM_FILTER_SIZE and self.nHashFuncs <= self.MAX_HASH_FUNCS

    def IsRelevantAndUpdate(tx, tx_hash):
        # Not useful for a client, so not implemented yet.
        raise NotImplementedError

    __struct = struct.Struct(b'<IIB')

    @classmethod
    def stream_deserialize(cls, f):
        vData = bytearray(bitcoin.core.serialize.BytesSerializer.stream_deserialize(f))
        (nHashFuncs,
         nTweak,
         nFlags) = CBloomFilter.__struct.unpack(bitcoin.core.ser_read(f, CBloomFilter.__struct.size))
        # These arguments can be fake, the real values are set just after
        deserialized = cls(1, 0.01, 0, CBloomFilter.UPDATE_ALL)
        deserialized.vData = vData
        deserialized.nHashFuncs = nHashFuncs
        deserialized.nTweak = nTweak
        deserialized.nFlags = nFlags
        return deserialized

    def stream_serialize(self, f):
        if sys.version > '3':
            bitcoin.core.serialize.BytesSerializer.stream_serialize(self.vData, f)
        else:
            # 2.7 has problems with f.write(bytearray())
            bitcoin.core.serialize.BytesSerializer.stream_serialize(bytes(self.vData), f)
        f.write(self.__struct.pack(self.nHashFuncs, self.nTweak, self.nFlags))

__all__ = (
        'MurmurHash3',
        'CBloomFilter',
)
