#!/usr/bin/env python3
# Copyright (c) 2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

import base64

from .messages import (
    CTransaction,
    deser_string,
    from_binary,
    ser_compact_size,
)


class PSBTMap:
    """Class for serializing and deserializing PSBT maps"""

    def __init__(self, map=None):
        self.map = map if map is not None else {}

    def deserialize(self, f):
        m = {}
        while True:
            k = deser_string(f)
            if len(k) == 0:
                break
            v = deser_string(f)
            if len(k) == 1:
                k = k[0]
            assert k not in m
            m[k] = v
        self.map = m

    def serialize(self):
        m = b""
        for k,v in self.map.items():
            if isinstance(k, int) and 0 <= k and k <= 255:
                k = bytes([k])
            m += ser_compact_size(len(k)) + k
            m += ser_compact_size(len(v)) + v
        m += b"\x00"
        return m

class PSBT:
    """Class for serializing and deserializing PSBTs"""

    def __init__(self):
        self.g = PSBTMap()
        self.i = []
        self.o = []
        self.tx = None

    def deserialize(self, f):
        assert f.read(5) == b"psbt\xff"
        self.g = from_binary(PSBTMap, f)
        assert 0 in self.g.map
        self.tx = from_binary(CTransaction, self.g.map[0])
        self.i = [from_binary(PSBTMap, f) for _ in self.tx.vin]
        self.o = [from_binary(PSBTMap, f) for _ in self.tx.vout]
        return self

    def serialize(self):
        assert isinstance(self.g, PSBTMap)
        assert isinstance(self.i, list) and all(isinstance(x, PSBTMap) for x in self.i)
        assert isinstance(self.o, list) and all(isinstance(x, PSBTMap) for x in self.o)
        assert 0 in self.g.map
        tx = from_binary(CTransaction, self.g.map[0])
        assert len(tx.vin) == len(self.i)
        assert len(tx.vout) == len(self.o)

        psbt = [x.serialize() for x in [self.g] + self.i + self.o]
        return b"psbt\xff" + b"".join(psbt)

    def to_base64(self):
        return base64.b64encode(self.serialize()).decode("utf8")

    @classmethod
    def from_base64(cls, b64psbt):
        return from_binary(cls, base64.b64decode(b64psbt))
