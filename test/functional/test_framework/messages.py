#!/usr/bin/env python3
# Copyright (c) 2010 ArtForz -- public domain half-a-node
# Copyright (c) 2012 Jeff Garzik
# Copyright (c) 2010-2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Bitcoin test framework primitive and message structures
CBlock, CTransaction, CBlockHeader, CTxIn, CTxOut, etc....:
    data structures that should map to corresponding structures in
    bitcoin/primitives
msg_block, msg_tx, msg_headers, etc.:
    data structures that represent network messages

ser_*, deser_*: functions that handle serialization/deserialization.

Classes use __slots__ to ensure extraneous attributes aren't accidentally added
by tests, compromising their intended effect.
"""
from base64 import b32decode, b32encode
import copy
from collections import namedtuple
import hashlib
from io import BytesIO
import random
import socket
import struct
import time
import unittest

from test_framework.crypto.siphash import siphash256
from test_framework.util import assert_equal

import dash_hash

MAX_LOCATOR_SZ = 101
MAX_BLOCK_SIZE = 2000000
MAX_BLOOM_FILTER_SIZE = 36000
MAX_BLOOM_HASH_FUNCS = 50

COIN = 100000000  # 1 btc in satoshis
MAX_MONEY = 21000000 * COIN

BIP125_SEQUENCE_NUMBER = 0xfffffffd  # Sequence number that is BIP 125 opt-in and BIP 68-opt-out
SEQUENCE_FINAL = 0xffffffff  # Sequence number that disables nLockTime if set for every input of a tx

MAX_PROTOCOL_MESSAGE_LENGTH = 3 * 1024 * 1024  # Maximum length of incoming protocol messages
MAX_HEADERS_UNCOMPRESSED_RESULT = 2000  # Number of headers sent in one getheaders result
MAX_HEADERS_COMPRESSED_RESULT = 8000  # Number of headers2 sent in one getheaders2 result
MAX_INV_SIZE = 50000  # Maximum number of entries in an 'inv' protocol message

NODE_NETWORK = (1 << 0)
NODE_BLOOM = (1 << 2)
NODE_COMPACT_FILTERS = (1 << 6)
NODE_NETWORK_LIMITED = (1 << 10)
NODE_HEADERS_COMPRESSED = (1 << 11)
NODE_P2P_V2 = (1 << 12)

MSG_TX = 1
MSG_BLOCK = 2
MSG_FILTERED_BLOCK = 3
MSG_GOVERNANCE_OBJECT = 17
MSG_GOVERNANCE_OBJECT_VOTE = 18
MSG_CMPCT_BLOCK = 20
MSG_TYPE_MASK = 0xffffffff >> 2

FILTER_TYPE_BASIC = 0

DEFAULT_ANCESTOR_LIMIT = 25    # default max number of in-mempool ancestors
DEFAULT_DESCENDANT_LIMIT = 25  # default max number of in-mempool descendants

# Default setting for -datacarriersize. 80 bytes of data, +1 for OP_RETURN, +2 for the pushdata opcodes.
MAX_OP_RETURN_RELAY = 83

MAGIC_BYTES = {
    "mainnet": b"\xbf\x0c\x6b\xbd",   # mainnet
    "testnet3": b"\xce\xe2\xca\xff",  # testnet3
    "regtest": b"\xfc\xc1\xb7\xdc",   # regtest
    "devnet": b"\xe2\xca\xff\xce",    # devnet
}

def sha256(s):
    return hashlib.sha256(s).digest()


def sha3(s):
    return hashlib.sha3_256(s).digest()


def hash256(s):
    return sha256(sha256(s))

def dashhash(s):
    return dash_hash.getPoWHash(s)

def ser_compact_size(l):
    r = b""
    if l < 253:
        r = struct.pack("B", l)
    elif l < 0x10000:
        r = struct.pack("<BH", 253, l)
    elif l < 0x100000000:
        r = struct.pack("<BI", 254, l)
    else:
        r = struct.pack("<BQ", 255, l)
    return r


def deser_compact_size(f):
    nit = struct.unpack("<B", f.read(1))[0]
    if nit == 253:
        nit = struct.unpack("<H", f.read(2))[0]
    elif nit == 254:
        nit = struct.unpack("<I", f.read(4))[0]
    elif nit == 255:
        nit = struct.unpack("<Q", f.read(8))[0]
    return nit


def deser_string(f):
    nit = deser_compact_size(f)
    return f.read(nit)


def ser_string(s):
    return ser_compact_size(len(s)) + s


def deser_uint256(f):
    return int.from_bytes(f.read(32), 'little')


def ser_uint256(u):
    return u.to_bytes(32, 'little')


def uint256_from_str(s):
    return int.from_bytes(s[:32], 'little')


def uint256_to_string(uint256):
    return '%064x' % uint256


def uint256_from_compact(c):
    nbytes = (c >> 24) & 0xFF
    v = (c & 0xFFFFFF) << (8 * (nbytes - 3))
    return v


# deser_function_name: Allow for an alternate deserialization function on the
# entries in the vector.
def deser_vector(f, c, deser_function_name=None):
    nit = deser_compact_size(f)
    r = []
    for _ in range(nit):
        t = c()
        if deser_function_name:
            getattr(t, deser_function_name)(f)
        else:
            t.deserialize(f)
        r.append(t)
    return r


# ser_function_name: Allow for an alternate serialization function on the
# entries in the vector (we use this for serializing addrv2 messages).
def ser_vector(l, ser_function_name=None):
    r = ser_compact_size(len(l))
    for i in l:
        if ser_function_name:
            r += getattr(i, ser_function_name)()
        else:
            r += i.serialize()
    return r


def deser_uint256_vector(f):
    nit = deser_compact_size(f)
    r = []
    for _ in range(nit):
        t = deser_uint256(f)
        r.append(t)
    return r


def ser_uint256_vector(l):
    r = ser_compact_size(len(l))
    for i in l:
        r += ser_uint256(i)
    return r


def deser_dyn_bitset(f, bytes_based):
    if bytes_based:
        nb = deser_compact_size(f)
        n = nb * 8
    else:
        n = deser_compact_size(f)
        nb = int((n + 7) / 8)
    b = f.read(nb)
    r = []
    for i in range(n):
        r.append((b[int(i / 8)] & (1 << (i % 8))) != 0)
    return r


def ser_dyn_bitset(l, bytes_based):
    n = len(l)
    nb = int((n + 7) / 8)
    r = [0] * nb
    for i in range(n):
        r[int(i / 8)] |= (1 if l[i] else 0) << (i % 8)
    if bytes_based:
        r = ser_compact_size(nb) + bytes(r)
    else:
        r = ser_compact_size(n) + bytes(r)
    return r


def from_hex(obj, hex_string):
    """Deserialize from a hex string representation (e.g. from RPC)

    Note that there is no complementary helper like e.g. `to_hex` for the
    inverse operation. To serialize a message object to a hex string, simply
    use obj.serialize().hex()"""
    obj.deserialize(BytesIO(bytes.fromhex(hex_string)))
    return obj


def tx_from_hex(hex_string):
    """Deserialize from hex string to a transaction object"""
    return from_hex(CTransaction(), hex_string)


# Objects that map to dashd objects, which can be serialized/deserialized

class CService:
    __slots__ = ("ip", "port")

    def __init__(self):
        self.ip = ""
        self.port = 0

    def deserialize(self, f):
        self.ip = socket.inet_ntop(socket.AF_INET6, f.read(16))
        self.port = struct.unpack(">H", f.read(2))[0]

    def serialize(self):
        r = b""
        r += socket.inet_pton(socket.AF_INET6, self.ip)
        r += struct.pack(">H", self.port)
        return r

    def __repr__(self):
        return "CService(ip=%s port=%i)" % (self.ip, self.port)


class CAddress:
    __slots__ = ("net", "ip", "nServices", "port", "time")

    # see https://github.com/bitcoin/bips/blob/master/bip-0155.mediawiki
    NET_IPV4 = 1
    NET_IPV6 = 2
    NET_TORV3 = 4
    NET_I2P = 5
    NET_CJDNS = 6

    ADDRV2_NET_NAME = {
        NET_IPV4: "IPv4",
        NET_IPV6: "IPv6",
        NET_TORV3: "TorV3",
        NET_I2P: "I2P",
        NET_CJDNS: "CJDNS"
    }

    ADDRV2_ADDRESS_LENGTH = {
        NET_IPV4: 4,
        NET_IPV6: 16,
        NET_TORV3: 32,
        NET_I2P: 32,
        NET_CJDNS: 16
    }

    I2P_PAD = "===="

    def __init__(self):
        self.time = 0
        self.nServices = 1
        self.net = self.NET_IPV4
        self.ip = "0.0.0.0"
        self.port = 0

    def __eq__(self, other):
        return self.net == other.net and self.ip == other.ip and self.nServices == other.nServices and self.port == other.port and self.time == other.time

    def deserialize(self, f, *, with_time=True):
        """Deserialize from addrv1 format (pre-BIP155)"""
        if with_time:
            # VERSION messages serialize CAddress objects without time
            self.time = struct.unpack("<I", f.read(4))[0]
        self.nServices = struct.unpack("<Q", f.read(8))[0]
        # We only support IPv4 which means skip 12 bytes and read the next 4 as IPv4 address.
        f.read(12)
        self.net = self.NET_IPV4
        self.ip = socket.inet_ntoa(f.read(4))
        self.port = struct.unpack(">H", f.read(2))[0]

    def serialize(self, *, with_time=True):
        """Serialize in addrv1 format (pre-BIP155)"""
        assert self.net == self.NET_IPV4
        r = b""
        if with_time:
            # VERSION messages serialize CAddress objects without time
            r += struct.pack("<I", self.time)
        r += struct.pack("<Q", self.nServices)
        r += b"\x00" * 10 + b"\xff" * 2
        r += socket.inet_aton(self.ip)
        r += struct.pack(">H", self.port)
        return r

    def deserialize_v2(self, f):
        """Deserialize from addrv2 format (BIP155)"""
        self.time = struct.unpack("<I", f.read(4))[0]

        self.nServices = deser_compact_size(f)

        self.net = struct.unpack("B", f.read(1))[0]
        assert self.net in self.ADDRV2_NET_NAME

        address_length = deser_compact_size(f)
        assert address_length == self.ADDRV2_ADDRESS_LENGTH[self.net]

        addr_bytes = f.read(address_length)
        if self.net == self.NET_IPV4:
            self.ip = socket.inet_ntoa(addr_bytes)
        elif self.net == self.NET_IPV6:
            self.ip = socket.inet_ntop(socket.AF_INET6, addr_bytes)
        elif self.net == self.NET_TORV3:
            prefix = b".onion checksum"
            version = bytes([3])
            checksum = sha3(prefix + addr_bytes + version)[:2]
            self.ip = b32encode(addr_bytes + checksum + version).decode("ascii").lower() + ".onion"
        elif self.net == self.NET_I2P:
            self.ip = b32encode(addr_bytes)[0:-len(self.I2P_PAD)].decode("ascii").lower() + ".b32.i2p"
        elif self.net == self.NET_CJDNS:
            self.ip = socket.inet_ntop(socket.AF_INET6, addr_bytes)
        else:
            raise Exception(f"Address type not supported")

        self.port = struct.unpack(">H", f.read(2))[0]

    def serialize_v2(self):
        """Serialize in addrv2 format (BIP155)"""
        assert self.net in self.ADDRV2_NET_NAME
        r = b""
        r += struct.pack("<I", self.time)
        r += ser_compact_size(self.nServices)
        r += struct.pack("B", self.net)
        r += ser_compact_size(self.ADDRV2_ADDRESS_LENGTH[self.net])
        if self.net == self.NET_IPV4:
            r += socket.inet_aton(self.ip)
        elif self.net == self.NET_IPV6:
            r += socket.inet_pton(socket.AF_INET6, self.ip)
        elif self.net == self.NET_TORV3:
            sfx = ".onion"
            assert self.ip.endswith(sfx)
            r += b32decode(self.ip[0:-len(sfx)], True)[0:32]
        elif self.net == self.NET_I2P:
            sfx = ".b32.i2p"
            assert self.ip.endswith(sfx)
            r += b32decode(self.ip[0:-len(sfx)] + self.I2P_PAD, True)
        elif self.net == self.NET_CJDNS:
            r += socket.inet_pton(socket.AF_INET6, self.ip)
        else:
            raise Exception(f"Address type not supported")
        r += struct.pack(">H", self.port)
        return r

    def __repr__(self):
        return ("CAddress(nServices=%i net=%s addr=%s port=%i)"
                % (self.nServices, self.ADDRV2_NET_NAME[self.net], self.ip, self.port))


class CInv:
    __slots__ = ("hash", "type")

    typemap = {
        0: "Error",
        MSG_TX: "TX",
        MSG_BLOCK: "Block",
        MSG_FILTERED_BLOCK: "filtered Block",
        MSG_GOVERNANCE_OBJECT: "Governance Object",
        MSG_GOVERNANCE_OBJECT_VOTE: "Governance Vote",
        MSG_CMPCT_BLOCK: "CompactBlock",
    }

    def __init__(self, t=0, h=0):
        self.type = t
        self.hash = h

    def deserialize(self, f):
        self.type = struct.unpack("<I", f.read(4))[0]
        self.hash = deser_uint256(f)

    def serialize(self):
        r = b""
        r += struct.pack("<I", self.type)
        r += ser_uint256(self.hash)
        return r

    def __repr__(self):
        return "CInv(type=%s hash=%064x)" \
               % (self.typemap.get(self.type, "%d" % self.type), self.hash)

    def __eq__(self, other):
        return isinstance(other, CInv) and self.hash == other.hash and self.type == other.type


class CBlockLocator:
    __slots__ = ("nVersion", "vHave")

    def __init__(self):
        self.vHave = []

    def deserialize(self, f):
        struct.unpack("<i", f.read(4))[0]  # Ignore version field.
        self.vHave = deser_uint256_vector(f)

    def serialize(self):
        r = b""
        r += struct.pack("<i", 0)  # Bitcoin Core ignores version field. Set it to 0.
        r += ser_uint256_vector(self.vHave)
        return r

    def __repr__(self):
        return "CBlockLocator(vHave=%s)" % (repr(self.vHave))


class COutPoint:
    __slots__ = ("hash", "n")

    def __init__(self, hash=0, n=0xFFFFFFFF):
        self.hash = hash
        self.n = n

    def deserialize(self, f):
        self.hash = deser_uint256(f)
        self.n = struct.unpack("<I", f.read(4))[0]

    def serialize(self):
        r = b""
        r += ser_uint256(self.hash)
        r += struct.pack("<I", self.n)
        return r

    def __repr__(self):
        return "COutPoint(hash=%064x n=%i)" % (self.hash, self.n)


class CTxIn:
    __slots__ = ("nSequence", "prevout", "scriptSig")

    def __init__(self, outpoint=None, scriptSig=b"", nSequence=0):
        if outpoint is None:
            self.prevout = COutPoint()
        else:
            self.prevout = outpoint
        self.scriptSig = scriptSig
        self.nSequence = nSequence

    def deserialize(self, f):
        self.prevout = COutPoint()
        self.prevout.deserialize(f)
        self.scriptSig = deser_string(f)
        self.nSequence = struct.unpack("<I", f.read(4))[0]

    def serialize(self):
        r = b""
        r += self.prevout.serialize()
        r += ser_string(self.scriptSig)
        r += struct.pack("<I", self.nSequence)
        return r

    def __repr__(self):
        return "CTxIn(prevout=%s scriptSig=%s nSequence=%i)" \
               % (repr(self.prevout), self.scriptSig.hex(),
                  self.nSequence)


class CTxOut:
    __slots__ = ("nValue", "scriptPubKey")

    def __init__(self, nValue=0, scriptPubKey=b""):
        self.nValue = nValue
        self.scriptPubKey = scriptPubKey

    def deserialize(self, f):
        self.nValue = struct.unpack("<q", f.read(8))[0]
        self.scriptPubKey = deser_string(f)

    def serialize(self):
        r = b""
        r += struct.pack("<q", self.nValue)
        r += ser_string(self.scriptPubKey)
        return r

    def __repr__(self):
        return "CTxOut(nValue=%i.%08i scriptPubKey=%s)" \
               % (self.nValue // COIN, self.nValue % COIN,
                  self.scriptPubKey.hex())


class CTransaction:
    __slots__ = ("hash", "nLockTime", "nVersion", "sha256", "vin", "vout",
                 "nType", "vExtraPayload")

    def __init__(self, tx=None):
        if tx is None:
            self.nVersion = 2
            self.nType = 0
            self.vin = []
            self.vout = []
            self.nLockTime = 0
            self.vExtraPayload = None
            self.sha256 = None
            self.hash = None
        else:
            self.nVersion = tx.nVersion
            self.nType = tx.nType
            self.vin = copy.deepcopy(tx.vin)
            self.vout = copy.deepcopy(tx.vout)
            self.nLockTime = tx.nLockTime
            self.vExtraPayload = tx.vExtraPayload
            self.sha256 = tx.sha256
            self.hash = tx.hash

    def deserialize(self, f):
        ver32bit = struct.unpack("<i", f.read(4))[0]
        self.nVersion = ver32bit & 0xffff
        self.nType = (ver32bit >> 16) & 0xffff
        self.vin = deser_vector(f, CTxIn)
        self.vout = deser_vector(f, CTxOut)
        self.nLockTime = struct.unpack("<I", f.read(4))[0]
        if self.nType != 0:
            self.vExtraPayload = deser_string(f)
        self.sha256 = None
        self.hash = None

    def serialize(self):
        r = b""
        ver32bit = int(self.nVersion | (self.nType << 16))
        r += struct.pack("<i", ver32bit)
        r += ser_vector(self.vin)
        r += ser_vector(self.vout)
        r += struct.pack("<I", self.nLockTime)
        if self.nType != 0:
            r += ser_string(self.vExtraPayload)
        return r

    def rehash(self):
        self.sha256 = None
        self.calc_sha256()
        return self.hash

    def calc_sha256(self):
        if self.sha256 is None:
            self.sha256 = uint256_from_str(hash256(self.serialize()))
        self.hash = hash256(self.serialize())[::-1].hex()

    def is_valid(self):
        self.calc_sha256()
        for tout in self.vout:
            if tout.nValue < 0 or tout.nValue > 21000000 * COIN:
                return False
        return True

    # Calculate the virtual transaction size using
    # serialization size (does NOT use sigops).
    def get_vsize(self):
        return len(self.serialize())

    # it's just a helper that return vsize to reduce conflicts during backporting
    def get_weight(self):
        return self.get_vsize()

    def __repr__(self):
        return "CTransaction(nVersion=%i vin=%s vout=%s nLockTime=%i)" \
               % (self.nVersion, repr(self.vin), repr(self.vout), self.nLockTime)


class CBlockHeader:
    __slots__ = ("hash", "hashMerkleRoot", "hashPrevBlock", "nBits", "nNonce",
                 "nTime", "nVersion", "sha256")

    def __init__(self, header=None):
        if header is None:
            self.set_null()
        else:
            self.nVersion = header.nVersion
            self.hashPrevBlock = header.hashPrevBlock
            self.hashMerkleRoot = header.hashMerkleRoot
            self.nTime = header.nTime
            self.nBits = header.nBits
            self.nNonce = header.nNonce
            self.sha256 = header.sha256
            self.hash = header.hash
            self.calc_sha256()

    def set_null(self):
        self.nVersion = 4
        self.hashPrevBlock = 0
        self.hashMerkleRoot = 0
        self.nTime = 0
        self.nBits = 0
        self.nNonce = 0
        self.sha256 = None
        self.hash = None

    def deserialize(self, f):
        self.nVersion = struct.unpack("<i", f.read(4))[0]
        self.hashPrevBlock = deser_uint256(f)
        self.hashMerkleRoot = deser_uint256(f)
        self.nTime = struct.unpack("<I", f.read(4))[0]
        self.nBits = struct.unpack("<I", f.read(4))[0]
        self.nNonce = struct.unpack("<I", f.read(4))[0]
        self.sha256 = None
        self.hash = None

    def serialize(self):
        r = b""
        r += struct.pack("<i", self.nVersion)
        r += ser_uint256(self.hashPrevBlock)
        r += ser_uint256(self.hashMerkleRoot)
        r += struct.pack("<I", self.nTime)
        r += struct.pack("<I", self.nBits)
        r += struct.pack("<I", self.nNonce)
        return r

    def calc_sha256(self):
        if self.sha256 is None:
            r = b""
            r += struct.pack("<i", self.nVersion)
            r += ser_uint256(self.hashPrevBlock)
            r += ser_uint256(self.hashMerkleRoot)
            r += struct.pack("<I", self.nTime)
            r += struct.pack("<I", self.nBits)
            r += struct.pack("<I", self.nNonce)
            self.sha256 = uint256_from_str(dashhash(r))
            self.hash = dashhash(r)[::-1].hex()

    def rehash(self):
        self.sha256 = None
        self.calc_sha256()
        return self.sha256

    def __repr__(self):
        return "CBlockHeader(nVersion=%i hashPrevBlock=%064x hashMerkleRoot=%064x nTime=%s nBits=%08x nNonce=%08x)" \
               % (self.nVersion, self.hashPrevBlock, self.hashMerkleRoot,
                  time.ctime(self.nTime), self.nBits, self.nNonce)

BLOCK_HEADER_SIZE = len(CBlockHeader().serialize())
assert_equal(BLOCK_HEADER_SIZE, 80)

class CBlock(CBlockHeader):
    __slots__ = ("vtx",)

    def __init__(self, header=None):
        super().__init__(header)
        self.vtx = []

    def deserialize(self, f):
        super().deserialize(f)
        self.vtx = deser_vector(f, CTransaction)

    def serialize(self):
        r = b""
        r += super().serialize()
        r += ser_vector(self.vtx)
        return r

    # Calculate the merkle root given a vector of transaction hashes
    @staticmethod
    def get_merkle_root(hashes):
        if len(hashes) == 0:
            return 0
        while len(hashes) > 1:
            newhashes = []
            for i in range(0, len(hashes), 2):
                i2 = min(i+1, len(hashes)-1)
                newhashes.append(hash256(hashes[i] + hashes[i2]))
            hashes = newhashes
        return uint256_from_str(hashes[0])

    def calc_merkle_root(self):
        hashes = []
        for tx in self.vtx:
            tx.calc_sha256()
            hashes.append(ser_uint256(tx.sha256))
        return self.get_merkle_root(hashes)

    def is_valid(self):
        self.calc_sha256()
        target = uint256_from_compact(self.nBits)
        if self.sha256 > target:
            return False
        for tx in self.vtx:
            if not tx.is_valid():
                return False
        if self.calc_merkle_root() != self.hashMerkleRoot:
            return False
        return True

    def solve(self):
        self.rehash()
        target = uint256_from_compact(self.nBits)
        while self.sha256 > target:
            self.nNonce += 1
            self.rehash()

    # it's just a helper that return vsize to reduce conflicts during backporting
    def get_weight(self):
        return len(self.serialize())

    def __repr__(self):
        return "CBlock(nVersion=%i hashPrevBlock=%064x hashMerkleRoot=%064x nTime=%s nBits=%08x nNonce=%08x vtx=%s)" \
               % (self.nVersion, self.hashPrevBlock, self.hashMerkleRoot,
                  time.ctime(self.nTime), self.nBits, self.nNonce, repr(self.vtx))


class CompressibleBlockHeader:
    __slots__ = ("bitfield", "timeOffset", "nVersion", "hashPrevBlock", "hashMerkleRoot", "nTime", "nBits", "nNonce",
                 "hash", "sha256")

    FLAG_VERSION_BIT_0 = 1 << 0
    FLAG_VERSION_BIT_1 = 1 << 1
    FLAG_VERSION_BIT_2 = 1 << 2
    FLAG_PREV_BLOCK_HASH = 1 << 3
    FLAG_TIMESTAMP = 1 << 4
    FLAG_NBITS = 1 << 5

    BITMASK_VERSION = FLAG_VERSION_BIT_0 | FLAG_VERSION_BIT_1 | FLAG_VERSION_BIT_2

    def __init__(self, header=None):
        if header is None:
            self.set_null()
        else:
            self.bitfield = 0
            self.timeOffset = 0
            self.nVersion = header.nVersion
            self.hashPrevBlock = header.hashPrevBlock
            self.hashMerkleRoot = header.hashMerkleRoot
            self.nTime = header.nTime
            self.nBits = header.nBits
            self.nNonce = header.nNonce
            self.hash = None
            self.sha256 = None
            self.calc_sha256()

    def set_null(self):
        self.bitfield = 0
        self.timeOffset = 0
        self.nVersion = 0
        self.hashPrevBlock = 0
        self.hashMerkleRoot = 0
        self.nTime = 0
        self.nBits = 0
        self.nNonce = 0
        self.hash = None
        self.sha256 = None

    def deserialize(self, f):
        self.bitfield = struct.unpack("<B", f.read(1))[0]
        if self.bitfield & self.BITMASK_VERSION == 0:
            self.nVersion = struct.unpack("<i", f.read(4))[0]
        if self.bitfield & self.FLAG_PREV_BLOCK_HASH:
            self.hashPrevBlock = deser_uint256(f)
        self.hashMerkleRoot = deser_uint256(f)
        if self.bitfield & self.FLAG_TIMESTAMP:
            self.nTime = struct.unpack("<I", f.read(4))[0]
        else:
            self.timeOffset = struct.unpack("<h", f.read(2))[0]
        if self.bitfield & self.FLAG_NBITS:
            self.nBits = struct.unpack("<I", f.read(4))[0]
        self.nNonce = struct.unpack("<I", f.read(4))[0]
        self.rehash()

    def serialize(self):
        r = b""
        r += struct.pack("<B", self.bitfield)
        if not self.bitfield & self.BITMASK_VERSION:
            r += struct.pack("<i", self.nVersion)
        if self.bitfield & self.FLAG_PREV_BLOCK_HASH:
            r += ser_uint256(self.hashPrevBlock)
        r += ser_uint256(self.hashMerkleRoot)
        r += struct.pack("<I", self.nTime) if self.bitfield & self.FLAG_TIMESTAMP else struct.pack("<h", self.timeOffset)
        if self.bitfield & self.FLAG_NBITS:
            r += struct.pack("<I", self.nBits)
        r += struct.pack("<I", self.nNonce)
        return r

    def calc_sha256(self):
        if self.sha256 is None:
            r = b""
            r += struct.pack("<i", self.nVersion)
            r += ser_uint256(self.hashPrevBlock)
            r += ser_uint256(self.hashMerkleRoot)
            r += struct.pack("<I", self.nTime)
            r += struct.pack("<I", self.nBits)
            r += struct.pack("<I", self.nNonce)
            self.sha256 = uint256_from_str(dashhash(r))
            self.hash = int(dashhash(r)[::-1].hex(), 16)

    def rehash(self):
        self.sha256 = None
        self.calc_sha256()
        return self.sha256

    def __repr__(self):
        return "BlockHeaderCompressed(bitfield=%064x, nVersion=%i hashPrevBlock=%064x hashMerkleRoot=%064x nTime=%s " \
               "nBits=%08x nNonce=%08x timeOffset=%i)" % \
               (self.bitfield, self.nVersion, self.hashPrevBlock, self.hashMerkleRoot, time.ctime(self.nTime), self.nBits, self.nNonce, self.timeOffset)

    def __save_version_as_most_recent(self, last_unique_versions):
        last_unique_versions.insert(0, self.nVersion)

        # Evict the oldest version
        if len(last_unique_versions) > 7:
            last_unique_versions.pop()

    @staticmethod
    def __mark_version_as_most_recent(last_unique_versions, version_idx):
        # Move version to the front of the list
        last_unique_versions.insert(0, last_unique_versions.pop(version_idx))

    def compress(self, last_blocks, last_unique_versions):
        if not last_blocks:
            # First block, everything must be uncompressed
            self.bitfield &= (~CompressibleBlockHeader.BITMASK_VERSION)
            self.bitfield |= CompressibleBlockHeader.FLAG_PREV_BLOCK_HASH
            self.bitfield |= CompressibleBlockHeader.FLAG_TIMESTAMP
            self.bitfield |= CompressibleBlockHeader.FLAG_NBITS
            self.__save_version_as_most_recent(last_unique_versions)
            return

        # Compress version
        try:
            version_idx = last_unique_versions.index(self.nVersion)
            version_offset = len(last_unique_versions) - version_idx
            self.bitfield &= (~CompressibleBlockHeader.BITMASK_VERSION)
            self.bitfield |= (version_offset & CompressibleBlockHeader.BITMASK_VERSION)
            self.__mark_version_as_most_recent(last_unique_versions, version_idx)
        except ValueError:
            self.__save_version_as_most_recent(last_unique_versions)

        # We have the previous block
        last_block = last_blocks[-1]

        # Compress time
        self.timeOffset = self.nTime - last_block.nTime
        if self.timeOffset > 32767 or self.timeOffset < -32768:
            # Time diff overflows, we have to send it as 4 bytes (uncompressed)
            self.bitfield |= CompressibleBlockHeader.FLAG_TIMESTAMP

        # If nBits doesn't match previous block, we have to send it
        if self.nBits != last_block.nBits:
            self.bitfield |= CompressibleBlockHeader.FLAG_NBITS

    def uncompress(self, last_compressed_blocks, last_unique_versions):
        if not last_compressed_blocks:
            # First block header is always uncompressed
            self.__save_version_as_most_recent(last_unique_versions)
            return

        previous_block = last_compressed_blocks[-1]

        # Uncompress version
        version_idx = self.bitfield & self.BITMASK_VERSION
        if version_idx != 0:
            if version_idx <= len(last_unique_versions):
                self.nVersion = last_unique_versions[version_idx - 1]
                self.__mark_version_as_most_recent(last_unique_versions, version_idx - 1)
        else:
            self.__save_version_as_most_recent(last_unique_versions)

        # Uncompress prev block hash
        if not self.bitfield & self.FLAG_PREV_BLOCK_HASH:
            self.hashPrevBlock = previous_block.hash

        # Uncompress time
        if not self.bitfield & self.FLAG_TIMESTAMP:
            self.nTime = previous_block.nTime + self.timeOffset

        # Uncompress time bits
        if not self.bitfield & self.FLAG_NBITS:
            self.nBits = previous_block.nBits

        self.rehash()


class PrefilledTransaction:
    __slots__ = ("index", "tx")

    def __init__(self, index=0, tx = None):
        self.index = index
        self.tx = tx

    def deserialize(self, f):
        self.index = deser_compact_size(f)
        self.tx = CTransaction()
        self.tx.deserialize(f)

    def serialize(self):
        r = b""
        r += ser_compact_size(self.index)
        r += self.tx.serialize()
        return r

    def __repr__(self):
        return "PrefilledTransaction(index=%d, tx=%s)" % (self.index, repr(self.tx))


# This is what we send on the wire, in a cmpctblock message.
class P2PHeaderAndShortIDs:
    __slots__ = ("header", "nonce", "prefilled_txn", "prefilled_txn_length",
                 "shortids", "shortids_length")

    def __init__(self):
        self.header = CBlockHeader()
        self.nonce = 0
        self.shortids_length = 0
        self.shortids = []
        self.prefilled_txn_length = 0
        self.prefilled_txn = []

    def deserialize(self, f):
        self.header.deserialize(f)
        self.nonce = struct.unpack("<Q", f.read(8))[0]
        self.shortids_length = deser_compact_size(f)
        for _ in range(self.shortids_length):
            # shortids are defined to be 6 bytes in the spec, so append
            # two zero bytes and read it in as an 8-byte number
            self.shortids.append(struct.unpack("<Q", f.read(6) + b'\x00\x00')[0])
        self.prefilled_txn = deser_vector(f, PrefilledTransaction)
        self.prefilled_txn_length = len(self.prefilled_txn)

    def serialize(self):
        r = b""
        r += self.header.serialize()
        r += struct.pack("<Q", self.nonce)
        r += ser_compact_size(self.shortids_length)
        for x in self.shortids:
            # We only want the first 6 bytes
            r += struct.pack("<Q", x)[0:6]
        r += ser_vector(self.prefilled_txn)
        return r

    def __repr__(self):
        return "P2PHeaderAndShortIDs(header=%s, nonce=%d, shortids_length=%d, shortids=%s, prefilled_txn_length=%d, prefilledtxn=%s" % (repr(self.header), self.nonce, self.shortids_length, repr(self.shortids), self.prefilled_txn_length, repr(self.prefilled_txn))


# Calculate the BIP 152-compact blocks shortid for a given transaction hash
def calculate_shortid(k0, k1, tx_hash):
    expected_shortid = siphash256(k0, k1, tx_hash)
    expected_shortid &= 0x0000ffffffffffff
    return expected_shortid


# This version gets rid of the array lengths, and reinterprets the differential
# encoding into indices that can be used for lookup.
class HeaderAndShortIDs:
    __slots__ = ("header", "nonce", "prefilled_txn", "shortids")

    def __init__(self, p2pheaders_and_shortids = None):
        self.header = CBlockHeader()
        self.nonce = 0
        self.shortids = []
        self.prefilled_txn = []

        if p2pheaders_and_shortids is not None:
            self.header = p2pheaders_and_shortids.header
            self.nonce = p2pheaders_and_shortids.nonce
            self.shortids = p2pheaders_and_shortids.shortids
            last_index = -1
            for x in p2pheaders_and_shortids.prefilled_txn:
                self.prefilled_txn.append(PrefilledTransaction(x.index + last_index + 1, x.tx))
                last_index = self.prefilled_txn[-1].index

    def to_p2p(self):
        ret = P2PHeaderAndShortIDs()
        ret.header = self.header
        ret.nonce = self.nonce
        ret.shortids_length = len(self.shortids)
        ret.shortids = self.shortids
        ret.prefilled_txn_length = len(self.prefilled_txn)
        ret.prefilled_txn = []
        last_index = -1
        for x in self.prefilled_txn:
            ret.prefilled_txn.append(PrefilledTransaction(x.index - last_index - 1, x.tx))
            last_index = x.index
        return ret

    def get_siphash_keys(self):
        header_nonce = self.header.serialize()
        header_nonce += struct.pack("<Q", self.nonce)
        hash_header_nonce_as_str = sha256(header_nonce)
        key0 = struct.unpack("<Q", hash_header_nonce_as_str[0:8])[0]
        key1 = struct.unpack("<Q", hash_header_nonce_as_str[8:16])[0]
        return [ key0, key1 ]

    def initialize_from_block(self, block, nonce=0, prefill_list=None):
        if prefill_list is None:
            prefill_list = [0]
        self.header = CBlockHeader(block)
        self.nonce = nonce
        self.prefilled_txn = [ PrefilledTransaction(i, block.vtx[i]) for i in prefill_list ]
        self.shortids = []
        [k0, k1] = self.get_siphash_keys()
        for i in range(len(block.vtx)):
            if i not in prefill_list:
                self.shortids.append(calculate_shortid(k0, k1, block.vtx[i].sha256))

    def __repr__(self):
        return "HeaderAndShortIDs(header=%s, nonce=%d, shortids=%s, prefilledtxn=%s" % (repr(self.header), self.nonce, repr(self.shortids), repr(self.prefilled_txn))


class BlockTransactionsRequest:
    __slots__ = ("blockhash", "indexes")

    def __init__(self, blockhash=0, indexes = None):
        self.blockhash = blockhash
        self.indexes = indexes if indexes is not None else []

    def deserialize(self, f):
        self.blockhash = deser_uint256(f)
        indexes_length = deser_compact_size(f)
        for _ in range(indexes_length):
            self.indexes.append(deser_compact_size(f))

    def serialize(self):
        r = b""
        r += ser_uint256(self.blockhash)
        r += ser_compact_size(len(self.indexes))
        for x in self.indexes:
            r += ser_compact_size(x)
        return r

    # helper to set the differentially encoded indexes from absolute ones
    def from_absolute(self, absolute_indexes):
        self.indexes = []
        last_index = -1
        for x in absolute_indexes:
            self.indexes.append(x-last_index-1)
            last_index = x

    def to_absolute(self):
        absolute_indexes = []
        last_index = -1
        for x in self.indexes:
            absolute_indexes.append(x+last_index+1)
            last_index = absolute_indexes[-1]
        return absolute_indexes

    def __repr__(self):
        return "BlockTransactionsRequest(hash=%064x indexes=%s)" % (self.blockhash, repr(self.indexes))


class BlockTransactions:
    __slots__ = ("blockhash", "transactions")

    def __init__(self, blockhash=0, transactions = None):
        self.blockhash = blockhash
        self.transactions = transactions if transactions is not None else []

    def deserialize(self, f):
        self.blockhash = deser_uint256(f)
        self.transactions = deser_vector(f, CTransaction)

    def serialize(self):
        r = b""
        r += ser_uint256(self.blockhash)
        r += ser_vector(self.transactions)
        return r

    def __repr__(self):
        return "BlockTransactions(hash=%064x transactions=%s)" % (self.blockhash, repr(self.transactions))


class CPartialMerkleTree:
    __slots__ = ("nTransactions", "vBits", "vHash")

    def __init__(self):
        self.nTransactions = 0
        self.vBits = []
        self.vHash = []

    def deserialize(self, f):
        self.nTransactions = struct.unpack("<I", f.read(4))[0]
        self.vHash = deser_uint256_vector(f)
        self.vBits = deser_dyn_bitset(f, True)

    def serialize(self):
        r = b""
        r += struct.pack("<I", self.nTransactions)
        r += ser_uint256_vector(self.vHash)
        r += ser_dyn_bitset(self.vBits, True)
        return r

    def __repr__(self):
        return "CPartialMerkleTree(nTransactions=%d vBits.size=%d vHash.size=%d)" % (self.nTransactions, len(self.vBits), len(self.vHash))


class CMerkleBlock:
    __slots__ = ("header", "txn")

    def __init__(self, header=None, txn=None):
        if header is None:
            self.header = CBlockHeader()
        else:
            self.header = header
        if txn is None:
            self.txn = CPartialMerkleTree()
        else:
            self.txn = txn

    def deserialize(self, f):
        self.header.deserialize(f)
        self.txn.deserialize(f)

    def serialize(self):
        r = b""
        r += self.header.serialize()
        r += self.txn.serialize()
        return r

    def __repr__(self):
        return "CMerkleBlock(header=%s txn=%s)" % (repr(self.header), repr(self.txn))


class CCbTx:
    __slots__ = ("nVersion", "nHeight", "merkleRootMNList", "merkleRootQuorums", "bestCLHeightDiff", "bestCLSignature", "assetLockedAmount")

    def __init__(self, version=None, height=None, merkleRootMNList=None, merkleRootQuorums=None, bestCLHeightDiff=None, bestCLSignature=None, assetLockedAmount=None):
        self.set_null()
        if version is not None:
            self.nVersion = version
        if height is not None:
            self.nHeight = height
        if merkleRootMNList is not None:
            self.merkleRootMNList = merkleRootMNList
        if merkleRootQuorums is not None:
            self.merkleRootQuorums = merkleRootQuorums
        if bestCLHeightDiff is not None:
            self.bestCLHeightDiff = bestCLHeightDiff
        if bestCLSignature is not None:
            self.bestCLSignature = bestCLSignature
        if assetLockedAmount is not None:
            self.assetLockedAmount = assetLockedAmount

    def set_null(self):
        self.nVersion = 0
        self.nHeight = 0
        self.merkleRootMNList = None
        self.merkleRootQuorums = None
        self.bestCLHeightDiff = 0
        self.bestCLSignature = b'\x00' * 96
        self.assetLockedAmount = 0

    def deserialize(self, f):
        self.nVersion = struct.unpack("<H", f.read(2))[0]
        self.nHeight = struct.unpack("<i", f.read(4))[0]
        self.merkleRootMNList = deser_uint256(f)
        if self.nVersion >= 2:
            self.merkleRootQuorums = deser_uint256(f)
            if self.nVersion >= 3:
                self.bestCLHeightDiff = deser_compact_size(f)
                self.bestCLSignature = f.read(96)
                self.assetLockedAmount = struct.unpack("<q", f.read(8))[0]

    def serialize(self):
        r = b""
        r += struct.pack("<H", self.nVersion)
        r += struct.pack("<i", self.nHeight)
        r += ser_uint256(self.merkleRootMNList)
        if self.nVersion >= 2:
            r += ser_uint256(self.merkleRootQuorums)
            if self.nVersion >= 3:
                r += ser_compact_size(self.bestCLHeightDiff)
                r += self.bestCLSignature
                r += struct.pack("<q", self.assetLockedAmount)
        return r

    def __repr__(self):
        return "CCbTx(nVersion=%i nHeight=%i merkleRootMNList=%s merkleRootQuorums=%s bestCLHeightDiff=%i bestCLSignature=%s assetLockedAmount=%i)" \
               % (self.nVersion, self.nHeight, self.merkleRootMNList.hex(), self.merkleRootQuorums.hex(), self.bestCLHeightDiff, self.bestCLSignature.hex(), self.assetLockedAmount)


class CAssetLockTx:
    __slots__ = ("version", "creditOutputs")

    def __init__(self, version=None, creditOutputs=None):
        self.set_null()
        if version is not None:
            self.version = version
        self.creditOutputs = creditOutputs if creditOutputs is not None else []

    def set_null(self):
        self.version = 0
        self.creditOutputs = None

    def deserialize(self, f):
        self.version = struct.unpack("<B", f.read(1))[0]
        self.creditOutputs = deser_vector(f, CTxOut)

    def serialize(self):
        r = b""
        r += struct.pack("<B", self.version)
        r += ser_vector(self.creditOutputs)
        return r

    def __repr__(self):
        return "CAssetLockTx(version={} creditOutputs={}" \
            .format(self.version, repr(self.creditOutputs))


class CAssetUnlockTx:
    __slots__ = ("version", "index", "fee", "requestedHeight", "quorumHash", "quorumSig")

    def __init__(self, version=None, index=None, fee=None, requestedHeight=None, quorumHash = 0, quorumSig = None):
        self.set_null()
        if version is not None:
            self.version = version
        if index is not None:
            self.index = index
        if fee is not None:
            self.fee = fee
        if requestedHeight is not None:
            self.requestedHeight = requestedHeight
        if quorumHash is not None:
            self.quorumHash = quorumHash
        if quorumSig is not None:
            self.quorumSig = quorumSig

    def set_null(self):
        self.version = 0
        self.index = 0
        self.fee = None
        self.requestedHeight = 0
        self.quorumHash = 0
        self.quorumSig = b'\x00' * 96

    def deserialize(self, f):
        self.version = struct.unpack("<B", f.read(1))[0]
        self.index = struct.unpack("<Q", f.read(8))[0]
        self.fee = struct.unpack("<I", f.read(4))[0]
        self.requestedHeight = struct.unpack("<I", f.read(4))[0]
        self.quorumHash = deser_uint256(f)
        self.quorumSig = f.read(96)

    def serialize(self):
        r = b""
        r += struct.pack("<B", self.version)
        r += struct.pack("<Q", self.index)
        r += struct.pack("<I", self.fee)
        r += struct.pack("<I", self.requestedHeight)
        r += ser_uint256(self.quorumHash)
        r += self.quorumSig
        return r

    def __repr__(self):
        return "CAssetUnlockTx(version={} index={} fee={} requestedHeight={} quorumHash={:x} quorumSig={}" \
            .format(self.version, self.index, self.fee, self.requestedHeight, self.quorumHash, self.quorumSig.hex())


class CMnEhf:
    __slots__ = ("version", "versionBit", "quorumHash", "quorumSig")

    def __init__(self, version=None, versionBit=None, quorumHash = 0, quorumSig = None):
        self.set_null()
        if version is not None:
            self.version = version
        if versionBit is not None:
            self.versionBit = versionBit
        if quorumHash is not None:
            self.quorumHash = quorumHash
        if quorumSig is not None:
            self.quorumSig = quorumSig

    def set_null(self):
        self.version = 0
        self.versionBit = 0
        self.quorumHash = 0
        self.quorumSig = b'\x00' * 96

    def deserialize(self, f):
        self.version = struct.unpack("<B", f.read(1))[0]
        self.versionBit = struct.unpack("<B", f.read(1))[0]
        self.quorumHash = deser_uint256(f)
        self.quorumSig = f.read(96)

    def serialize(self):
        r = b""
        r += struct.pack("<B", self.version)
        r += struct.pack("<B", self.versionBit)
        r += ser_uint256(self.quorumHash)
        r += self.quorumSig
        return r

    def __repr__(self):
        return "CMnEhf(version={} versionBit={} quorumHash={:x} quorumSig={}" \
            .format(self.version, self.versionBit, self.quorumHash, self.quorumSig.hex())


class CSimplifiedMNListEntry:
    __slots__ = ("proRegTxHash", "confirmedHash", "service", "pubKeyOperator", "keyIDVoting", "isValid", "nVersion", "type", "platformHTTPPort", "platformNodeID")

    def __init__(self):
        self.set_null()

    def set_null(self):
        self.proRegTxHash = 0
        self.confirmedHash = 0
        self.service = CService()
        self.pubKeyOperator = b'\x00' * 48
        self.keyIDVoting = 0
        self.isValid = False
        self.nVersion = 0
        self.type = 0
        self.platformHTTPPort = 0
        self.platformNodeID = b'\x00' * 20

    def deserialize(self, f):
        self.nVersion = struct.unpack("<H", f.read(2))[0]
        self.proRegTxHash = deser_uint256(f)
        self.confirmedHash = deser_uint256(f)
        self.service.deserialize(f)
        self.pubKeyOperator = f.read(48)
        self.keyIDVoting = f.read(20)
        self.isValid = struct.unpack("<?", f.read(1))[0]
        if self.nVersion == 2:
            self.type = struct.unpack("<H", f.read(2))[0]
            if self.type == 1:
                self.platformHTTPPort = struct.unpack("<H", f.read(2))[0]
                self.platformNodeID = f.read(20)

    def serialize(self, with_version = True):
        r = b""
        if with_version:
            r += struct.pack("<H", self.nVersion)
        r += ser_uint256(self.proRegTxHash)
        r += ser_uint256(self.confirmedHash)
        r += self.service.serialize()
        r += self.pubKeyOperator
        r += self.keyIDVoting
        r += struct.pack("<?", self.isValid)
        if self.nVersion == 2:
            r += struct.pack("<H", self.type)
            if self.type == 1:
                r += struct.pack("<H", self.platformHTTPPort)
                r += self.platformNodeID
        return r


class CFinalCommitment:
    __slots__ = ("nVersion", "llmqType", "quorumHash", "quorumIndex", "signers", "validMembers", "quorumPublicKey",
                 "quorumVvecHash", "quorumSig", "membersSig")

    def __init__(self):
        self.set_null()

    def set_null(self):
        self.nVersion = 0
        self.llmqType = 0
        self.quorumHash = 0
        self.quorumIndex = 0
        self.signers = []
        self.validMembers = []
        self.quorumPublicKey = b'\x00' * 48
        self.quorumVvecHash = 0
        self.quorumSig = b'\x00' * 96
        self.membersSig = b'\x00' * 96

    def deserialize(self, f):
        self.nVersion = struct.unpack("<H", f.read(2))[0]
        self.llmqType = struct.unpack("<B", f.read(1))[0]
        self.quorumHash = deser_uint256(f)
        if self.nVersion == 2 or self.nVersion == 4:
            self.quorumIndex = struct.unpack("<H", f.read(2))[0]
        self.signers = deser_dyn_bitset(f, False)
        self.validMembers = deser_dyn_bitset(f, False)
        self.quorumPublicKey = f.read(48)
        self.quorumVvecHash = deser_uint256(f)
        self.quorumSig = f.read(96)
        self.membersSig = f.read(96)

    def serialize(self):
        r = b""
        r += struct.pack("<H", self.nVersion)
        r += struct.pack("<B", self.llmqType)
        r += ser_uint256(self.quorumHash)
        if self.nVersion == 2 or self.nVersion == 4:
            r += struct.pack("<H", self.quorumIndex)
        r += ser_dyn_bitset(self.signers, False)
        r += ser_dyn_bitset(self.validMembers, False)
        r += self.quorumPublicKey
        r += ser_uint256(self.quorumVvecHash)
        r += self.quorumSig
        r += self.membersSig
        return r

    def __repr__(self):
        return "CFinalCommitment(nVersion={} llmqType={} quorumHash={:x} quorumIndex={} signers={}" \
               " validMembers={} quorumPublicKey={} quorumVvecHash={:x}) quorumSig={} membersSig={})" \
            .format(self.nVersion, self.llmqType, self.quorumHash, self.quorumIndex, repr(self.signers),
                    repr(self.validMembers), self.quorumPublicKey.hex(), self.quorumVvecHash, self.quorumSig.hex(), self.membersSig.hex())


class CFinalCommitmentPayload:
    __slots__ = ("nVersion", "nHeight", "commitment")

    def __init__(self):
        self.set_null()

    def set_null(self):
        self.nVersion = 0
        self.nHeight = 0
        self.commitment = CFinalCommitment()

    def deserialize(self, f):
        self.nVersion = struct.unpack("<H", f.read(2))[0]
        self.nHeight = struct.unpack("<I", f.read(4))[0]
        self.commitment = CFinalCommitment()
        self.commitment.deserialize(f)

    def serialize(self):
        r = b""
        r += struct.pack("<H", self.nVersion)
        r += struct.pack("<I", self.nHeight)
        r += self.commitment.serialize()
        return r

    def __repr__(self):
        return f"CFinalCommitmentPayload(nVersion={self.nVersion} nHeight={self.nHeight} commitment={self.commitment})"

class CGovernanceObject:
    __slots__ = ("nHashParent", "nRevision", "nTime", "nCollateralHash", "vchData", "nObjectType",
                 "masternodeOutpoint", "vchSig")

    def __init__(self):
        self.nHashParent = 0
        self.nRevision = 0
        self.nTime = 0
        self.nCollateralHash = 0
        self.vchData = []
        self.nObjectType = 0
        self.masternodeOutpoint = COutPoint()
        self.vchSig = []

    def deserialize(self, f):
        self.nHashParent = deser_uint256(f)
        self.nRevision = struct.unpack("<i", f.read(4))[0]
        self.nTime = struct.unpack("<q", f.read(8))[0]
        self.nCollateralHash = deser_uint256(f)
        size = deser_compact_size(f)
        if size > 0:
            self.vchData = f.read(size)
        self.nObjectType = struct.unpack("<i", f.read(4))[0]
        self.masternodeOutpoint.deserialize(f)
        size = deser_compact_size(f)
        if size > 0:
            self.vchSig = f.read(size)

    def serialize(self):
        r = b""
        r += ser_uint256(self.nParentHash)
        r += struct.pack("<i", self.nRevision)
        r += struct.pack("<q", self.nTime)
        r += deser_uint256(self.nCollateralHash)
        r += deser_compact_size(len(self.vchData))
        r += self.vchData
        r += struct.pack("<i", self.nObjectType)
        r += self.masternodeOutpoint.serialize()
        r += deser_compact_size(len(self.vchSig))
        r += self.vchSig
        return r


class CGovernanceVote:
    __slots__ = ("masternodeOutpoint", "nParentHash", "nVoteOutcome", "nVoteSignal", "nTime", "vchSig")

    def __init__(self):
        self.masternodeOutpoint = COutPoint()
        self.nParentHash = 0
        self.nVoteOutcome = 0
        self.nVoteSignal = 0
        self.nTime = 0
        self.vchSig = []

    def deserialize(self, f):
        self.masternodeOutpoint.deserialize(f)
        self.nParentHash = deser_uint256(f)
        self.nVoteOutcome = struct.unpack("<i", f.read(4))[0]
        self.nVoteSignal = struct.unpack("<i", f.read(4))[0]
        self.nTime = struct.unpack("<q", f.read(8))[0]
        size = deser_compact_size(f)
        if size > 0:
            self.vchSig = f.read(size)

    def serialize(self):
        r = b""
        r += self.masternodeOutpoint.serialize()
        r += ser_uint256(self.nParentHash)
        r += struct.pack("<i", self.nVoteOutcome)
        r += struct.pack("<i", self.nVoteSignal)
        r += struct.pack("<q", self.nTime)
        r += ser_compact_size(len(self.vchSig))
        r += self.vchSig
        return r


class CRecoveredSig:
    __slots__ = ("llmqType", "quorumHash", "id", "msgHash", "sig")

    def __init__(self):
        self.llmqType = 0
        self.quorumHash = 0
        self.id = 0
        self.msgHash = 0
        self.sig = b'\x00' * 96

    def deserialize(self, f):
        self.llmqType = struct.unpack("<B", f.read(1))[0]
        self.quorumHash = deser_uint256(f)
        self.id = deser_uint256(f)
        self.msgHash = deser_uint256(f)
        self.sig = f.read(96)

    def serialize(self):
        r = b""
        r += struct.pack("<B", self.llmqType)
        r += ser_uint256(self.quorumHash)
        r += ser_uint256(self.id)
        r += ser_uint256(self.msgHash)
        r += self.sig
        return r


class CSigShare:
    __slots__ = ("llmqType", "quorumHash", "quorumMember", "id", "msgHash", "sigShare")

    def __init__(self):
        self.llmqType = 0
        self.quorumHash = 0
        self.quorumMember = 0
        self.id = 0
        self.msgHash = 0
        self.sigShare = b'\x00' * 96

    def deserialize(self, f):
        self.llmqType = struct.unpack("<B", f.read(1))[0]
        self.quorumHash = deser_uint256(f)
        self.quorumMember = struct.unpack("<H", f.read(2))[0]
        self.id = deser_uint256(f)
        self.msgHash = deser_uint256(f)
        self.sigShare = f.read(96)

    def serialize(self):
        r = b""
        r += struct.pack("<B", self.llmqType)
        r += ser_uint256(self.quorumHash)
        r += struct.pack("<H", self.quorumMember)
        r += ser_uint256(self.id)
        r += ser_uint256(self.msgHash)
        r += self.sigShare
        return r


class CBLSPublicKey:
    __slots__ = ("data")

    def __init__(self):
        self.data = b'\x00' * 48

    def deserialize(self, f):
        self.data = f.read(48)

    def serialize(self):
        r = b""
        r += self.data
        return r


class CBLSIESEncryptedSecretKey:
    __slots__ = ("ephemeral_pubKey", "iv", "data")

    def __init__(self):
        self.ephemeral_pubKey = b'\x00' * 48
        self.iv = b'\x00' * 32
        self.data = b'\x00' * 32

    def deserialize(self, f):
        self.ephemeral_pubKey = f.read(48)
        self.iv = f.read(32)
        data_size = deser_compact_size(f)
        self.data = f.read(data_size)

    def serialize(self):
        r = b""
        r += self.ephemeral_pubKey
        r += self.iv
        r += ser_compact_size(len(self.data))
        r += self.data
        return r


# Objects that correspond to messages on the wire
class msg_version:
    __slots__ = ("addrFrom", "addrTo", "nNonce", "relay", "nServices",
                 "nStartingHeight", "nTime", "nVersion", "strSubVer")
    msgtype = b"version"

    def __init__(self):
        self.nVersion = 0
        self.nServices = 0
        self.nTime = int(time.time())
        self.addrTo = CAddress()
        self.addrFrom = CAddress()
        self.nNonce = random.getrandbits(64)
        self.strSubVer = ''
        self.nStartingHeight = -1
        self.relay = 0

    def deserialize(self, f):
        self.nVersion = struct.unpack("<i", f.read(4))[0]
        self.nServices = struct.unpack("<Q", f.read(8))[0]
        self.nTime = struct.unpack("<q", f.read(8))[0]
        self.addrTo = CAddress()
        self.addrTo.deserialize(f, with_time=False)

        self.addrFrom = CAddress()
        self.addrFrom.deserialize(f, with_time=False)
        self.nNonce = struct.unpack("<Q", f.read(8))[0]
        self.strSubVer = deser_string(f).decode('utf-8')

        self.nStartingHeight = struct.unpack("<i", f.read(4))[0]

        # Relay field is optional for version 70001 onwards
        # But, unconditionally check it to match behaviour in bitcoind
        try:
            self.relay = struct.unpack("<b", f.read(1))[0]
        except struct.error:
            self.relay = 0

    def serialize(self):
        r = b""
        r += struct.pack("<i", self.nVersion)
        r += struct.pack("<Q", self.nServices)
        r += struct.pack("<q", self.nTime)
        r += self.addrTo.serialize(with_time=False)
        r += self.addrFrom.serialize(with_time=False)
        r += struct.pack("<Q", self.nNonce)
        r += ser_string(self.strSubVer.encode('utf-8'))
        r += struct.pack("<i", self.nStartingHeight)
        r += struct.pack("<b", self.relay)
        return r

    def __repr__(self):
        return 'msg_version(nVersion=%i nServices=%i nTime=%s addrTo=%s addrFrom=%s nNonce=0x%016X strSubVer=%s nStartingHeight=%i relay=%i)' \
               % (self.nVersion, self.nServices, time.ctime(self.nTime),
                  repr(self.addrTo), repr(self.addrFrom), self.nNonce,
                  self.strSubVer, self.nStartingHeight, self.relay)


class msg_verack:
    __slots__ = ()
    msgtype = b"verack"

    def __init__(self):
        pass

    def deserialize(self, f):
        pass

    def serialize(self):
        return b""

    def __repr__(self):
        return "msg_verack()"


class msg_addr:
    __slots__ = ("addrs",)
    msgtype = b"addr"

    def __init__(self):
        self.addrs = []

    def deserialize(self, f):
        self.addrs = deser_vector(f, CAddress)

    def serialize(self):
        return ser_vector(self.addrs)

    def __repr__(self):
        return "msg_addr(addrs=%s)" % (repr(self.addrs))


class msg_addrv2:
    __slots__ = ("addrs",)
    # msgtype = b"addrv2"
    msgtype = b"addrv2"

    def __init__(self):
        self.addrs = []

    def deserialize(self, f):
        self.addrs = deser_vector(f, CAddress, "deserialize_v2")

    def serialize(self):
        return ser_vector(self.addrs, "serialize_v2")

    def __repr__(self):
        return "msg_addrv2(addrs=%s)" % (repr(self.addrs))


class msg_sendaddrv2:
    __slots__ = ()
    # msgtype = b"sendaddrv2"
    msgtype = b"sendaddrv2"

    def __init__(self):
        pass

    def deserialize(self, f):
        pass

    def serialize(self):
        return b""

    def __repr__(self):
        return "msg_sendaddrv2()"


class msg_inv:
    __slots__ = ("inv",)
    msgtype = b"inv"

    def __init__(self, inv=None):
        if inv is None:
            self.inv = []
        else:
            self.inv = inv

    def deserialize(self, f):
        self.inv = deser_vector(f, CInv)

    def serialize(self):
        return ser_vector(self.inv)

    def __repr__(self):
        return "msg_inv(inv=%s)" % (repr(self.inv))


class msg_getdata:
    __slots__ = ("inv",)
    msgtype = b"getdata"

    def __init__(self, inv=None):
        self.inv = inv if inv is not None else []

    def deserialize(self, f):
        self.inv = deser_vector(f, CInv)

    def serialize(self):
        return ser_vector(self.inv)

    def __repr__(self):
        return "msg_getdata(inv=%s)" % (repr(self.inv))


class msg_getblocks:
    __slots__ = ("locator", "hashstop")
    msgtype = b"getblocks"

    def __init__(self):
        self.locator = CBlockLocator()
        self.hashstop = 0

    def deserialize(self, f):
        self.locator = CBlockLocator()
        self.locator.deserialize(f)
        self.hashstop = deser_uint256(f)

    def serialize(self):
        r = b""
        r += self.locator.serialize()
        r += ser_uint256(self.hashstop)
        return r

    def __repr__(self):
        return "msg_getblocks(locator=%s hashstop=%064x)" \
               % (repr(self.locator), self.hashstop)


class msg_tx:
    __slots__ = ("tx",)
    msgtype = b"tx"

    def __init__(self, tx=None):
        if tx is None:
            self.tx = CTransaction()
        else:
            self.tx = tx

    def deserialize(self, f):
        self.tx.deserialize(f)

    def serialize(self):
        return self.tx.serialize()

    def __repr__(self):
        return "msg_tx(tx=%s)" % (repr(self.tx))


class msg_block:
    __slots__ = ("block",)
    msgtype = b"block"

    def __init__(self, block=None):
        if block is None:
            self.block = CBlock()
        else:
            self.block = block

    def deserialize(self, f):
        self.block.deserialize(f)

    def serialize(self):
        return self.block.serialize()

    def __repr__(self):
        return "msg_block(block=%s)" % (repr(self.block))

# for cases where a user needs tighter control over what is sent over the wire
# note that the user must supply the name of the msgtype, and the data
class msg_generic:
    __slots__ = ("data")

    def __init__(self, msgtype, data=None):
        self.msgtype = msgtype
        self.data = data

    def serialize(self):
        return self.data

    def __repr__(self):
        return "msg_generic()"


class msg_getaddr:
    __slots__ = ()
    msgtype = b"getaddr"

    def __init__(self):
        pass

    def deserialize(self, f):
        pass

    def serialize(self):
        return b""

    def __repr__(self):
        return "msg_getaddr()"


class msg_ping:
    __slots__ = ("nonce",)
    msgtype = b"ping"

    def __init__(self, nonce=0):
        self.nonce = nonce

    def deserialize(self, f):
        self.nonce = struct.unpack("<Q", f.read(8))[0]

    def serialize(self):
        r = b""
        r += struct.pack("<Q", self.nonce)
        return r

    def __repr__(self):
        return "msg_ping(nonce=%08x)" % self.nonce


class msg_pong:
    __slots__ = ("nonce",)
    msgtype = b"pong"

    def __init__(self, nonce=0):
        self.nonce = nonce

    def deserialize(self, f):
        self.nonce = struct.unpack("<Q", f.read(8))[0]

    def serialize(self):
        r = b""
        r += struct.pack("<Q", self.nonce)
        return r

    def __repr__(self):
        return "msg_pong(nonce=%08x)" % self.nonce


class msg_mempool:
    __slots__ = ()
    msgtype = b"mempool"

    def __init__(self):
        pass

    def deserialize(self, f):
        pass

    def serialize(self):
        return b""

    def __repr__(self):
        return "msg_mempool()"

class msg_notfound:
    __slots__ = ("vec", )
    msgtype = b"notfound"

    def __init__(self, vec=None):
        self.vec = vec or []

    def deserialize(self, f):
        self.vec = deser_vector(f, CInv)

    def serialize(self):
        return ser_vector(self.vec)

    def __repr__(self):
        return "msg_notfound(vec=%s)" % (repr(self.vec))


class msg_sendheaders:
    __slots__ = ()
    msgtype = b"sendheaders"

    def __init__(self):
        pass

    def deserialize(self, f):
        pass

    def serialize(self):
        return b""

    def __repr__(self):
        return "msg_sendheaders()"


class msg_sendheaders2:
    __slots__ = ()
    msgtype = b"sendheaders2"

    def __init__(self):
        pass

    def deserialize(self, f):
        pass

    def serialize(self):
        return b""

    def __repr__(self):
        return "msg_sendheaders2()"


# getheaders message has
# number of entries
# vector of hashes
# hash_stop (hash of last desired block header, 0 to get as many as possible)
class msg_getheaders:
    __slots__ = ("hashstop", "locator",)
    msgtype = b"getheaders"

    def __init__(self):
        self.locator = CBlockLocator()
        self.hashstop = 0

    def deserialize(self, f):
        self.locator = CBlockLocator()
        self.locator.deserialize(f)
        self.hashstop = deser_uint256(f)

    def serialize(self):
        r = b""
        r += self.locator.serialize()
        r += ser_uint256(self.hashstop)
        return r

    def __repr__(self):
        return "msg_getheaders(locator=%s, stop=%064x)" \
               % (repr(self.locator), self.hashstop)


# same as msg_getheaders, but to request the headers compressed
class msg_getheaders2:
    __slots__ = ("hashstop", "locator",)
    msgtype = b"getheaders2"

    def __init__(self):
        self.locator = CBlockLocator()
        self.hashstop = 0

    def deserialize(self, f):
        self.locator = CBlockLocator()
        self.locator.deserialize(f)
        self.hashstop = deser_uint256(f)

    def serialize(self):
        r = b""
        r += self.locator.serialize()
        r += ser_uint256(self.hashstop)
        return r

    def __repr__(self):
        return "msg_getheaders2(locator=%s, stop=%064x)" \
               % (repr(self.locator), self.hashstop)


# headers message has
# <count> <vector of block headers>
class msg_headers:
    __slots__ = ("headers",)
    msgtype = b"headers"

    def __init__(self, headers=None):
        self.headers = headers if headers is not None else []

    def deserialize(self, f):
        # comment in dashd indicates these should be deserialized as blocks
        blocks = deser_vector(f, CBlock)
        for x in blocks:
            self.headers.append(CBlockHeader(x))

    def serialize(self):
        blocks = [CBlock(x) for x in self.headers]
        return ser_vector(blocks)

    def __repr__(self):
        return "msg_headers(headers=%s)" % repr(self.headers)


# headers message has
# <count> <vector of compressed block headers>
class msg_headers2:
    __slots__ = ("headers",)
    msgtype = b"headers2"

    def __init__(self, headers=None):
        self.headers = headers if headers is not None else []

    def deserialize(self, f):
        self.headers = deser_vector(f, CompressibleBlockHeader)
        last_unique_versions = []
        for idx in range(len(self.headers)):
            self.headers[idx].uncompress(self.headers[:idx], last_unique_versions)

    def serialize(self):
        last_unique_versions = []
        for idx in range(len(self.headers)):
            self.headers[idx].compress(self.headers[:idx], last_unique_versions)
        return ser_vector(self.headers)

    def __repr__(self):
        return "msg_headers2(headers=%s)" % repr(self.headers)

class msg_merkleblock:
    __slots__ = ("merkleblock",)
    msgtype = b"merkleblock"

    def __init__(self, merkleblock=None):
        if merkleblock is None:
            self.merkleblock = CMerkleBlock()
        else:
            self.merkleblock = merkleblock

    def deserialize(self, f):
        self.merkleblock.deserialize(f)

    def serialize(self):
        return self.merkleblock.serialize()

    def __repr__(self):
        return "msg_merkleblock(merkleblock=%s)" % (repr(self.merkleblock))


class msg_filterload:
    __slots__ = ("data", "nHashFuncs", "nTweak", "nFlags")
    msgtype = b"filterload"

    def __init__(self, data=b'00', nHashFuncs=0, nTweak=0, nFlags=0):
        self.data = data
        self.nHashFuncs = nHashFuncs
        self.nTweak = nTweak
        self.nFlags = nFlags

    def deserialize(self, f):
        self.data = deser_string(f)
        self.nHashFuncs = struct.unpack("<I", f.read(4))[0]
        self.nTweak = struct.unpack("<I", f.read(4))[0]
        self.nFlags = struct.unpack("<B", f.read(1))[0]

    def serialize(self):
        r = b""
        r += ser_string(self.data)
        r += struct.pack("<I", self.nHashFuncs)
        r += struct.pack("<I", self.nTweak)
        r += struct.pack("<B", self.nFlags)
        return r

    def __repr__(self):
        return "msg_filterload(data={}, nHashFuncs={}, nTweak={}, nFlags={})".format(
            self.data, self.nHashFuncs, self.nTweak, self.nFlags)


class msg_filteradd:
    __slots__ = ("data")
    msgtype = b"filteradd"

    def __init__(self, data):
        self.data = data

    def deserialize(self, f):
        self.data = deser_string(f)

    def serialize(self):
        r = b""
        r += ser_string(self.data)
        return r

    def __repr__(self):
        return "msg_filteradd(data={})".format(self.data)


class msg_filterclear:
    __slots__ = ()
    msgtype = b"filterclear"

    def __init__(self):
        pass

    def deserialize(self, f):
        pass

    def serialize(self):
        return b""

    def __repr__(self):
        return "msg_filterclear()"


class msg_sendcmpct:
    __slots__ = ("announce", "version")
    msgtype = b"sendcmpct"

    def __init__(self, announce=False, version=1):
        self.announce = announce
        self.version = version

    def deserialize(self, f):
        self.announce = struct.unpack("<?", f.read(1))[0]
        self.version = struct.unpack("<Q", f.read(8))[0]

    def serialize(self):
        r = b""
        r += struct.pack("<?", self.announce)
        r += struct.pack("<Q", self.version)
        return r

    def __repr__(self):
        return "msg_sendcmpct(announce=%s, version=%lu)" % (self.announce, self.version)


class msg_cmpctblock:
    __slots__ = ("header_and_shortids",)
    msgtype = b"cmpctblock"

    def __init__(self, header_and_shortids = None):
        self.header_and_shortids = header_and_shortids

    def deserialize(self, f):
        self.header_and_shortids = P2PHeaderAndShortIDs()
        self.header_and_shortids.deserialize(f)

    def serialize(self):
        r = b""
        r += self.header_and_shortids.serialize()
        return r

    def __repr__(self):
        return "msg_cmpctblock(HeaderAndShortIDs=%s)" % repr(self.header_and_shortids)


class msg_getblocktxn:
    __slots__ = ("block_txn_request",)
    msgtype = b"getblocktxn"

    def __init__(self):
        self.block_txn_request = None

    def deserialize(self, f):
        self.block_txn_request = BlockTransactionsRequest()
        self.block_txn_request.deserialize(f)

    def serialize(self):
        r = b""
        r += self.block_txn_request.serialize()
        return r

    def __repr__(self):
        return "msg_getblocktxn(block_txn_request=%s)" % (repr(self.block_txn_request))


class msg_blocktxn:
    __slots__ = ("block_transactions",)
    msgtype = b"blocktxn"

    def __init__(self):
        self.block_transactions = BlockTransactions()

    def deserialize(self, f):
        self.block_transactions.deserialize(f)

    def serialize(self):
        r = b""
        r += self.block_transactions.serialize()
        return r

    def __repr__(self):
        return "msg_blocktxn(block_transactions=%s)" % (repr(self.block_transactions))


class msg_getmnlistd:
    __slots__ = ("baseBlockHash", "blockHash",)
    msgtype = b"getmnlistd"

    def __init__(self, baseBlockHash=0, blockHash=0):
        self.baseBlockHash = baseBlockHash
        self.blockHash = blockHash

    def deserialize(self, f):
        self.baseBlockHash = deser_uint256(f)
        self.blockHash = deser_uint256(f)

    def serialize(self):
        r = b""
        r += ser_uint256(self.baseBlockHash)
        r += ser_uint256(self.blockHash)
        return r

    def __repr__(self):
        return "msg_getmnlistd(baseBlockHash=%064x, blockHash=%064x)" % (self.baseBlockHash, self.blockHash)

QuorumId = namedtuple('QuorumId', ['llmqType', 'quorumHash'])

class msg_mnlistdiff:
    __slots__ = ("baseBlockHash", "blockHash", "merkleProof", "cbTx", "nVersion", "deletedMNs", "mnList", "deletedQuorums", "newQuorums", "quorumsCLSigs")
    msgtype = b"mnlistdiff"

    def __init__(self):
        self.baseBlockHash = 0
        self.blockHash = 0
        self.merkleProof = CPartialMerkleTree()
        self.cbTx = None
        self.nVersion = 0
        self.deletedMNs = []
        self.mnList = []
        self.deletedQuorums = []
        self.newQuorums = []
        self.quorumsCLSigs = {}


    def deserialize(self, f):
        self.nVersion = struct.unpack("<H", f.read(2))[0]
        self.baseBlockHash = deser_uint256(f)
        self.blockHash = deser_uint256(f)
        self.merkleProof.deserialize(f)
        self.cbTx = CTransaction()
        self.cbTx.deserialize(f)
        self.cbTx.rehash()
        self.deletedMNs = deser_uint256_vector(f)
        self.mnList = []
        for _ in range(deser_compact_size(f)):
            e = CSimplifiedMNListEntry()
            e.deserialize(f)
            self.mnList.append(e)

        self.deletedQuorums = []
        for _ in range(deser_compact_size(f)):
            llmqType = struct.unpack("<B", f.read(1))[0]
            quorumHash = deser_uint256(f)
            self.deletedQuorums.append(QuorumId(llmqType, quorumHash))
        self.newQuorums = []
        for _ in range(deser_compact_size(f)):
            qc = CFinalCommitment()
            qc.deserialize(f)
            self.newQuorums.append(qc)
        self.quorumsCLSigs = {}
        for _ in range(deser_compact_size(f)):
            signature = f.read(96)
            idx_set = set()
            for _ in range(deser_compact_size(f)):
                set_element = struct.unpack('H', f.read(2))[0]
                idx_set.add(set_element)
            self.quorumsCLSigs[signature] = idx_set

    def __repr__(self):
        return "msg_mnlistdiff(baseBlockHash=%064x, blockHash=%064x)" % (self.baseBlockHash, self.blockHash)


class msg_clsig:
    __slots__ = ("height", "blockHash", "sig",)
    msgtype = b"clsig"

    def __init__(self, height=0, blockHash=0, sig=b'\x00' * 96):
        self.height = height
        self.blockHash = blockHash
        self.sig = sig

    def deserialize(self, f):
        self.height = struct.unpack('<i', f.read(4))[0]
        self.blockHash = deser_uint256(f)
        self.sig = f.read(96)

    def serialize(self):
        r = b""
        r += struct.pack('<i', self.height)
        r += ser_uint256(self.blockHash)
        r += self.sig
        return r

    def __repr__(self):
        return "msg_clsig(height=%d, blockHash=%064x)" % (self.height, self.blockHash)


class msg_isdlock:
    __slots__ = ("nVersion", "inputs", "txid", "cycleHash", "sig")
    msgtype = b"isdlock"

    def __init__(self, nVersion=1, inputs=None, txid=0, cycleHash=0, sig=b'\x00' * 96):
        self.nVersion = nVersion
        self.inputs = inputs if inputs is not None else []
        self.txid = txid
        self.cycleHash = cycleHash
        self.sig = sig

    def deserialize(self, f):
        self.nVersion = struct.unpack("<B", f.read(1))[0]
        self.inputs = deser_vector(f, COutPoint)
        self.txid = deser_uint256(f)
        self.cycleHash = deser_uint256(f)
        self.sig = f.read(96)

    def serialize(self):
        r = b""
        r += struct.pack("<B", self.nVersion)
        r += ser_vector(self.inputs)
        r += ser_uint256(self.txid)
        r += ser_uint256(self.cycleHash)
        r += self.sig
        return r

    def __repr__(self):
        return "msg_isdlock(nVersion=%d, inputs=%s, txid=%064x, cycleHash=%064x)" % \
               (self.nVersion, repr(self.inputs), self.txid, self.cycleHash)


class msg_platformban:
    __slots__ = ("protx_hash", "requested_height", "quorum_hash", "sig")
    msgtype = b"platformban"

    def __init__(self, protx_hash=0, requested_height=0, quorum_hash=0, sig=b'\x00' * 96):
        self.protx_hash = protx_hash
        self.requested_height = requested_height
        self.quorum_hash = quorum_hash
        self.sig = sig

    def deserialize(self, f):
        self.protx_hash = deser_uint256(f)
        self.requested_height= struct.unpack("<I", f.read(4))[0]
        self.quorum_hash = deser_uint256(f)
        self.sig = f.read(96)

    def serialize(self):
        r = b""
        r += ser_uint256(self.protx_hash)
        r += struct.pack("<I", self.requested_height)
        r += ser_uint256(self.quorum_hash)
        r += self.sig
        return r

    def calc_sha256(self):
        r = b""
        r += ser_uint256(self.protx_hash)
        r += struct.pack("<I", self.requested_height)
        return uint256_from_str(hash256(r))

    def __repr__(self):
        return "msg_platformban(protx_hash=%064x requested_height=%d, quorum_hash=%064x)" % \
               (self.protx_hash, self.requested_height, self.quorum_hash)


class msg_qsigshare:
    __slots__ = ("sig_shares",)
    msgtype = b"qsigshare"

    def __init__(self, sig_shares=None):
        self.sig_shares = sig_shares if sig_shares is not None else []

    def deserialize(self, f):
        self.sig_shares = deser_vector(f, CSigShare)

    def serialize(self):
        r = b""
        r += ser_vector(self.sig_shares)
        return r

    def __repr__(self):
        return "msg_qsigshare(sigShares=%d)" % (len(self.sig_shares))


class msg_qwatch:
    __slots__ = ()
    msgtype = b"qwatch"

    def __init__(self):
        pass

    def deserialize(self, f):
        pass

    def serialize(self):
        return b""

    def __repr__(self):
        return "msg_qwatch()"


class msg_qgetdata:
    __slots__ = ("quorum_hash", "quorum_type", "data_mask", "protx_hash")
    msgtype = b"qgetdata"

    def __init__(self, quorum_hash=0, quorum_type=-1, data_mask=0, protx_hash=0):
        self.quorum_hash = quorum_hash
        self.quorum_type = quorum_type
        self.data_mask = data_mask
        self.protx_hash = protx_hash

    def deserialize(self, f):
        self.quorum_type = struct.unpack("<B", f.read(1))[0]
        self.quorum_hash = deser_uint256(f)
        self.data_mask = struct.unpack("<H", f.read(2))[0]
        self.protx_hash = deser_uint256(f)

    def serialize(self):
        r = b""
        r += struct.pack("<B", self.quorum_type)
        r += ser_uint256(self.quorum_hash)
        r += struct.pack("<H", self.data_mask)
        r += ser_uint256(self.protx_hash)
        return r

    def __repr__(self):
        return "msg_qgetdata(quorum_hash=%064x, quorum_type=%d, data_mask=%d, protx_hash=%064x)" % (
                                                                                self.quorum_hash,
                                                                                self.quorum_type,
                                                                                self.data_mask,
                                                                                self.protx_hash)


class msg_qdata:
    __slots__ = ("quorum_hash", "quorum_type", "data_mask", "protx_hash", "error", "quorum_vvec", "enc_contributions",)
    msgtype = b"qdata"

    def __init__(self):
        self.quorum_type = 0
        self.quorum_hash = 0
        self.data_mask = 0
        self.protx_hash = 0
        self.error = 0
        self.quorum_vvec = list()
        self.enc_contributions = list()

    def deserialize(self, f):
        self.quorum_type = struct.unpack("<B", f.read(1))[0]
        self.quorum_hash = deser_uint256(f)
        self.data_mask = struct.unpack("<H", f.read(2))[0]
        self.protx_hash = deser_uint256(f)
        self.error = struct.unpack("<B", f.read(1))[0]
        if self.error == 0:
            if self.data_mask & 0x01:
                self.quorum_vvec = deser_vector(f, CBLSPublicKey)
            if self.data_mask & 0x02:
                self.enc_contributions = deser_vector(f, CBLSIESEncryptedSecretKey)

    def serialize(self):
        r = b""
        r += struct.pack("<B", self.quorum_type)
        r += ser_uint256(self.quorum_hash)
        r += struct.pack("<H", self.data_mask)
        r += ser_uint256(self.protx_hash)
        r += struct.pack("<B", self.error)
        if self.error == 0:
            if self.data_mask & 0x01:
                r += ser_vector(self.quorum_vvec)
            if self.data_mask & 0x02:
                r += ser_vector(self.enc_contributions)
        return r

    def __repr__(self):
        return "msg_qdata(error=%d, quorum_vvec=%d, enc_contributions=%d)" % (self.error, len(self.quorum_vvec),
                                                                                          len(self.enc_contributions))

class msg_getcfilters:
    __slots__ = ("filter_type", "start_height", "stop_hash")
    msgtype =  b"getcfilters"

    def __init__(self, filter_type=None, start_height=None, stop_hash=None):
        self.filter_type = filter_type
        self.start_height = start_height
        self.stop_hash = stop_hash

    def deserialize(self, f):
        self.filter_type = struct.unpack("<B", f.read(1))[0]
        self.start_height = struct.unpack("<I", f.read(4))[0]
        self.stop_hash = deser_uint256(f)

    def serialize(self):
        r = b""
        r += struct.pack("<B", self.filter_type)
        r += struct.pack("<I", self.start_height)
        r += ser_uint256(self.stop_hash)
        return r

    def __repr__(self):
        return "msg_getcfilters(filter_type={:#x}, start_height={}, stop_hash={:x})".format(
            self.filter_type, self.start_height, self.stop_hash)

class msg_cfilter:
    __slots__ = ("filter_type", "block_hash", "filter_data")
    msgtype =  b"cfilter"

    def __init__(self, filter_type=None, block_hash=None, filter_data=None):
        self.filter_type = filter_type
        self.block_hash = block_hash
        self.filter_data = filter_data

    def deserialize(self, f):
        self.filter_type = struct.unpack("<B", f.read(1))[0]
        self.block_hash = deser_uint256(f)
        self.filter_data = deser_string(f)

    def serialize(self):
        r = b""
        r += struct.pack("<B", self.filter_type)
        r += ser_uint256(self.block_hash)
        r += ser_string(self.filter_data)
        return r

    def __repr__(self):
        return "msg_cfilter(filter_type={:#x}, block_hash={:x})".format(
            self.filter_type, self.block_hash)

class msg_getcfheaders:
    __slots__ = ("filter_type", "start_height", "stop_hash")
    msgtype =  b"getcfheaders"

    def __init__(self, filter_type=None, start_height=None, stop_hash=None):
        self.filter_type = filter_type
        self.start_height = start_height
        self.stop_hash = stop_hash

    def deserialize(self, f):
        self.filter_type = struct.unpack("<B", f.read(1))[0]
        self.start_height = struct.unpack("<I", f.read(4))[0]
        self.stop_hash = deser_uint256(f)

    def serialize(self):
        r = b""
        r += struct.pack("<B", self.filter_type)
        r += struct.pack("<I", self.start_height)
        r += ser_uint256(self.stop_hash)
        return r

    def __repr__(self):
        return "msg_getcfheaders(filter_type={:#x}, start_height={}, stop_hash={:x})".format(
            self.filter_type, self.start_height, self.stop_hash)

class msg_cfheaders:
    __slots__ = ("filter_type", "stop_hash", "prev_header", "hashes")
    msgtype =  b"cfheaders"

    def __init__(self, filter_type=None, stop_hash=None, prev_header=None, hashes=None):
        self.filter_type = filter_type
        self.stop_hash = stop_hash
        self.prev_header = prev_header
        self.hashes = hashes

    def deserialize(self, f):
        self.filter_type = struct.unpack("<B", f.read(1))[0]
        self.stop_hash = deser_uint256(f)
        self.prev_header = deser_uint256(f)
        self.hashes = deser_uint256_vector(f)

    def serialize(self):
        r = b""
        r += struct.pack("<B", self.filter_type)
        r += ser_uint256(self.stop_hash)
        r += ser_uint256(self.prev_header)
        r += ser_uint256_vector(self.hashes)
        return r

    def __repr__(self):
        return "msg_cfheaders(filter_type={:#x}, stop_hash={:x})".format(
            self.filter_type, self.stop_hash)

class msg_getcfcheckpt:
    __slots__ = ("filter_type", "stop_hash")
    msgtype =  b"getcfcheckpt"

    def __init__(self, filter_type=None, stop_hash=None):
        self.filter_type = filter_type
        self.stop_hash = stop_hash

    def deserialize(self, f):
        self.filter_type = struct.unpack("<B", f.read(1))[0]
        self.stop_hash = deser_uint256(f)

    def serialize(self):
        r = b""
        r += struct.pack("<B", self.filter_type)
        r += ser_uint256(self.stop_hash)
        return r

    def __repr__(self):
        return "msg_getcfcheckpt(filter_type={:#x}, stop_hash={:x})".format(
            self.filter_type, self.stop_hash)

class msg_cfcheckpt:
    __slots__ = ("filter_type", "stop_hash", "headers")
    msgtype =  b"cfcheckpt"

    def __init__(self, filter_type=None, stop_hash=None, headers=None):
        self.filter_type = filter_type
        self.stop_hash = stop_hash
        self.headers = headers

    def deserialize(self, f):
        self.filter_type = struct.unpack("<B", f.read(1))[0]
        self.stop_hash = deser_uint256(f)
        self.headers = deser_uint256_vector(f)

    def serialize(self):
        r = b""
        r += struct.pack("<B", self.filter_type)
        r += ser_uint256(self.stop_hash)
        r += ser_uint256_vector(self.headers)
        return r

    def __repr__(self):
        return "msg_cfcheckpt(filter_type={:#x}, stop_hash={:x})".format(
            self.filter_type, self.stop_hash)

class msg_sendtxrcncl:
    __slots__ = ("version", "salt")
    msgtype = b"sendtxrcncl"

    def __init__(self):
        self.version = 0
        self.salt = 0

    def deserialize(self, f):
        self.version = struct.unpack("<I", f.read(4))[0]
        self.salt = struct.unpack("<Q", f.read(8))[0]

    def serialize(self):
        r = b""
        r += struct.pack("<I", self.version)
        r += struct.pack("<Q", self.salt)
        return r

    def __repr__(self):
        return "msg_sendtxrcncl(version=%lu, salt=%lu)" %\
            (self.version, self.salt)

class TestFrameworkScript(unittest.TestCase):
    def test_addrv2_encode_decode(self):
        def check_addrv2(ip, net):
            addr = CAddress()
            addr.net, addr.ip = net, ip
            ser = addr.serialize_v2()
            actual = CAddress()
            actual.deserialize_v2(BytesIO(ser))
            self.assertEqual(actual, addr)

        check_addrv2("1.65.195.98", CAddress.NET_IPV4)
        check_addrv2("2001:41f0::62:6974:636f:696e", CAddress.NET_IPV6)
        check_addrv2("2bqghnldu6mcug4pikzprwhtjjnsyederctvci6klcwzepnjd46ikjyd.onion", CAddress.NET_TORV3)
        check_addrv2("255fhcp6ajvftnyo7bwz3an3t4a4brhopm3bamyh2iu5r3gnr2rq.b32.i2p", CAddress.NET_I2P)
        check_addrv2("fc32:17ea:e415:c3bf:9808:149d:b5a2:c9aa", CAddress.NET_CJDNS)
