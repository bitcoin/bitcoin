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


# global types
PSBT_GLOBAL_UNSIGNED_TX = 0x00
PSBT_GLOBAL_XPUB = 0x01
PSBT_GLOBAL_TX_VERSION = 0x02
PSBT_GLOBAL_FALLBACK_LOCKTIME = 0x03
PSBT_GLOBAL_INPUT_COUNT = 0x04
PSBT_GLOBAL_OUTPUT_COUNT = 0x05
PSBT_GLOBAL_TX_MODIFIABLE = 0x06
PSBT_GLOBAL_VERSION = 0xfb
PSBT_GLOBAL_PROPRIETARY = 0xfc

# per-input types
PSBT_IN_NON_WITNESS_UTXO = 0x00
PSBT_IN_WITNESS_UTXO = 0x01
PSBT_IN_PARTIAL_SIG = 0x02
PSBT_IN_SIGHASH_TYPE = 0x03
PSBT_IN_REDEEM_SCRIPT = 0x04
PSBT_IN_WITNESS_SCRIPT = 0x05
PSBT_IN_BIP32_DERIVATION = 0x06
PSBT_IN_FINAL_SCRIPTSIG = 0x07
PSBT_IN_FINAL_SCRIPTWITNESS = 0x08
PSBT_IN_POR_COMMITMENT = 0x09
PSBT_IN_RIPEMD160 = 0x0a
PSBT_IN_SHA256 = 0x0b
PSBT_IN_HASH160 = 0x0c
PSBT_IN_HASH256 = 0x0d
PSBT_IN_PREVIOUS_TXID = 0x0e
PSBT_IN_OUTPUT_INDEX = 0x0f
PSBT_IN_SEQUENCE = 0x10
PSBT_IN_REQUIRED_TIME_LOCKTIME = 0x11
PSBT_IN_REQUIRED_HEIGHT_LOCKTIME = 0x12
PSBT_IN_TAP_KEY_SIG = 0x13
PSBT_IN_TAP_SCRIPT_SIG = 0x14
PSBT_IN_TAP_LEAF_SCRIPT = 0x15
PSBT_IN_TAP_BIP32_DERIVATION = 0x16
PSBT_IN_TAP_INTERNAL_KEY = 0x17
PSBT_IN_TAP_MERKLE_ROOT = 0x18
PSBT_IN_PROPRIETARY = 0xfc

# per-output types
PSBT_OUT_REDEEM_SCRIPT = 0x00
PSBT_OUT_WITNESS_SCRIPT = 0x01
PSBT_OUT_BIP32_DERIVATION = 0x02
PSBT_OUT_AMOUNT = 0x03
PSBT_OUT_SCRIPT = 0x04
PSBT_OUT_TAP_INTERNAL_KEY = 0x05
PSBT_OUT_TAP_TREE = 0x06
PSBT_OUT_TAP_BIP32_DERIVATION = 0x07
PSBT_OUT_PROPRIETARY = 0xfc


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

    def __init__(self, *, g=None, i=None, o=None):
        self.g = g if g is not None else PSBTMap()
        self.i = i if i is not None else []
        self.o = o if o is not None else []
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

    def make_blank(self):
        """
        Remove all fields except for PSBT_GLOBAL_UNSIGNED_TX
        """
        for m in self.i + self.o:
            m.map.clear()

        self.g = PSBTMap(map={0: self.g.map[0]})

    def to_base64(self):
        return base64.b64encode(self.serialize()).decode("utf8")

    @classmethod
    def from_base64(cls, b64psbt):
        return from_binary(cls, base64.b64decode(b64psbt))
