#!/usr/bin/env python3
# Copyright (c) 2010 ArtForz -- public domain half-a-node
# Copyright (c) 2012 Jeff Garzik
# Copyright (c) 2010-2017 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Bitcoin test framework primitive and message structures
CBlock, CTransaction, CBlockHeader, CTxIn, CTxOut, etc....:
    data structures that should map to corresponding structures in
    bitcoin/primitives
msg_block, msg_tx, msg_headers, etc.:
    data structures that represent network messages
ser_*, deser_*: functions that handle serialization/deserialization."""
from codecs import encode
import copy
from collections import namedtuple
import hashlib
from io import BytesIO
import random
import socket
import struct
import time

from test_framework.siphash import siphash256
from test_framework.util import hex_str_to_bytes, bytes_to_hex_str

import dash_hash

MIN_VERSION_SUPPORTED = 60001
MY_VERSION = 70219  # LLMQ_DATA_MESSAGES_VERSION
MY_SUBVERSION = b"/python-mininode-tester:0.0.3%s/"
MY_RELAY = 1 # from version 70001 onwards, fRelay should be appended to version messages (BIP37)

MAX_INV_SZ = 50000
MAX_LOCATOR_SZ = 101
MAX_BLOCK_SIZE = 1000000

COIN = 100000000  # 1 btc in satoshis

BIP125_SEQUENCE_NUMBER = 0xfffffffd  # Sequence number that is BIP 125 opt-in and BIP 68-opt-out

NODE_NETWORK = (1 << 0)
# NODE_GETUTXO = (1 << 1)
NODE_BLOOM = (1 << 2)
NODE_NETWORK_LIMITED = (1 << 10)

# Serialization/deserialization tools
def sha256(s):
    return hashlib.new('sha256', s).digest()


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
    r = 0
    for i in range(8):
        t = struct.unpack("<I", f.read(4))[0]
        r += t << (i * 32)
    return r


def ser_uint256(u):
    rs = b""
    for i in range(8):
        rs += struct.pack("<I", u & 0xFFFFFFFF)
        u >>= 32
    return rs


def uint256_from_str(s):
    r = 0
    t = struct.unpack("<IIIIIIII", s[:32])
    for i in range(8):
        r += t[i] << (i * 32)
    return r


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
    for i in range(nit):
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
    for i in range(nit):
        t = deser_uint256(f)
        r.append(t)
    return r


def ser_uint256_vector(l):
    r = ser_compact_size(len(l))
    for i in l:
        r += ser_uint256(i)
    return r


def deser_string_vector(f):
    nit = deser_compact_size(f)
    r = []
    for i in range(nit):
        t = deser_string(f)
        r.append(t)
    return r


def ser_string_vector(l):
    r = ser_compact_size(len(l))
    for sv in l:
        r += ser_string(sv)
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


# Deserialize from a hex string representation (eg from RPC)
def FromHex(obj, hex_string):
    obj.deserialize(BytesIO(hex_str_to_bytes(hex_string)))
    return obj

# Convert a binary-serializable object to hex (eg for submission via RPC)
def ToHex(obj):
    return bytes_to_hex_str(obj.serialize())

# Objects that map to dashd objects, which can be serialized/deserialized

class CService():
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


class CAddress():
    __slots__ = ("net", "ip", "nServices", "port", "time")

    # see https://github.com/bitcoin/bips/blob/master/bip-0155.mediawiki
    NET_IPV4 = 1

    ADDRV2_NET_NAME = {
        NET_IPV4: "IPv4"
    }

    ADDRV2_ADDRESS_LENGTH = {
        NET_IPV4: 4
    }

    def __init__(self):
        self.time = 0
        self.nServices = 1
        self.net = self.NET_IPV4
        self.ip = "0.0.0.0"
        self.port = 0

    def deserialize(self, f, with_time=True):
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

    def serialize(self, with_time=True):
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
        assert self.net == self.NET_IPV4

        address_length = deser_compact_size(f)
        assert address_length == self.ADDRV2_ADDRESS_LENGTH[self.net]

        self.ip = socket.inet_ntoa(f.read(4))

        self.port = struct.unpack(">H", f.read(2))[0]

    def serialize_v2(self):
        """Serialize in addrv2 format (BIP155)"""
        assert self.net == self.NET_IPV4
        r = b""
        r += struct.pack("<I", self.time)
        r += ser_compact_size(self.nServices)
        r += struct.pack("B", self.net)
        r += ser_compact_size(self.ADDRV2_ADDRESS_LENGTH[self.net])
        r += socket.inet_aton(self.ip)
        r += struct.pack(">H", self.port)
        return r

    def __repr__(self):
        return ("CAddress(nServices=%i net=%s addr=%s port=%i)"
                % (self.nServices, self.ADDRV2_NET_NAME[self.net], self.ip, self.port))


class CInv():
    typemap = {
        0: "Error",
        1: "TX",
        2: "Block",
        20: "CompactBlock"
    }

    def __init__(self, t=0, h=0):
        self.type = t
        self.hash = h

    def deserialize(self, f):
        self.type = struct.unpack("<i", f.read(4))[0]
        self.hash = deser_uint256(f)

    def serialize(self):
        r = b""
        r += struct.pack("<i", self.type)
        r += ser_uint256(self.hash)
        return r

    def __repr__(self):
        return "CInv(type=%s hash=%064x)" \
               % (self.typemap.get(self.type, "%d" % self.type), self.hash)


class CBlockLocator():
    def __init__(self):
        self.nVersion = MY_VERSION
        self.vHave = []

    def deserialize(self, f):
        self.nVersion = struct.unpack("<i", f.read(4))[0]
        self.vHave = deser_uint256_vector(f)

    def serialize(self):
        r = b""
        r += struct.pack("<i", self.nVersion)
        r += ser_uint256_vector(self.vHave)
        return r

    def __repr__(self):
        return "CBlockLocator(nVersion=%i vHave=%s)" \
               % (self.nVersion, repr(self.vHave))


class COutPoint():
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


class CTxIn():
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
               % (repr(self.prevout), bytes_to_hex_str(self.scriptSig),
                  self.nSequence)


class CTxOut():
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
                  bytes_to_hex_str(self.scriptPubKey))


class CTransaction():
    def __init__(self, tx=None):
        if tx is None:
            self.nVersion = 1
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
        self.hash = encode(hash256(self.serialize())[::-1], 'hex_codec').decode('ascii')

    def is_valid(self):
        self.calc_sha256()
        for tout in self.vout:
            if tout.nValue < 0 or tout.nValue > 21000000 * COIN:
                return False
        return True

    def __repr__(self):
        return "CTransaction(nVersion=%i vin=%s vout=%s nLockTime=%i)" \
               % (self.nVersion, repr(self.vin), repr(self.vout), self.nLockTime)


class CBlockHeader():
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
        self.nVersion = 1
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
            self.hash = encode(dashhash(r)[::-1], 'hex_codec').decode('ascii')

    def rehash(self):
        self.sha256 = None
        self.calc_sha256()
        return self.sha256

    def __repr__(self):
        return "CBlockHeader(nVersion=%i hashPrevBlock=%064x hashMerkleRoot=%064x nTime=%s nBits=%08x nNonce=%08x)" \
               % (self.nVersion, self.hashPrevBlock, self.hashMerkleRoot,
                  time.ctime(self.nTime), self.nBits, self.nNonce)


class CBlock(CBlockHeader):
    def __init__(self, header=None):
        super(CBlock, self).__init__(header)
        self.vtx = []

    def deserialize(self, f):
        super(CBlock, self).deserialize(f)
        self.vtx = deser_vector(f, CTransaction)

    def serialize(self):
        r = b""
        r += super(CBlock, self).serialize()
        r += ser_vector(self.vtx)
        return r

    # Calculate the merkle root given a vector of transaction hashes
    @staticmethod
    def get_merkle_root(hashes):
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

    def __repr__(self):
        return "CBlock(nVersion=%i hashPrevBlock=%064x hashMerkleRoot=%064x nTime=%s nBits=%08x nNonce=%08x vtx=%s)" \
               % (self.nVersion, self.hashPrevBlock, self.hashMerkleRoot,
                  time.ctime(self.nTime), self.nBits, self.nNonce, repr(self.vtx))

class PrefilledTransaction():
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
class P2PHeaderAndShortIDs():
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
        for i in range(self.shortids_length):
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
class HeaderAndShortIDs():
    def __init__(self, p2pheaders_and_shortids = None):
        self.header = CBlockHeader()
        self.nonce = 0
        self.shortids = []
        self.prefilled_txn = []

        if p2pheaders_and_shortids != None:
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

    def initialize_from_block(self, block, nonce=0, prefill_list = [0]):
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


class BlockTransactionsRequest():

    def __init__(self, blockhash=0, indexes = None):
        self.blockhash = blockhash
        self.indexes = indexes if indexes != None else []

    def deserialize(self, f):
        self.blockhash = deser_uint256(f)
        indexes_length = deser_compact_size(f)
        for i in range(indexes_length):
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


class BlockTransactions():

    def __init__(self, blockhash=0, transactions = None):
        self.blockhash = blockhash
        self.transactions = transactions if transactions != None else []

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


class CPartialMerkleTree():
    def __init__(self):
        self.nTransactions = 0
        self.vBits = []
        self.vHash = []
        self.fBad = False

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


class CMerkleBlock():
    def __init__(self, header=CBlockHeader(), txn=CPartialMerkleTree()):
        self.header = header
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


class CCbTx():
    def __init__(self, version=None, height=None, merkleRootMNList=None, merkleRootQuorums=None):
        self.set_null()
        if version is not None:
            self.version = version
        if height is not None:
            self.height = height
        if merkleRootMNList is not None:
            self.merkleRootMNList = merkleRootMNList
        if merkleRootQuorums is not None:
            self.merkleRootQuorums = merkleRootQuorums

    def set_null(self):
        self.version = 0
        self.height = 0
        self.merkleRootMNList = None

    def deserialize(self, f):
        self.version = struct.unpack("<H", f.read(2))[0]
        self.height = struct.unpack("<i", f.read(4))[0]
        self.merkleRootMNList = deser_uint256(f)
        if self.version >= 2:
            self.merkleRootQuorums = deser_uint256(f)

    def serialize(self):
        r = b""
        r += struct.pack("<H", self.version)
        r += struct.pack("<i", self.height)
        r += ser_uint256(self.merkleRootMNList)
        if self.version >= 2:
            r += ser_uint256(self.merkleRootQuorums)
        return r


class CSimplifiedMNListEntry():
    def __init__(self):
        self.set_null()

    def set_null(self):
        self.proRegTxHash = 0
        self.confirmedHash = 0
        self.service = CService()
        self.pubKeyOperator = b'\\x0' * 48
        self.keyIDVoting = 0
        self.isValid = False

    def deserialize(self, f):
        self.proRegTxHash = deser_uint256(f)
        self.confirmedHash = deser_uint256(f)
        self.service.deserialize(f)
        self.pubKeyOperator = f.read(48)
        self.keyIDVoting = f.read(20)
        self.isValid = struct.unpack("<?", f.read(1))[0]

    def serialize(self):
        r = b""
        r += ser_uint256(self.proRegTxHash)
        r += ser_uint256(self.confirmedHash)
        r += self.service.serialize()
        r += self.pubKeyOperator
        r += self.keyIDVoting
        r += struct.pack("<?", self.isValid)
        return r


class CFinalCommitment:
    def __init__(self):
        self.set_null()

    def set_null(self):
        self.nVersion = 0
        self.llmqType = 0
        self.quorumHash = 0
        self.signers = []
        self.validMembers = []
        self.quorumPublicKey = b'\\x0' * 48
        self.quorumVvecHash = 0
        self.quorumSig = b'\\x0' * 96
        self.membersSig = b'\\x0' * 96

    def deserialize(self, f):
        self.nVersion = struct.unpack("<H", f.read(2))[0]
        self.llmqType = struct.unpack("<B", f.read(1))[0]
        self.quorumHash = deser_uint256(f)
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
        r += ser_dyn_bitset(self.signers, False)
        r += ser_dyn_bitset(self.validMembers, False)
        r += self.quorumPublicKey
        r += ser_uint256(self.quorumVvecHash)
        r += self.quorumSig
        r += self.membersSig
        return r


class CGovernanceObject:
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
    def __init__(self):
        self.llmqType = 0
        self.quorumHash = 0
        self.id = 0
        self.msgHash = 0
        self.sig = b'\\x0' * 96

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
    def __init__(self):
        self.llmqType = 0
        self.quorumHash = 0
        self.quorumMember = 0
        self.id = 0
        self.msgHash = 0
        self.sigShare = b'\\x0' * 96

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
    def __init__(self):
        self.data = b'\\x0' * 48

    def deserialize(self, f):
        self.data = f.read(48)

    def serialize(self):
        r = b""
        r += self.data
        return r


class CBLSIESEncryptedSecretKey:
    def __init__(self):
        self.ephemeral_pubKey = b'\\x0' * 48
        self.iv = b'\\x0' * 32
        self.data = b'\\x0' * 32

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
class msg_version():
    command = b"version"

    def __init__(self):
        self.nVersion = MY_VERSION
        self.nServices = 1
        self.nTime = int(time.time())
        self.addrTo = CAddress()
        self.addrFrom = CAddress()
        self.nNonce = random.getrandbits(64)
        self.strSubVer = MY_SUBVERSION % b""
        self.nStartingHeight = -1
        self.nRelay = MY_RELAY

    def deserialize(self, f):
        self.nVersion = struct.unpack("<i", f.read(4))[0]
        self.nServices = struct.unpack("<Q", f.read(8))[0]
        self.nTime = struct.unpack("<q", f.read(8))[0]
        self.addrTo = CAddress()
        self.addrTo.deserialize(f, False)

        self.addrFrom = CAddress()
        self.addrFrom.deserialize(f, False)
        self.nNonce = struct.unpack("<Q", f.read(8))[0]
        self.strSubVer = deser_string(f)

        self.nStartingHeight = struct.unpack("<i", f.read(4))[0]

        if self.nVersion >= 70001:
            # Relay field is optional for version 70001 onwards
            try:
                self.nRelay = struct.unpack("<b", f.read(1))[0]
            except:
                self.nRelay = 0
        else:
            self.nRelay = 0

    def serialize(self):
        r = b""
        r += struct.pack("<i", self.nVersion)
        r += struct.pack("<Q", self.nServices)
        r += struct.pack("<q", self.nTime)
        r += self.addrTo.serialize(False)
        r += self.addrFrom.serialize(False)
        r += struct.pack("<Q", self.nNonce)
        r += ser_string(self.strSubVer)
        r += struct.pack("<i", self.nStartingHeight)
        r += struct.pack("<b", self.nRelay)
        return r

    def __repr__(self):
        return 'msg_version(nVersion=%i nServices=%i nTime=%s addrTo=%s addrFrom=%s nNonce=0x%016X strSubVer=%s nStartingHeight=%i nRelay=%i)' \
               % (self.nVersion, self.nServices, time.ctime(self.nTime),
                  repr(self.addrTo), repr(self.addrFrom), self.nNonce,
                  self.strSubVer, self.nStartingHeight, self.nRelay)


class msg_verack():
    command = b"verack"

    def __init__(self):
        pass

    def deserialize(self, f):
        pass

    def serialize(self):
        return b""

    def __repr__(self):
        return "msg_verack()"


class msg_addr():
    command = b"addr"

    def __init__(self):
        self.addrs = []

    def deserialize(self, f):
        self.addrs = deser_vector(f, CAddress)

    def serialize(self):
        return ser_vector(self.addrs)

    def __repr__(self):
        return "msg_addr(addrs=%s)" % (repr(self.addrs))


class msg_addrv2():
    # __slots__ = ("addrs",)
    # msgtype = b"addrv2"
    command = b"addrv2"

    def __init__(self):
        self.addrs = []

    def deserialize(self, f):
        self.addrs = deser_vector(f, CAddress, "deserialize_v2")

    def serialize(self):
        return ser_vector(self.addrs, "serialize_v2")

    def __repr__(self):
        return "msg_addrv2(addrs=%s)" % (repr(self.addrs))


class msg_sendaddrv2():
    # __slots__ = ()
    # msgtype = b"sendaddrv2"
    command = b"sendaddrv2"

    def __init__(self):
        pass

    def deserialize(self, f):
        pass

    def serialize(self):
        return b""

    def __repr__(self):
        return "msg_sendaddrv2()"


class msg_inv():
    command = b"inv"

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


class msg_getdata():
    command = b"getdata"

    def __init__(self, inv=None):
        self.inv = inv if inv != None else []

    def deserialize(self, f):
        self.inv = deser_vector(f, CInv)

    def serialize(self):
        return ser_vector(self.inv)

    def __repr__(self):
        return "msg_getdata(inv=%s)" % (repr(self.inv))


class msg_getblocks():
    command = b"getblocks"

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


class msg_tx():
    command = b"tx"

    def __init__(self, tx=CTransaction()):
        self.tx = tx

    def deserialize(self, f):
        self.tx.deserialize(f)

    def serialize(self):
        return self.tx.serialize()

    def __repr__(self):
        return "msg_tx(tx=%s)" % (repr(self.tx))


class msg_block():
    command = b"block"

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
# note that the user must supply the name of the command, and the data
class msg_generic():
    def __init__(self, command, data=None):
        self.command = command
        self.data = data

    def serialize(self):
        return self.data

    def __repr__(self):
        return "msg_generic()"

class msg_getaddr():
    command = b"getaddr"

    def __init__(self):
        pass

    def deserialize(self, f):
        pass

    def serialize(self):
        return b""

    def __repr__(self):
        return "msg_getaddr()"


class msg_ping():
    command = b"ping"

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


class msg_pong():
    command = b"pong"

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


class msg_mempool():
    command = b"mempool"

    def __init__(self):
        pass

    def deserialize(self, f):
        pass

    def serialize(self):
        return b""

    def __repr__(self):
        return "msg_mempool()"

class msg_sendheaders():
    command = b"sendheaders"

    def __init__(self):
        pass

    def deserialize(self, f):
        pass

    def serialize(self):
        return b""

    def __repr__(self):
        return "msg_sendheaders()"


# getheaders message has
# number of entries
# vector of hashes
# hash_stop (hash of last desired block header, 0 to get as many as possible)
class msg_getheaders():
    command = b"getheaders"

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


# headers message has
# <count> <vector of block headers>
class msg_headers():
    command = b"headers"

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


class msg_reject():
    command = b"reject"
    REJECT_MALFORMED = 1

    def __init__(self):
        self.message = b""
        self.code = 0
        self.reason = b""
        self.data = 0

    def deserialize(self, f):
        self.message = deser_string(f)
        self.code = struct.unpack("<B", f.read(1))[0]
        self.reason = deser_string(f)
        if (self.code != self.REJECT_MALFORMED and
                (self.message == b"block" or self.message == b"tx")):
            self.data = deser_uint256(f)

    def serialize(self):
        r = ser_string(self.message)
        r += struct.pack("<B", self.code)
        r += ser_string(self.reason)
        if (self.code != self.REJECT_MALFORMED and
                (self.message == b"block" or self.message == b"tx")):
            r += ser_uint256(self.data)
        return r

    def __repr__(self):
        return "msg_reject: %s %d %s [%064x]" \
               % (self.message, self.code, self.reason, self.data)


class msg_sendcmpct():
    command = b"sendcmpct"

    def __init__(self):
        self.announce = False
        self.version = 1

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

class msg_cmpctblock():
    command = b"cmpctblock"

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

class msg_getblocktxn():
    command = b"getblocktxn"

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

class msg_blocktxn():
    command = b"blocktxn"

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

class msg_getmnlistd():
    command = b"getmnlistd"

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

class msg_mnlistdiff():
    command = b"mnlistdiff"

    def __init__(self):
        self.baseBlockHash = 0
        self.blockHash = 0
        self.merkleProof = CPartialMerkleTree()
        self.cbTx = None
        self.deletedMNs = []
        self.mnList = []
        self.deletedQuorums = []
        self.newQuorums = []

    def deserialize(self, f):
        self.baseBlockHash = deser_uint256(f)
        self.blockHash = deser_uint256(f)
        self.merkleProof.deserialize(f)
        self.cbTx = CTransaction()
        self.cbTx.deserialize(f)
        self.cbTx.rehash()
        self.deletedMNs = deser_uint256_vector(f)
        self.mnList = []
        for i in range(deser_compact_size(f)):
            e = CSimplifiedMNListEntry()
            e.deserialize(f)
            self.mnList.append(e)

        self.deletedQuorums = []
        for i in range(deser_compact_size(f)):
            llmqType = struct.unpack("<B", f.read(1))[0]
            quorumHash = deser_uint256(f)
            self.deletedQuorums.append(QuorumId(llmqType, quorumHash))
        self.newQuorums = []
        for i in range(deser_compact_size(f)):
            qc = CFinalCommitment()
            qc.deserialize(f)
            self.newQuorums.append(qc)

    def __repr__(self):
        return "msg_mnlistdiff(baseBlockHash=%064x, blockHash=%064x)" % (self.baseBlockHash, self.blockHash)


class msg_clsig():
    command = b"clsig"

    def __init__(self, height=0, blockHash=0, sig=b'\\x0' * 96):
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


class msg_islock():
    command = b"islock"

    def __init__(self, inputs=[], txid=0, sig=b'\\x0' * 96):
        self.inputs = inputs
        self.txid = txid
        self.sig = sig

    def deserialize(self, f):
        self.inputs = deser_vector(f, COutPoint)
        self.txid = deser_uint256(f)
        self.sig = f.read(96)

    def serialize(self):
        r = b""
        r += ser_vector(self.inputs)
        r += ser_uint256(self.txid)
        r += self.sig
        return r

    def __repr__(self):
        return "msg_islock(inputs=%s, txid=%064x)" % (repr(self.inputs), self.txid)


class msg_qsigshare():
    command = b"qsigshare"

    def __init__(self, sig_shares=[]):
        self.sig_shares = sig_shares

    def deserialize(self, f):
        self.sig_shares = deser_vector(f, CSigShare)

    def serialize(self):
        r = b""
        r += ser_vector(self.sig_shares)
        return r

    def __repr__(self):
        return "msg_qsigshare(sigShares=%d)" % (len(self.sig_shares))


class msg_qwatch():
    command = b"qwatch"

    def __init__(self):
        pass

    def deserialize(self, f):
        pass

    def serialize(self):
        return b""

    def __repr__(self):
        return "msg_qwatch()"


class msg_qgetdata():
    command = b"qgetdata"

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


class msg_qdata():
    command = b"qdata"

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
