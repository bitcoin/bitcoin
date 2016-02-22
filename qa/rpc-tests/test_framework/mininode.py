# mininode.py - Bitcoin P2P network half-a-node
#
# Distributed under the MIT/X11 software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
#
# This python code was modified from ArtForz' public domain  half-a-node, as
# found in the mini-node branch of http://github.com/jgarzik/pynode.
#
# NodeConn: an object which manages p2p connectivity to a bitcoin node
# NodeConnCB: a base class that describes the interface for receiving
#             callbacks with network messages from a NodeConn
# CBlock, CTransaction, CBlockHeader, CTxIn, CTxOut, etc....:
#     data structures that should map to corresponding structures in
#     bitcoin/primitives
# msg_block, msg_tx, msg_headers, etc.:
#     data structures that represent network messages
# ser_*, deser_*: functions that handle serialization/deserialization


import struct
import socket
import asyncore
import binascii
import time
import sys
import random
import cStringIO
import hashlib
from threading import RLock
from threading import Thread
import logging
import copy

BIP0031_VERSION = 60000
MY_VERSION = 60001  # past bip-31 for ping/pong
MY_SUBVERSION = "/python-mininode-tester:0.0.1/"

MAX_INV_SZ = 50000
MAX_BLOCK_SIZE = 1000000

# Keep our own socket map for asyncore, so that we can track disconnects
# ourselves (to workaround an issue with closing an asyncore socket when 
# using select)
mininode_socket_map = dict()

# One lock for synchronizing all data access between the networking thread (see
# NetworkThread below) and the thread running the test logic.  For simplicity,
# NodeConn acquires this lock whenever delivering a message to to a NodeConnCB,
# and whenever adding anything to the send buffer (in send_message()).  This
# lock should be acquired in the thread running the test logic to synchronize
# access to any data shared with the NodeConnCB or NodeConn.
mininode_lock = RLock()

# Serialization/deserialization tools
def sha256(s):
    return hashlib.new('sha256', s).digest()


def hash256(s):
    return sha256(sha256(s))


def deser_string(f):
    nit = struct.unpack("<B", f.read(1))[0]
    if nit == 253:
        nit = struct.unpack("<H", f.read(2))[0]
    elif nit == 254:
        nit = struct.unpack("<I", f.read(4))[0]
    elif nit == 255:
        nit = struct.unpack("<Q", f.read(8))[0]
    return f.read(nit)


def ser_string(s):
    if len(s) < 253:
        return chr(len(s)) + s
    elif len(s) < 0x10000:
        return chr(253) + struct.pack("<H", len(s)) + s
    elif len(s) < 0x100000000L:
        return chr(254) + struct.pack("<I", len(s)) + s
    return chr(255) + struct.pack("<Q", len(s)) + s


def deser_uint256(f):
    r = 0L
    for i in xrange(8):
        t = struct.unpack("<I", f.read(4))[0]
        r += t << (i * 32)
    return r


def ser_uint256(u):
    rs = ""
    for i in xrange(8):
        rs += struct.pack("<I", u & 0xFFFFFFFFL)
        u >>= 32
    return rs


def uint256_from_str(s):
    r = 0L
    t = struct.unpack("<IIIIIIII", s[:32])
    for i in xrange(8):
        r += t[i] << (i * 32)
    return r


def uint256_from_compact(c):
    nbytes = (c >> 24) & 0xFF
    v = (c & 0xFFFFFFL) << (8 * (nbytes - 3))
    return v


def deser_vector(f, c):
    nit = struct.unpack("<B", f.read(1))[0]
    if nit == 253:
        nit = struct.unpack("<H", f.read(2))[0]
    elif nit == 254:
        nit = struct.unpack("<I", f.read(4))[0]
    elif nit == 255:
        nit = struct.unpack("<Q", f.read(8))[0]
    r = []
    for i in xrange(nit):
        t = c()
        t.deserialize(f)
        r.append(t)
    return r


def ser_vector(l):
    r = ""
    if len(l) < 253:
        r = chr(len(l))
    elif len(l) < 0x10000:
        r = chr(253) + struct.pack("<H", len(l))
    elif len(l) < 0x100000000L:
        r = chr(254) + struct.pack("<I", len(l))
    else:
        r = chr(255) + struct.pack("<Q", len(l))
    for i in l:
        r += i.serialize()
    return r


def deser_uint256_vector(f):
    nit = struct.unpack("<B", f.read(1))[0]
    if nit == 253:
        nit = struct.unpack("<H", f.read(2))[0]
    elif nit == 254:
        nit = struct.unpack("<I", f.read(4))[0]
    elif nit == 255:
        nit = struct.unpack("<Q", f.read(8))[0]
    r = []
    for i in xrange(nit):
        t = deser_uint256(f)
        r.append(t)
    return r


def ser_uint256_vector(l):
    r = ""
    if len(l) < 253:
        r = chr(len(l))
    elif len(l) < 0x10000:
        r = chr(253) + struct.pack("<H", len(l))
    elif len(l) < 0x100000000L:
        r = chr(254) + struct.pack("<I", len(l))
    else:
        r = chr(255) + struct.pack("<Q", len(l))
    for i in l:
        r += ser_uint256(i)
    return r


def deser_string_vector(f):
    nit = struct.unpack("<B", f.read(1))[0]
    if nit == 253:
        nit = struct.unpack("<H", f.read(2))[0]
    elif nit == 254:
        nit = struct.unpack("<I", f.read(4))[0]
    elif nit == 255:
        nit = struct.unpack("<Q", f.read(8))[0]
    r = []
    for i in xrange(nit):
        t = deser_string(f)
        r.append(t)
    return r


def ser_string_vector(l):
    r = ""
    if len(l) < 253:
        r = chr(len(l))
    elif len(l) < 0x10000:
        r = chr(253) + struct.pack("<H", len(l))
    elif len(l) < 0x100000000L:
        r = chr(254) + struct.pack("<I", len(l))
    else:
        r = chr(255) + struct.pack("<Q", len(l))
    for sv in l:
        r += ser_string(sv)
    return r


def deser_int_vector(f):
    nit = struct.unpack("<B", f.read(1))[0]
    if nit == 253:
        nit = struct.unpack("<H", f.read(2))[0]
    elif nit == 254:
        nit = struct.unpack("<I", f.read(4))[0]
    elif nit == 255:
        nit = struct.unpack("<Q", f.read(8))[0]
    r = []
    for i in xrange(nit):
        t = struct.unpack("<i", f.read(4))[0]
        r.append(t)
    return r


def ser_int_vector(l):
    r = ""
    if len(l) < 253:
        r = chr(len(l))
    elif len(l) < 0x10000:
        r = chr(253) + struct.pack("<H", len(l))
    elif len(l) < 0x100000000L:
        r = chr(254) + struct.pack("<I", len(l))
    else:
        r = chr(255) + struct.pack("<Q", len(l))
    for i in l:
        r += struct.pack("<i", i)
    return r


# Objects that map to bitcoind objects, which can be serialized/deserialized

class CAddress(object):
    def __init__(self):
        self.nServices = 1
        self.pchReserved = "\x00" * 10 + "\xff" * 2
        self.ip = "0.0.0.0"
        self.port = 0

    def deserialize(self, f):
        self.nServices = struct.unpack("<Q", f.read(8))[0]
        self.pchReserved = f.read(12)
        self.ip = socket.inet_ntoa(f.read(4))
        self.port = struct.unpack(">H", f.read(2))[0]

    def serialize(self):
        r = ""
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
        2: "Block"}

    def __init__(self, t=0, h=0L):
        self.type = t
        self.hash = h

    def deserialize(self, f):
        self.type = struct.unpack("<i", f.read(4))[0]
        self.hash = deser_uint256(f)

    def serialize(self):
        r = ""
        r += struct.pack("<i", self.type)
        r += ser_uint256(self.hash)
        return r

    def __repr__(self):
        return "CInv(type=%s hash=%064x)" \
            % (self.typemap[self.type], self.hash)


class CBlockLocator(object):
    def __init__(self):
        self.nVersion = MY_VERSION
        self.vHave = []

    def deserialize(self, f):
        self.nVersion = struct.unpack("<i", f.read(4))[0]
        self.vHave = deser_uint256_vector(f)

    def serialize(self):
        r = ""
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
        r = ""
        r += ser_uint256(self.hash)
        r += struct.pack("<I", self.n)
        return r

    def __repr__(self):
        return "COutPoint(hash=%064x n=%i)" % (self.hash, self.n)


class CTxIn(object):
    def __init__(self, outpoint=None, scriptSig="", nSequence=0):
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
        r = ""
        r += self.prevout.serialize()
        r += ser_string(self.scriptSig)
        r += struct.pack("<I", self.nSequence)
        return r

    def __repr__(self):
        return "CTxIn(prevout=%s scriptSig=%s nSequence=%i)" \
            % (repr(self.prevout), binascii.hexlify(self.scriptSig),
               self.nSequence)


class CTxOut(object):
    def __init__(self, nValue=0, scriptPubKey=""):
        self.nValue = nValue
        self.scriptPubKey = scriptPubKey

    def deserialize(self, f):
        self.nValue = struct.unpack("<q", f.read(8))[0]
        self.scriptPubKey = deser_string(f)

    def serialize(self):
        r = ""
        r += struct.pack("<q", self.nValue)
        r += ser_string(self.scriptPubKey)
        return r

    def __repr__(self):
        return "CTxOut(nValue=%i.%08i scriptPubKey=%s)" \
            % (self.nValue // 100000000, self.nValue % 100000000,
               binascii.hexlify(self.scriptPubKey))


class CTransaction(object):
    def __init__(self, tx=None):
        if tx is None:
            self.nVersion = 1
            self.vin = []
            self.vout = []
            self.nLockTime = 0
            self.sha256 = None
            self.hash = None
        else:
            self.nVersion = tx.nVersion
            self.vin = copy.deepcopy(tx.vin)
            self.vout = copy.deepcopy(tx.vout)
            self.nLockTime = tx.nLockTime
            self.sha256 = None
            self.hash = None

    def deserialize(self, f):
        self.nVersion = struct.unpack("<i", f.read(4))[0]
        self.vin = deser_vector(f, CTxIn)
        self.vout = deser_vector(f, CTxOut)
        self.nLockTime = struct.unpack("<I", f.read(4))[0]
        self.sha256 = None
        self.hash = None

    def serialize(self):
        r = ""
        r += struct.pack("<i", self.nVersion)
        r += ser_vector(self.vin)
        r += ser_vector(self.vout)
        r += struct.pack("<I", self.nLockTime)
        return r

    def rehash(self):
        self.sha256 = None
        self.calc_sha256()

    def calc_sha256(self):
        if self.sha256 is None:
            self.sha256 = uint256_from_str(hash256(self.serialize()))
        self.hash = hash256(self.serialize())[::-1].encode('hex_codec')

    def is_valid(self):
        self.calc_sha256()
        for tout in self.vout:
            if tout.nValue < 0 or tout.nValue > 21000000L * 100000000L:
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
        r = ""
        r += struct.pack("<i", self.nVersion)
        r += ser_uint256(self.hashPrevBlock)
        r += ser_uint256(self.hashMerkleRoot)
        r += struct.pack("<I", self.nTime)
        r += struct.pack("<I", self.nBits)
        r += struct.pack("<I", self.nNonce)
        return r

    def calc_sha256(self):
        if self.sha256 is None:
            r = ""
            r += struct.pack("<i", self.nVersion)
            r += ser_uint256(self.hashPrevBlock)
            r += ser_uint256(self.hashMerkleRoot)
            r += struct.pack("<I", self.nTime)
            r += struct.pack("<I", self.nBits)
            r += struct.pack("<I", self.nNonce)
            self.sha256 = uint256_from_str(hash256(r))
            self.hash = hash256(r)[::-1].encode('hex_codec')

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
        r = ""
        r += super(CBlock, self).serialize()
        r += ser_vector(self.vtx)
        return r

    def calc_merkle_root(self):
        hashes = []
        for tx in self.vtx:
            tx.calc_sha256()
            hashes.append(ser_uint256(tx.sha256))
        while len(hashes) > 1:
            newhashes = []
            for i in xrange(0, len(hashes), 2):
                i2 = min(i+1, len(hashes)-1)
                newhashes.append(hash256(hashes[i] + hashes[i2]))
            hashes = newhashes
        return uint256_from_str(hashes[0])

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


class CUnsignedAlert(object):
    def __init__(self):
        self.nVersion = 1
        self.nRelayUntil = 0
        self.nExpiration = 0
        self.nID = 0
        self.nCancel = 0
        self.setCancel = []
        self.nMinVer = 0
        self.nMaxVer = 0
        self.setSubVer = []
        self.nPriority = 0
        self.strComment = ""
        self.strStatusBar = ""
        self.strReserved = ""

    def deserialize(self, f):
        self.nVersion = struct.unpack("<i", f.read(4))[0]
        self.nRelayUntil = struct.unpack("<q", f.read(8))[0]
        self.nExpiration = struct.unpack("<q", f.read(8))[0]
        self.nID = struct.unpack("<i", f.read(4))[0]
        self.nCancel = struct.unpack("<i", f.read(4))[0]
        self.setCancel = deser_int_vector(f)
        self.nMinVer = struct.unpack("<i", f.read(4))[0]
        self.nMaxVer = struct.unpack("<i", f.read(4))[0]
        self.setSubVer = deser_string_vector(f)
        self.nPriority = struct.unpack("<i", f.read(4))[0]
        self.strComment = deser_string(f)
        self.strStatusBar = deser_string(f)
        self.strReserved = deser_string(f)

    def serialize(self):
        r = ""
        r += struct.pack("<i", self.nVersion)
        r += struct.pack("<q", self.nRelayUntil)
        r += struct.pack("<q", self.nExpiration)
        r += struct.pack("<i", self.nID)
        r += struct.pack("<i", self.nCancel)
        r += ser_int_vector(self.setCancel)
        r += struct.pack("<i", self.nMinVer)
        r += struct.pack("<i", self.nMaxVer)
        r += ser_string_vector(self.setSubVer)
        r += struct.pack("<i", self.nPriority)
        r += ser_string(self.strComment)
        r += ser_string(self.strStatusBar)
        r += ser_string(self.strReserved)
        return r

    def __repr__(self):
        return "CUnsignedAlert(nVersion %d, nRelayUntil %d, nExpiration %d, nID %d, nCancel %d, nMinVer %d, nMaxVer %d, nPriority %d, strComment %s, strStatusBar %s, strReserved %s)" \
            % (self.nVersion, self.nRelayUntil, self.nExpiration, self.nID,
               self.nCancel, self.nMinVer, self.nMaxVer, self.nPriority,
               self.strComment, self.strStatusBar, self.strReserved)


class CAlert(object):
    def __init__(self):
        self.vchMsg = ""
        self.vchSig = ""

    def deserialize(self, f):
        self.vchMsg = deser_string(f)
        self.vchSig = deser_string(f)

    def serialize(self):
        r = ""
        r += ser_string(self.vchMsg)
        r += ser_string(self.vchSig)
        return r

    def __repr__(self):
        return "CAlert(vchMsg.sz %d, vchSig.sz %d)" \
            % (len(self.vchMsg), len(self.vchSig))


# Objects that correspond to messages on the wire
class msg_version(object):
    command = "version"

    def __init__(self):
        self.nVersion = MY_VERSION
        self.nServices = 1
        self.nTime = time.time()
        self.addrTo = CAddress()
        self.addrFrom = CAddress()
        self.nNonce = random.getrandbits(64)
        self.strSubVer = MY_SUBVERSION
        self.nStartingHeight = -1

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
            if self.nVersion >= 209:
                self.nStartingHeight = struct.unpack("<i", f.read(4))[0]
            else:
                self.nStartingHeight = None
        else:
            self.addrFrom = None
            self.nNonce = None
            self.strSubVer = None
            self.nStartingHeight = None

    def serialize(self):
        r = ""
        r += struct.pack("<i", self.nVersion)
        r += struct.pack("<Q", self.nServices)
        r += struct.pack("<q", self.nTime)
        r += self.addrTo.serialize()
        r += self.addrFrom.serialize()
        r += struct.pack("<Q", self.nNonce)
        r += ser_string(self.strSubVer)
        r += struct.pack("<i", self.nStartingHeight)
        return r

    def __repr__(self):
        return 'msg_version(nVersion=%i nServices=%i nTime=%s addrTo=%s addrFrom=%s nNonce=0x%016X strSubVer=%s nStartingHeight=%i)' \
            % (self.nVersion, self.nServices, time.ctime(self.nTime),
               repr(self.addrTo), repr(self.addrFrom), self.nNonce,
               self.strSubVer, self.nStartingHeight)


class msg_verack(object):
    command = "verack"

    def __init__(self):
        pass

    def deserialize(self, f):
        pass

    def serialize(self):
        return ""

    def __repr__(self):
        return "msg_verack()"


class msg_addr(object):
    command = "addr"

    def __init__(self):
        self.addrs = []

    def deserialize(self, f):
        self.addrs = deser_vector(f, CAddress)

    def serialize(self):
        return ser_vector(self.addrs)

    def __repr__(self):
        return "msg_addr(addrs=%s)" % (repr(self.addrs))


class msg_alert(object):
    command = "alert"

    def __init__(self):
        self.alert = CAlert()

    def deserialize(self, f):
        self.alert = CAlert()
        self.alert.deserialize(f)

    def serialize(self):
        r = ""
        r += self.alert.serialize()
        return r

    def __repr__(self):
        return "msg_alert(alert=%s)" % (repr(self.alert), )


class msg_inv(object):
    command = "inv"

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
    command = "getdata"

    def __init__(self, inv=None):
        self.inv = inv if inv != None else []

    def deserialize(self, f):
        self.inv = deser_vector(f, CInv)

    def serialize(self):
        return ser_vector(self.inv)

    def __repr__(self):
        return "msg_getdata(inv=%s)" % (repr(self.inv))


class msg_getblocks(object):
    command = "getblocks"

    def __init__(self):
        self.locator = CBlockLocator()
        self.hashstop = 0L

    def deserialize(self, f):
        self.locator = CBlockLocator()
        self.locator.deserialize(f)
        self.hashstop = deser_uint256(f)

    def serialize(self):
        r = ""
        r += self.locator.serialize()
        r += ser_uint256(self.hashstop)
        return r

    def __repr__(self):
        return "msg_getblocks(locator=%s hashstop=%064x)" \
            % (repr(self.locator), self.hashstop)


class msg_tx(object):
    command = "tx"

    def __init__(self, tx=CTransaction()):
        self.tx = tx

    def deserialize(self, f):
        self.tx.deserialize(f)

    def serialize(self):
        return self.tx.serialize()

    def __repr__(self):
        return "msg_tx(tx=%s)" % (repr(self.tx))


class msg_block(object):
    command = "block"

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


class msg_getaddr(object):
    command = "getaddr"

    def __init__(self):
        pass

    def deserialize(self, f):
        pass

    def serialize(self):
        return ""

    def __repr__(self):
        return "msg_getaddr()"


class msg_ping_prebip31(object):
    command = "ping"

    def __init__(self):
        pass

    def deserialize(self, f):
        pass

    def serialize(self):
        return ""

    def __repr__(self):
        return "msg_ping() (pre-bip31)"


class msg_ping(object):
    command = "ping"

    def __init__(self, nonce=0L):
        self.nonce = nonce

    def deserialize(self, f):
        self.nonce = struct.unpack("<Q", f.read(8))[0]

    def serialize(self):
        r = ""
        r += struct.pack("<Q", self.nonce)
        return r

    def __repr__(self):
        return "msg_ping(nonce=%08x)" % self.nonce


class msg_pong(object):
    command = "pong"

    def __init__(self, nonce=0L):
        self.nonce = nonce

    def deserialize(self, f):
        self.nonce = struct.unpack("<Q", f.read(8))[0]

    def serialize(self):
        r = ""
        r += struct.pack("<Q", self.nonce)
        return r

    def __repr__(self):
        return "msg_pong(nonce=%08x)" % self.nonce


class msg_mempool(object):
    command = "mempool"

    def __init__(self):
        pass

    def deserialize(self, f):
        pass

    def serialize(self):
        return ""

    def __repr__(self):
        return "msg_mempool()"

class msg_sendheaders(object):
    command = "sendheaders"

    def __init__(self):
        pass

    def deserialize(self, f):
        pass

    def serialize(self):
        return ""

    def __repr__(self):
        return "msg_sendheaders()"

# getheaders message has
# number of entries
# vector of hashes
# hash_stop (hash of last desired block header, 0 to get as many as possible)
class msg_getheaders(object):
    command = "getheaders"

    def __init__(self):
        self.locator = CBlockLocator()
        self.hashstop = 0L

    def deserialize(self, f):
        self.locator = CBlockLocator()
        self.locator.deserialize(f)
        self.hashstop = deser_uint256(f)

    def serialize(self):
        r = ""
        r += self.locator.serialize()
        r += ser_uint256(self.hashstop)
        return r

    def __repr__(self):
        return "msg_getheaders(locator=%s, stop=%064x)" \
            % (repr(self.locator), self.hashstop)


# headers message has
# <count> <vector of block headers>
class msg_headers(object):
    command = "headers"

    def __init__(self):
        self.headers = []

    def deserialize(self, f):
        # comment in bitcoind indicates these should be deserialized as blocks
        blocks = deser_vector(f, CBlock)
        for x in blocks:
            self.headers.append(CBlockHeader(x))

    def serialize(self):
        blocks = [CBlock(x) for x in self.headers]
        return ser_vector(blocks)

    def __repr__(self):
        return "msg_headers(headers=%s)" % repr(self.headers)


class msg_reject(object):
    command = "reject"

    def __init__(self):
        self.message = ""
        self.code = ""
        self.reason = ""
        self.data = 0L

    def deserialize(self, f):
        self.message = deser_string(f)
        self.code = struct.unpack("<B", f.read(1))[0]
        self.reason = deser_string(f)
        if (self.message == "block" or self.message == "tx"):
            self.data = deser_uint256(f)

    def serialize(self):
        r = ser_string(self.message)
        r += struct.pack("<B", self.code)
        r += ser_string(self.reason)
        if (self.message == "block" or self.message == "tx"):
            r += ser_uint256(self.data)
        return r

    def __repr__(self):
        return "msg_reject: %s %d %s [%064x]" \
            % (self.message, self.code, self.reason, self.data)


# This is what a callback should look like for NodeConn
# Reimplement the on_* functions to provide handling for events
class NodeConnCB(object):
    def __init__(self):
        self.verack_received = False
        # deliver_sleep_time is helpful for debugging race conditions in p2p
        # tests; it causes message delivery to sleep for the specified time
        # before acquiring the global lock and delivering the next message.
        self.deliver_sleep_time = None

    def set_deliver_sleep_time(self, value):
        with mininode_lock:
            self.deliver_sleep_time = value

    def get_deliver_sleep_time(self):
        with mininode_lock:
            return self.deliver_sleep_time

    # Spin until verack message is received from the node.
    # Tests may want to use this as a signal that the test can begin.
    # This can be called from the testing thread, so it needs to acquire the
    # global lock.
    def wait_for_verack(self):
        while True:
            with mininode_lock:
                if self.verack_received:
                    return
            time.sleep(0.05)

    def deliver(self, conn, message):
        deliver_sleep = self.get_deliver_sleep_time()
        if deliver_sleep is not None:
            time.sleep(deliver_sleep)
        with mininode_lock:
            try:
                getattr(self, 'on_' + message.command)(conn, message)
            except:
                print "ERROR delivering %s (%s)" % (repr(message),
                                                    sys.exc_info()[0])

    def on_version(self, conn, message):
        if message.nVersion >= 209:
            conn.send_message(msg_verack())
        conn.ver_send = min(MY_VERSION, message.nVersion)
        if message.nVersion < 209:
            conn.ver_recv = conn.ver_send

    def on_verack(self, conn, message):
        conn.ver_recv = conn.ver_send
        self.verack_received = True

    def on_inv(self, conn, message):
        want = msg_getdata()
        for i in message.inv:
            if i.type != 0:
                want.inv.append(i)
        if len(want.inv):
            conn.send_message(want)

    def on_addr(self, conn, message): pass
    def on_alert(self, conn, message): pass
    def on_getdata(self, conn, message): pass
    def on_getblocks(self, conn, message): pass
    def on_tx(self, conn, message): pass
    def on_block(self, conn, message): pass
    def on_getaddr(self, conn, message): pass
    def on_headers(self, conn, message): pass
    def on_getheaders(self, conn, message): pass
    def on_ping(self, conn, message):
        if conn.ver_send > BIP0031_VERSION:
            conn.send_message(msg_pong(message.nonce))
    def on_reject(self, conn, message): pass
    def on_close(self, conn): pass
    def on_mempool(self, conn): pass
    def on_pong(self, conn, message): pass


# The actual NodeConn class
# This class provides an interface for a p2p connection to a specified node
class NodeConn(asyncore.dispatcher):
    messagemap = {
        "version": msg_version,
        "verack": msg_verack,
        "addr": msg_addr,
        "alert": msg_alert,
        "inv": msg_inv,
        "getdata": msg_getdata,
        "getblocks": msg_getblocks,
        "tx": msg_tx,
        "block": msg_block,
        "getaddr": msg_getaddr,
        "ping": msg_ping,
        "pong": msg_pong,
        "headers": msg_headers,
        "getheaders": msg_getheaders,
        "reject": msg_reject,
        "mempool": msg_mempool
    }
    MAGIC_BYTES = {
        "mainnet": "\xf9\xbe\xb4\xd9",   # mainnet
        "testnet3": "\x0b\x11\x09\x07",  # testnet3
        "regtest": "\xfa\xbf\xb5\xda"    # regtest
    }

    def __init__(self, dstaddr, dstport, rpc, callback, net="regtest", services=1):
        asyncore.dispatcher.__init__(self, map=mininode_socket_map)
        self.log = logging.getLogger("NodeConn(%s:%d)" % (dstaddr, dstport))
        self.dstaddr = dstaddr
        self.dstport = dstport
        self.create_socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sendbuf = ""
        self.recvbuf = ""
        self.ver_send = 209
        self.ver_recv = 209
        self.last_sent = 0
        self.state = "connecting"
        self.network = net
        self.cb = callback
        self.disconnect = False

        # stuff version msg into sendbuf
        vt = msg_version()
        vt.nServices = services
        vt.addrTo.ip = self.dstaddr
        vt.addrTo.port = self.dstport
        vt.addrFrom.ip = "0.0.0.0"
        vt.addrFrom.port = 0
        self.send_message(vt, True)
        print 'MiniNode: Connecting to Bitcoin Node IP # ' + dstaddr + ':' \
            + str(dstport)

        try:
            self.connect((dstaddr, dstport))
        except:
            self.handle_close()
        self.rpc = rpc

    def show_debug_msg(self, msg):
        self.log.debug(msg)

    def handle_connect(self):
        self.show_debug_msg("MiniNode: Connected & Listening: \n")
        self.state = "connected"

    def handle_close(self):
        self.show_debug_msg("MiniNode: Closing Connection to %s:%d... "
                            % (self.dstaddr, self.dstport))
        self.state = "closed"
        self.recvbuf = ""
        self.sendbuf = ""
        try:
            self.close()
        except:
            pass
        self.cb.on_close(self)

    def handle_read(self):
        try:
            t = self.recv(8192)
            if len(t) > 0:
                self.recvbuf += t
                self.got_data()
        except:
            pass

    def readable(self):
        return True

    def writable(self):
        with mininode_lock:
            length = len(self.sendbuf)
        return (length > 0)

    def handle_write(self):
        with mininode_lock:
            try:
                sent = self.send(self.sendbuf)
            except:
                self.handle_close()
                return
            self.sendbuf = self.sendbuf[sent:]

    def got_data(self):
        while True:
            if len(self.recvbuf) < 4:
                return
            if self.recvbuf[:4] != self.MAGIC_BYTES[self.network]:
                raise ValueError("got garbage %s" % repr(self.recvbuf))
            if self.ver_recv < 209:
                if len(self.recvbuf) < 4 + 12 + 4:
                    return
                command = self.recvbuf[4:4+12].split("\x00", 1)[0]
                msglen = struct.unpack("<i", self.recvbuf[4+12:4+12+4])[0]
                checksum = None
                if len(self.recvbuf) < 4 + 12 + 4 + msglen:
                    return
                msg = self.recvbuf[4+12+4:4+12+4+msglen]
                self.recvbuf = self.recvbuf[4+12+4+msglen:]
            else:
                if len(self.recvbuf) < 4 + 12 + 4 + 4:
                    return
                command = self.recvbuf[4:4+12].split("\x00", 1)[0]
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
                f = cStringIO.StringIO(msg)
                t = self.messagemap[command]()
                t.deserialize(f)
                self.got_message(t)
            else:
                self.show_debug_msg("Unknown command: '" + command + "' " +
                                    repr(msg))

    def send_message(self, message, pushbuf=False):
        if self.state != "connected" and not pushbuf:
            return
        self.show_debug_msg("Send %s" % repr(message))
        command = message.command
        data = message.serialize()
        tmsg = self.MAGIC_BYTES[self.network]
        tmsg += command
        tmsg += "\x00" * (12 - len(command))
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
        if message.command == "version":
            if message.nVersion <= BIP0031_VERSION:
                self.messagemap['ping'] = msg_ping_prebip31
        if self.last_sent + 30 * 60 < time.time():
            self.send_message(self.messagemap['ping']())
        self.show_debug_msg("Recv %s" % repr(message))
        self.cb.deliver(self, message)

    def disconnect_node(self):
        self.disconnect = True


class NetworkThread(Thread):
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


# An exception we can raise if we detect a potential disconnect
# (p2p or rpc) before the test is complete
class EarlyDisconnectError(Exception):
    def __init__(self, value):
        self.value = value

    def __str__(self):
        return repr(self.value)
