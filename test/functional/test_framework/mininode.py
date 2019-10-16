#!/usr/bin/env python3
# Copyright (c) 2010 ArtForz -- public domain half-a-node
# Copyright (c) 2012 Jeff Garzik
# Copyright (c) 2010-2016 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Dash P2P network half-a-node.

This python code was modified from ArtForz' public domain  half-a-node, as
found in the mini-node branch of http://github.com/jgarzik/pynode.

NodeConn: an object which manages p2p connectivity to a bitcoin node
NodeConnCB: a base class that describes the interface for receiving
            callbacks with network messages from a NodeConn
P2PDataStore: A p2p interface class that keeps a store of transactions and blocks
              and can respond correctly to getdata and getheaders messages
CBlock, CTransaction, CBlockHeader, CTxIn, CTxOut, etc....:
    data structures that should map to corresponding structures in
    bitcoin/primitives
msg_block, msg_tx, msg_headers, etc.:
    data structures that represent network messages
ser_*, deser_*: functions that handle serialization/deserialization
"""

import asyncore
from collections import namedtuple

from codecs import encode
from collections import defaultdict
import copy
import hashlib
from io import BytesIO
import logging
import random
import socket
import struct
import sys
import time
import threading

from test_framework.siphash import siphash256
from test_framework.util import hex_str_to_bytes, bytes_to_hex_str, wait_until

import dash_hash

BIP0031_VERSION = 60000
MY_VERSION = 70214  # MIN_PEER_PROTO_VERSION
MY_SUBVERSION = b"/python-mininode-tester:0.0.3/"
MY_RELAY = 1 # from version 70001 onwards, fRelay should be appended to version messages (BIP37)

MAX_INV_SZ = 50000
MAX_BLOCK_SIZE = 1000000

COIN = 100000000 # 1 btc in satoshis

NODE_NETWORK = (1 << 0)
NODE_GETUTXO = (1 << 1)
NODE_BLOOM = (1 << 2)

MSG_TX = 1
MSG_BLOCK = 2
MSG_TYPE_MASK = 0xffffffff >> 2

logger = logging.getLogger("TestFramework.mininode")

# Keep our own socket map for asyncore, so that we can track disconnects
# ourselves (to workaround an issue with closing an asyncore socket when
# using select)
mininode_socket_map = dict()

# One lock for synchronizing all data access between the networking thread (see
# NetworkThread below) and the thread running the test logic.  For simplicity,
# NodeConn acquires this lock whenever delivering a message to a NodeConnCB,
# and whenever adding anything to the send buffer (in send_message()).  This
# lock should be acquired in the thread running the test logic to synchronize
# access to any data shared with the NodeConnCB or NodeConn.
mininode_lock = threading.RLock()

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


def uint256_from_compact(c):
    nbytes = (c >> 24) & 0xFF
    v = (c & 0xFFFFFF) << (8 * (nbytes - 3))
    return v


def deser_vector(f, c):
    nit = deser_compact_size(f)
    r = []
    for i in range(nit):
        t = c()
        t.deserialize(f)
        r.append(t)
    return r


def ser_vector(l):
    r = ser_compact_size(len(l))
    for i in l:
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


def deser_int_vector(f):
    nit = deser_compact_size(f)
    r = []
    for i in range(nit):
        t = struct.unpack("<i", f.read(4))[0]
        r.append(t)
    return r


def ser_int_vector(l):
    r = ser_compact_size(len(l))
    for i in l:
        r += struct.pack("<i", i)
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

class CService(object):
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


class CAddress(object):
    def __init__(self):
        self.nServices = 1
        self.pchReserved = b"\x00" * 10 + b"\xff" * 2
        self.ip = "0.0.0.0"
        self.port = 0

    def deserialize(self, f):
        self.nServices = struct.unpack("<Q", f.read(8))[0]
        self.pchReserved = f.read(12)
        self.ip = socket.inet_ntoa(f.read(4))
        self.port = struct.unpack(">H", f.read(2))[0]

    def serialize(self):
        r = b""
        r += struct.pack("<Q", self.nServices)
        r += self.pchReserved
        r += socket.inet_aton(self.ip)
        r += struct.pack(">H", self.port)
        return r

    def __repr__(self):
        return "CAddress(nServices=%i ip=%s port=%i)" % (self.nServices,
                                                         self.ip, self.port)


class CInv(object):
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


class CBlockLocator(object):
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


class COutPoint(object):
    def __init__(self, hash=0, n=0):
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


class CTxIn(object):
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


class CTxOut(object):
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


class CTransaction(object):
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


class CBlockHeader(object):
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

class PrefilledTransaction(object):
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
class P2PHeaderAndShortIDs(object):
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
class HeaderAndShortIDs(object):
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


class BlockTransactionsRequest(object):

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


class BlockTransactions(object):

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


class CPartialMerkleTree(object):
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


class CMerkleBlock(object):
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


class CCbTx(object):
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


class CSimplifiedMNListEntry(object):
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


# Objects that correspond to messages on the wire
class msg_version(object):
    command = b"version"

    def __init__(self):
        self.nVersion = MY_VERSION
        self.nServices = 1
        self.nTime = int(time.time())
        self.addrTo = CAddress()
        self.addrFrom = CAddress()
        self.nNonce = random.getrandbits(64)
        self.strSubVer = MY_SUBVERSION
        self.nStartingHeight = -1
        self.nRelay = MY_RELAY

    def deserialize(self, f):
        self.nVersion = struct.unpack("<i", f.read(4))[0]
        if self.nVersion == 10300:
            self.nVersion = 300
        self.nServices = struct.unpack("<Q", f.read(8))[0]
        self.nTime = struct.unpack("<q", f.read(8))[0]
        self.addrTo = CAddress()
        self.addrTo.deserialize(f)

        if self.nVersion >= 106:
            self.addrFrom = CAddress()
            self.addrFrom.deserialize(f)
            self.nNonce = struct.unpack("<Q", f.read(8))[0]
            self.strSubVer = deser_string(f)
        else:
            self.addrFrom = None
            self.nNonce = None
            self.strSubVer = None
            self.nStartingHeight = None

        if self.nVersion >= 209:
            self.nStartingHeight = struct.unpack("<i", f.read(4))[0]
        else:
            self.nStartingHeight = None

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
        r += self.addrTo.serialize()
        r += self.addrFrom.serialize()
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


class msg_verack(object):
    command = b"verack"

    def __init__(self):
        pass

    def deserialize(self, f):
        pass

    def serialize(self):
        return b""

    def __repr__(self):
        return "msg_verack()"


class msg_addr(object):
    command = b"addr"

    def __init__(self):
        self.addrs = []

    def deserialize(self, f):
        self.addrs = deser_vector(f, CAddress)

    def serialize(self):
        return ser_vector(self.addrs)

    def __repr__(self):
        return "msg_addr(addrs=%s)" % (repr(self.addrs))


class msg_inv(object):
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


class msg_getdata(object):
    command = b"getdata"

    def __init__(self, inv=None):
        self.inv = inv if inv != None else []

    def deserialize(self, f):
        self.inv = deser_vector(f, CInv)

    def serialize(self):
        return ser_vector(self.inv)

    def __repr__(self):
        return "msg_getdata(inv=%s)" % (repr(self.inv))


class msg_getblocks(object):
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


class msg_tx(object):
    command = b"tx"

    def __init__(self, tx=CTransaction()):
        self.tx = tx

    def deserialize(self, f):
        self.tx.deserialize(f)

    def serialize(self):
        return self.tx.serialize()

    def __repr__(self):
        return "msg_tx(tx=%s)" % (repr(self.tx))


class msg_block(object):
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
class msg_generic(object):
    def __init__(self, command, data=None):
        self.command = command
        self.data = data

    def serialize(self):
        return self.data

    def __repr__(self):
        return "msg_generic()"

class msg_getaddr(object):
    command = b"getaddr"

    def __init__(self):
        pass

    def deserialize(self, f):
        pass

    def serialize(self):
        return b""

    def __repr__(self):
        return "msg_getaddr()"


class msg_ping_prebip31(object):
    command = b"ping"

    def __init__(self):
        pass

    def deserialize(self, f):
        pass

    def serialize(self):
        return b""

    def __repr__(self):
        return "msg_ping() (pre-bip31)"


class msg_ping(object):
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


class msg_pong(object):
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


class msg_mempool(object):
    command = b"mempool"

    def __init__(self):
        pass

    def deserialize(self, f):
        pass

    def serialize(self):
        return b""

    def __repr__(self):
        return "msg_mempool()"

class msg_sendheaders(object):
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
class msg_getheaders(object):
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
class msg_headers(object):
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


class msg_reject(object):
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


class msg_sendcmpct(object):
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

class msg_cmpctblock(object):
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

class msg_getblocktxn(object):
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

class msg_blocktxn(object):
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

class msg_getmnlistd(object):
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

class msg_mnlistdiff(object):
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


class msg_clsig(object):
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


class msg_islock(object):
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


class NodeConnCB(object):
    """Callback and helper functions for P2P connection to a bitcoind node.

    Individual testcases should subclass this and override the on_* methods
    if they want to alter message handling behaviour.
    """

    def __init__(self):
        # Track whether we have a P2P connection open to the node
        self.connected = False
        self.connection = None

        # Track number of messages of each type received and the most recent
        # message of each type
        self.message_count = defaultdict(int)
        self.last_message = {}

        # A count of the number of ping messages we've sent to the node
        self.ping_counter = 1

        # deliver_sleep_time is helpful for debugging race conditions in p2p
        # tests; it causes message delivery to sleep for the specified time
        # before acquiring the global lock and delivering the next message.
        self.deliver_sleep_time = None

        # Remember the services our peer has advertised
        self.peer_services = None

    # Message receiving methods

    def deliver(self, conn, message):
        """Receive message and dispatch message to appropriate callback.

        We keep a count of how many of each message type has been received
        and the most recent message of each type.

        Optionally waits for deliver_sleep_time before dispatching message.
        """

        deliver_sleep = self.get_deliver_sleep_time()
        if deliver_sleep is not None:
            time.sleep(deliver_sleep)
        with mininode_lock:
            try:
                command = message.command.decode('ascii')
                self.message_count[command] += 1
                self.last_message[command] = message
                getattr(self, 'on_' + command)(conn, message)
            except:
                print("ERROR delivering %s (%s)" % (repr(message),
                                                    sys.exc_info()[0]))
                raise

    def set_deliver_sleep_time(self, value):
        with mininode_lock:
            self.deliver_sleep_time = value

    def get_deliver_sleep_time(self):
        with mininode_lock:
            return self.deliver_sleep_time

    # Callback methods. Can be overridden by subclasses in individual test
    # cases to provide custom message handling behaviour.

    def on_open(self, conn):
        self.connected = True

    def on_close(self, conn):
        self.connected = False
        self.connection = None

    def on_addr(self, conn, message): pass
    def on_block(self, conn, message): pass
    def on_blocktxn(self, conn, message): pass
    def on_cmpctblock(self, conn, message): pass
    def on_feefilter(self, conn, message): pass
    def on_getaddr(self, conn, message): pass
    def on_getblocks(self, conn, message): pass
    def on_getblocktxn(self, conn, message): pass
    def on_getdata(self, conn, message): pass
    def on_getheaders(self, conn, message): pass
    def on_headers(self, conn, message): pass
    def on_mempool(self, conn): pass
    def on_pong(self, conn, message): pass
    def on_reject(self, conn, message): pass
    def on_sendcmpct(self, conn, message): pass
    def on_sendheaders(self, conn, message): pass
    def on_tx(self, conn, message): pass

    def on_inv(self, conn, message):
        want = msg_getdata()
        for i in message.inv:
            if i.type != 0:
                want.inv.append(i)
        if len(want.inv):
            conn.send_message(want)

    def on_ping(self, conn, message):
        if conn.ver_send > BIP0031_VERSION:
            conn.send_message(msg_pong(message.nonce))

    def on_mnlistdiff(self, conn, message): pass
    def on_clsig(self, conn, message): pass
    def on_islock(self, conn, message): pass

    def on_verack(self, conn, message):
        conn.ver_recv = conn.ver_send
        self.verack_received = True

    def on_version(self, conn, message):
        if message.nVersion >= 209:
            conn.send_message(msg_verack())
        conn.ver_send = min(MY_VERSION, message.nVersion)
        if message.nVersion < 209:
            conn.ver_recv = conn.ver_send
        conn.nServices = message.nServices

    # Connection helper methods

    def add_connection(self, conn):
        self.connection = conn

    def wait_for_disconnect(self, timeout=60):
        test_function = lambda: not self.connected
        wait_until(test_function, timeout=timeout, lock=mininode_lock)

    # Message receiving helper methods

    def wait_for_block(self, blockhash, timeout=60):
        test_function = lambda: self.last_message.get("block") and self.last_message["block"].block.rehash() == blockhash
        wait_until(test_function, timeout=timeout, lock=mininode_lock)

    def wait_for_getdata(self, timeout=60):
        """Waits for a getdata message.

        Receiving any getdata message will satisfy the predicate. the last_message["getdata"]
        value must be explicitly cleared before calling this method, or this will return
        immediately with success. TODO: change this method to take a hash value and only
        return true if the correct block/tx has been requested."""
        test_function = lambda: self.last_message.get("getdata")
        wait_until(test_function, timeout=timeout, lock=mininode_lock)

    def wait_for_getheaders(self, timeout=60):
        """Waits for a getheaders message.

        Receiving any getheaders message will satisfy the predicate. the last_message["getheaders"]
        value must be explicitly cleared before calling this method, or this will return
        immediately with success. TODO: change this method to take a hash value and only
        return true if the correct block header has been requested."""
        test_function = lambda: self.last_message.get("getheaders")
        wait_until(test_function, timeout=timeout, lock=mininode_lock)

    def wait_for_inv(self, expected_inv, timeout=60):
        """Waits for an INV message and checks that the first inv object in the message was as expected."""
        if len(expected_inv) > 1:
            raise NotImplementedError("wait_for_inv() will only verify the first inv object")
        test_function = lambda: self.last_message.get("inv") and \
                                self.last_message["inv"].inv[0].type == expected_inv[0].type and \
                                self.last_message["inv"].inv[0].hash == expected_inv[0].hash
        wait_until(test_function, timeout=timeout, lock=mininode_lock)

    def wait_for_verack(self, timeout=60):
        test_function = lambda: self.message_count["verack"]
        wait_until(test_function, timeout=timeout, lock=mininode_lock)

    # Message sending helper functions

    def send_message(self, message):
        if self.connection:
            self.connection.send_message(message)
        else:
            logger.error("Cannot send message. No connection to node!")

    def send_and_ping(self, message):
        self.send_message(message)
        self.sync_with_ping()

    # Sync up with the node
    def sync_with_ping(self, timeout=60):
        self.send_message(msg_ping(nonce=self.ping_counter))
        test_function = lambda: self.last_message.get("pong") and self.last_message["pong"].nonce == self.ping_counter
        wait_until(test_function, timeout=timeout, lock=mininode_lock)
        self.ping_counter += 1

# The actual NodeConn class
# This class provides an interface for a p2p connection to a specified node
class NodeConn(asyncore.dispatcher):
    messagemap = {
        b"version": msg_version,
        b"verack": msg_verack,
        b"addr": msg_addr,
        b"inv": msg_inv,
        b"getdata": msg_getdata,
        b"getblocks": msg_getblocks,
        b"tx": msg_tx,
        b"block": msg_block,
        b"getaddr": msg_getaddr,
        b"ping": msg_ping,
        b"pong": msg_pong,
        b"headers": msg_headers,
        b"getheaders": msg_getheaders,
        b"reject": msg_reject,
        b"mempool": msg_mempool,
        b"sendheaders": msg_sendheaders,
        b"sendcmpct": msg_sendcmpct,
        b"cmpctblock": msg_cmpctblock,
        b"getblocktxn": msg_getblocktxn,
        b"blocktxn": msg_blocktxn,
        b"mnlistdiff": msg_mnlistdiff,
        b"clsig": msg_clsig,
        b"islock": msg_islock,
        b"notfound": None,
        b"senddsq": None,
        b"qsendrecsigs": None,
        b"getsporks": None,
        b"spork": None,
        b"govsync": None,
        b"qfcommit": None,
    }
    MAGIC_BYTES = {
        "mainnet": b"\xbf\x0c\x6b\xbd",   # mainnet
        "testnet3": b"\xce\xe2\xca\xff",  # testnet3
        "regtest": b"\xfc\xc1\xb7\xdc",   # regtest
        "devnet": b"\xe2\xca\xff\xce",    # devnet
    }

    def __init__(self, dstaddr, dstport, rpc, callback, net="regtest", services=NODE_NETWORK, send_version=True):
        asyncore.dispatcher.__init__(self, map=mininode_socket_map)
        self.dstaddr = dstaddr
        self.dstport = dstport
        self.create_socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sendbuf = b""
        self.recvbuf = b""
        self.ver_send = 209
        self.ver_recv = 209
        self.last_sent = 0
        self.state = "connecting"
        self.network = net
        self.cb = callback
        self.disconnect = False
        self.nServices = 0

        if send_version:
            # stuff version msg into sendbuf
            vt = msg_version()
            vt.nServices = services
            vt.addrTo.ip = self.dstaddr
            vt.addrTo.port = self.dstport
            vt.addrFrom.ip = "0.0.0.0"
            vt.addrFrom.port = 0
            self.send_message(vt, True)

        logger.debug('Connecting to Dash Node: %s:%d' % (self.dstaddr, self.dstport))

        try:
            self.connect((dstaddr, dstport))
        except:
            self.handle_close()
        self.rpc = rpc

    def handle_connect(self):
        if self.state != "connected":
            logger.debug("Connected & Listening: %s:%d" % (self.dstaddr, self.dstport))
            self.state = "connected"
            self.cb.on_open(self)

    def handle_close(self):
        logger.debug("Closing connection to: %s:%d" % (self.dstaddr, self.dstport))
        self.state = "closed"
        self.recvbuf = b""
        self.sendbuf = b""
        try:
            self.close()
        except:
            pass
        self.cb.on_close(self)

    def handle_read(self):
        t = self.recv(8192)
        if len(t) > 0:
            self.recvbuf += t
            self.got_data()

    def readable(self):
        return True

    def writable(self):
        with mininode_lock:
            pre_connection = self.state == "connecting"
            length = len(self.sendbuf)
        return (length > 0 or pre_connection)

    def handle_write(self):
        with mininode_lock:
            # asyncore does not expose socket connection, only the first read/write
            # event, thus we must check connection manually here to know when we
            # actually connect
            if self.state == "connecting":
                self.handle_connect()
            if not self.writable():
                return

            try:
                sent = self.send(self.sendbuf)
            except:
                self.handle_close()
                return
            self.sendbuf = self.sendbuf[sent:]

    def got_data(self):
        try:
            while True:
                if len(self.recvbuf) < 4:
                    return
                if self.recvbuf[:4] != self.MAGIC_BYTES[self.network]:
                    raise ValueError("got garbage %s" % repr(self.recvbuf))
                if self.ver_recv < 209:
                    if len(self.recvbuf) < 4 + 12 + 4:
                        return
                    command = self.recvbuf[4:4+12].split(b"\x00", 1)[0]
                    msglen = struct.unpack("<i", self.recvbuf[4+12:4+12+4])[0]
                    checksum = None
                    if len(self.recvbuf) < 4 + 12 + 4 + msglen:
                        return
                    msg = self.recvbuf[4+12+4:4+12+4+msglen]
                    self.recvbuf = self.recvbuf[4+12+4+msglen:]
                else:
                    if len(self.recvbuf) < 4 + 12 + 4 + 4:
                        return
                    command = self.recvbuf[4:4+12].split(b"\x00", 1)[0]
                    msglen = struct.unpack("<i", self.recvbuf[4+12:4+12+4])[0]
                    checksum = self.recvbuf[4+12+4:4+12+4+4]
                    if len(self.recvbuf) < 4 + 12 + 4 + 4 + msglen:
                        return
                    msg = self.recvbuf[4+12+4+4:4+12+4+4+msglen]
                    th = sha256(msg)
                    h = sha256(th)
                    if checksum != h[:4]:
                        raise ValueError("got bad checksum " + repr(self.recvbuf))
                    self.recvbuf = self.recvbuf[4+12+4+4+msglen:]
                if command in self.messagemap:
                    if self.messagemap[command] is None:
                        # Command is known but we don't want/need to handle it
                        continue
                    f = BytesIO(msg)
                    t = self.messagemap[command]()
                    t.deserialize(f)
                    self.got_message(t)
                else:
                    logger.warning("Received unknown command from %s:%d: '%s' %s" % (self.dstaddr, self.dstport, str(command), repr(msg)))
                    raise ValueError("Unknown command: '%s'" % (command))
        except Exception as e:
            logger.exception('got_data:', repr(e))
            raise

    def send_message(self, message, pushbuf=False):
        if self.state != "connected" and not pushbuf:
            raise IOError('Not connected, no pushbuf')
        self._log_message("send", message)
        command = message.command
        data = message.serialize()
        tmsg = self.MAGIC_BYTES[self.network]
        tmsg += command
        tmsg += b"\x00" * (12 - len(command))
        tmsg += struct.pack("<I", len(data))
        if self.ver_send >= 209:
            th = sha256(data)
            h = sha256(th)
            tmsg += h[:4]
        tmsg += data
        with mininode_lock:
            self.sendbuf += tmsg
            self.last_sent = time.time()

    def got_message(self, message):
        if message.command == b"version":
            if message.nVersion <= BIP0031_VERSION:
                self.messagemap[b'ping'] = msg_ping_prebip31
        if self.last_sent + 30 * 60 < time.time():
            self.send_message(self.messagemap[b'ping']())
        self._log_message("receive", message)
        self.cb.deliver(self, message)

    def _log_message(self, direction, msg):
        if direction == "send":
            log_message = "Send message to "
        elif direction == "receive":
            log_message = "Received message from "
        log_message += "%s:%d: %s" % (self.dstaddr, self.dstport, repr(msg)[:500])
        if len(log_message) > 500:
            log_message += "... (msg truncated)"
        logger.debug(log_message)

    def disconnect_node(self):
        self.disconnect = True


class NetworkThread(threading.Thread):
    def __init__(self):
        super().__init__(name="NetworkThread")

    def run(self):
        while mininode_socket_map:
            # We check for whether to disconnect outside of the asyncore
            # loop to workaround the behavior of asyncore when using
            # select
            disconnected = []
            for fd, obj in mininode_socket_map.items():
                if obj.disconnect:
                    disconnected.append(obj)
            [ obj.handle_close() for obj in disconnected ]
            asyncore.loop(0.1, use_poll=True, map=mininode_socket_map, count=1)

def network_thread_start():
    """Start the network thread."""
    # Only one network thread may run at a time
    assert not network_thread_running()

    NetworkThread().start()

def network_thread_running():
    """Return whether the network thread is running."""
    return any([thread.name == "NetworkThread" for thread in threading.enumerate()])

def network_thread_join(timeout=10):
    """Wait timeout seconds for the network thread to terminate.

    Throw if the network thread doesn't terminate in timeout seconds."""
    network_threads = [thread for thread in threading.enumerate() if thread.name == "NetworkThread"]
    assert len(network_threads) <= 1
    for thread in network_threads:
        thread.join(timeout)
        assert not thread.is_alive()

# An exception we can raise if we detect a potential disconnect
# (p2p or rpc) before the test is complete
class EarlyDisconnectError(Exception):
    def __init__(self, value):
        self.value = value

    def __str__(self):
        return repr(self.value)

class P2PDataStore(NodeConnCB):
    """A P2P data store class.

    Keeps a block and transaction store and responds correctly to getdata and getheaders requests."""

    def __init__(self):
        super().__init__()
        self.reject_code_received = None
        self.reject_reason_received = None
        # store of blocks. key is block hash, value is a CBlock object
        self.block_store = {}
        self.last_block_hash = ''
        # store of txs. key is txid, value is a CTransaction object
        self.tx_store = {}
        self.getdata_requests = []

    def on_getdata(self, conn, message):
        """Check for the tx/block in our stores and if found, reply with an inv message."""
        for inv in message.inv:
            self.getdata_requests.append(inv.hash)
            if (inv.type & MSG_TYPE_MASK) == MSG_TX and inv.hash in self.tx_store.keys():
                self.send_message(msg_tx(self.tx_store[inv.hash]))
            elif (inv.type & MSG_TYPE_MASK) == MSG_BLOCK and inv.hash in self.block_store.keys():
                self.send_message(msg_block(self.block_store[inv.hash]))
            else:
                logger.debug('getdata message type {} received.'.format(hex(inv.type)))

    def on_getheaders(self, conn, message):
        """Search back through our block store for the locator, and reply with a headers message if found."""

        locator, hash_stop = message.locator, message.hashstop

        # Assume that the most recent block added is the tip
        if not self.block_store:
            return

        headers_list = [self.block_store[self.last_block_hash]]
        maxheaders = 2000
        while headers_list[-1].sha256 not in locator.vHave:
            # Walk back through the block store, adding headers to headers_list
            # as we go.
            prev_block_hash = headers_list[-1].hashPrevBlock
            if prev_block_hash in self.block_store:
                prev_block_header = self.block_store[prev_block_hash]
                headers_list.append(prev_block_header)
                if prev_block_header.sha256 == hash_stop:
                    # if this is the hashstop header, stop here
                    break
            else:
                logger.debug('block hash {} not found in block store'.format(hex(prev_block_hash)))
                break

        # Truncate the list if there are too many headers
        headers_list = headers_list[:-maxheaders - 1:-1]
        response = msg_headers(headers_list)

        if response is not None:
            self.send_message(response)

    def on_reject(self, conn, message):
        """Store reject reason and code for testing."""
        self.reject_code_received = message.code
        self.reject_reason_received = message.reason

    def send_blocks_and_test(self, blocks, rpc, success=True, request_block=True, reject_code=None, reject_reason=None, timeout=60):
        """Send blocks to test node and test whether the tip advances.

         - add all blocks to our block_store
         - send a headers message for the final block
         - the on_getheaders handler will ensure that any getheaders are responded to
         - if request_block is True: wait for getdata for each of the blocks. The on_getdata handler will
           ensure that any getdata messages are responded to
         - if success is True: assert that the node's tip advances to the most recent block
         - if success is False: assert that the node's tip doesn't advance
         - if reject_code and reject_reason are set: assert that the correct reject message is received"""

        with mininode_lock:
            self.reject_code_received = None
            self.reject_reason_received = None

            for block in blocks:
                self.block_store[block.sha256] = block
                self.last_block_hash = block.sha256

        self.send_message(msg_headers([blocks[-1]]))

        if request_block:
            wait_until(lambda: blocks[-1].sha256 in self.getdata_requests, timeout=timeout, lock=mininode_lock)

        if success:
            wait_until(lambda: rpc.getbestblockhash() == blocks[-1].hash, timeout=timeout)
        else:
            assert rpc.getbestblockhash() != blocks[-1].hash

        if reject_code is not None:
            wait_until(lambda: self.reject_code_received == reject_code, lock=mininode_lock)
        if reject_reason is not None:
            wait_until(lambda: self.reject_reason_received == reject_reason, lock=mininode_lock)

    def send_txs_and_test(self, txs, rpc, success=True, expect_disconnect=False, reject_code=None, reject_reason=None):
        """Send txs to test node and test whether they're accepted to the mempool.

         - add all txs to our tx_store
         - send tx messages for all txs
         - if success is True/False: assert that the txs are/are not accepted to the mempool
         - if expect_disconnect is True: Skip the sync with ping
         - if reject_code and reject_reason are set: assert that the correct reject message is received."""

        with mininode_lock:
            self.reject_code_received = None
            self.reject_reason_received = None

            for tx in txs:
                self.tx_store[tx.sha256] = tx

        for tx in txs:
            self.send_message(msg_tx(tx))

        if expect_disconnect:
            self.wait_for_disconnect()
        else:
            self.sync_with_ping()

        raw_mempool = rpc.getrawmempool()
        if success:
            # Check that all txs are now in the mempool
            for tx in txs:
                assert tx.hash in raw_mempool, "{} not found in mempool".format(tx.hash)
        else:
            # Check that none of the txs are now in the mempool
            for tx in txs:
                assert tx.hash not in raw_mempool, "{} tx found in mempool".format(tx.hash)

        if reject_code is not None:
            wait_until(lambda: self.reject_code_received == reject_code, lock=mininode_lock)
        if reject_reason is not None:
            wait_until(lambda: self.reject_reason_received == reject_reason, lock=mininode_lock)
