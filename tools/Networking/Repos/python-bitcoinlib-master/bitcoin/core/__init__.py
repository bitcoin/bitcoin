# Copyright (C) 2012-2017 The python-bitcoinlib developers
#
# This file is part of python-bitcoinlib.
#
# It is subject to the license terms in the LICENSE file found in the top-level
# directory of this distribution.
#
# No part of python-bitcoinlib, including this file, may be copied, modified,
# propagated, or distributed except according to the terms contained in the
# LICENSE file.

from __future__ import absolute_import, division, print_function

import binascii
import struct
import sys
import time

from .script import CScript, CScriptWitness, CScriptOp, OP_RETURN

from .serialize import *

if sys.version > '3':
    _bytes = bytes
else:
    _bytes = lambda x: bytes(bytearray(x))

# Core definitions
COIN = 100000000
MAX_BLOCK_SIZE = 1000000
MAX_BLOCK_WEIGHT = 4000000
MAX_BLOCK_SIGOPS = MAX_BLOCK_SIZE/50
WITNESS_COINBASE_SCRIPTPUBKEY_MAGIC = _bytes([OP_RETURN, 0x24, 0xaa, 0x21, 0xa9, 0xed])

def MoneyRange(nValue, params=None):
    global coreparams
    if not params:
      params = coreparams

    return 0 <= nValue <= params.MAX_MONEY

def _py2_x(h):
    """Convert a hex string to bytes"""
    return binascii.unhexlify(h)

def x(h):
    """Convert a hex string to bytes"""
    return binascii.unhexlify(h.encode('utf8'))

def _py2_b2x(b):
    """Convert bytes to a hex string"""
    return binascii.hexlify(b)

def b2x(b):
    """Convert bytes to a hex string"""
    return binascii.hexlify(b).decode('utf8')

def _py2_lx(h):
    """Convert a little-endian hex string to bytes

    Lets you write uint256's and uint160's the way the Satoshi codebase shows
    them.
    """
    return binascii.unhexlify(h)[::-1]

def lx(h):
    """Convert a little-endian hex string to bytes

    Lets you write uint256's and uint160's the way the Satoshi codebase shows
    them.
    """
    return binascii.unhexlify(h.encode('utf8'))[::-1]

def _py2_b2lx(b):
    """Convert bytes to a little-endian hex string

    Lets you show uint256's and uint160's the way the Satoshi codebase shows
    them.
    """
    return binascii.hexlify(b[::-1])

def b2lx(b):
    """Convert bytes to a little-endian hex string

    Lets you show uint256's and uint160's the way the Satoshi codebase shows
    them.
    """
    return binascii.hexlify(b[::-1]).decode('utf8')

if not (sys.version > '3'):
    x = _py2_x
    b2x = _py2_b2x
    lx = _py2_lx
    b2lx = _py2_b2lx

del _py2_x
del _py2_b2x
del _py2_lx
del _py2_b2lx


def str_money_value(value):
    """Convert an integer money value to a fixed point string"""
    r = '%i.%08i' % (value // COIN, value % COIN)
    r = r.rstrip('0')
    if r[-1] == '.':
        r += '0'
    return r


class ValidationError(Exception):
    """Base class for all blockchain validation errors

    Everything that is related to validating the blockchain, blocks,
    transactions, scripts, etc. is derived from this class.
    """

def __make_mutable(cls):
    # For speed we use a class decorator that removes the immutable
    # restrictions directly. In addition the modified behavior of GetHash() and
    # hash() is undone.
    cls.__setattr__ = object.__setattr__
    cls.__delattr__ = object.__delattr__
    cls.GetHash = Serializable.GetHash
    cls.__hash__ = Serializable.__hash__
    return cls


class COutPoint(ImmutableSerializable):
    """The combination of a transaction hash and an index n into its vout"""
    __slots__ = ['hash', 'n']

    def __init__(self, hash=b'\x00'*32, n=0xffffffff):
        if not len(hash) == 32:
            raise ValueError('COutPoint: hash must be exactly 32 bytes; got %d bytes' % len(hash))
        object.__setattr__(self, 'hash', hash)
        if not (0 <= n <= 0xffffffff):
            raise ValueError('COutPoint: n must be in range 0x0 to 0xffffffff; got %x' % n)
        object.__setattr__(self, 'n', n)

    @classmethod
    def stream_deserialize(cls, f):
        hash = ser_read(f,32)
        n = struct.unpack(b"<I", ser_read(f,4))[0]
        return cls(hash, n)

    def stream_serialize(self, f):
        assert len(self.hash) == 32
        f.write(self.hash)
        f.write(struct.pack(b"<I", self.n))

    def is_null(self):
        return ((self.hash == b'\x00'*32) and (self.n == 0xffffffff))

    def __repr__(self):
        if self.is_null():
            return 'COutPoint()'
        else:
            return 'COutPoint(lx(%r), %i)' % (b2lx(self.hash), self.n)

    def __str__(self):
        return '%s:%i' % (b2lx(self.hash), self.n)

    @classmethod
    def from_outpoint(cls, outpoint):
        """Create an immutable copy of an existing OutPoint

        If outpoint is already immutable (outpoint.__class__ is COutPoint) it is
        returned directly.
        """
        if outpoint.__class__ is COutPoint:
            return outpoint

        else:
            return cls(outpoint.hash, outpoint.n)

@__make_mutable
class CMutableOutPoint(COutPoint):
    """A mutable COutPoint"""
    __slots__ = []

    @classmethod
    def from_outpoint(cls, outpoint):
        """Create a mutable copy of an existing COutPoint"""
        return cls(outpoint.hash, outpoint.n)

class CTxIn(ImmutableSerializable):
    """An input of a transaction

    Contains the location of the previous transaction's output that it claims,
    and a signature that matches the output's public key.
    """
    __slots__ = ['prevout', 'scriptSig', 'nSequence']

    def __init__(self, prevout=COutPoint(), scriptSig=CScript(), nSequence = 0xffffffff):
        if not (0 <= nSequence <= 0xffffffff):
            raise ValueError('CTxIn: nSequence must be an integer between 0x0 and 0xffffffff; got %x' % nSequence)
        object.__setattr__(self, 'nSequence', nSequence)

        object.__setattr__(self, 'prevout', prevout)
        object.__setattr__(self, 'scriptSig', scriptSig)

    @classmethod
    def stream_deserialize(cls, f):
        prevout = COutPoint.stream_deserialize(f)
        scriptSig = script.CScript(BytesSerializer.stream_deserialize(f))
        nSequence = struct.unpack(b"<I", ser_read(f,4))[0]
        return cls(prevout, scriptSig, nSequence)

    def stream_serialize(self, f):
        COutPoint.stream_serialize(self.prevout, f)
        BytesSerializer.stream_serialize(self.scriptSig, f)
        f.write(struct.pack(b"<I", self.nSequence))

    def is_final(self):
        return (self.nSequence == 0xffffffff)

    def __repr__(self):
        return "CTxIn(%s, %s, 0x%x)" % (repr(self.prevout), repr(self.scriptSig), self.nSequence)

    @classmethod
    def from_txin(cls, txin):
        """Create an immutable copy of an existing TxIn

        If txin is already immutable (txin.__class__ is CTxIn) it is returned
        directly.
        """
        if txin.__class__ is CTxIn:
            return txin

        else:
            return cls(COutPoint.from_outpoint(txin.prevout), txin.scriptSig, txin.nSequence)

@__make_mutable
class CMutableTxIn(CTxIn):
    """A mutable CTxIn"""
    __slots__ = []

    def __init__(self, prevout=None, scriptSig=CScript(), nSequence = 0xffffffff):
        if not (0 <= nSequence <= 0xffffffff):
            raise ValueError('CTxIn: nSequence must be an integer between 0x0 and 0xffffffff; got %x' % nSequence)
        self.nSequence = nSequence

        if prevout is None:
            prevout = CMutableOutPoint()
        self.prevout = prevout
        self.scriptSig = scriptSig

    @classmethod
    def from_txin(cls, txin):
        """Create a fully mutable copy of an existing TxIn"""
        prevout = CMutableOutPoint.from_outpoint(txin.prevout)
        return cls(prevout, txin.scriptSig, txin.nSequence)


class CTxOut(ImmutableSerializable):
    """An output of a transaction

    Contains the public key that the next input must be able to sign with to
    claim it.
    """
    __slots__ = ['nValue', 'scriptPubKey']

    def __init__(self, nValue=-1, scriptPubKey=script.CScript()):
        object.__setattr__(self, 'nValue', int(nValue))
        object.__setattr__(self, 'scriptPubKey', scriptPubKey)

    @classmethod
    def stream_deserialize(cls, f):
        nValue = struct.unpack(b"<q", ser_read(f,8))[0]
        scriptPubKey = script.CScript(BytesSerializer.stream_deserialize(f))
        return cls(nValue, scriptPubKey)

    def stream_serialize(self, f):
        f.write(struct.pack(b"<q", self.nValue))
        BytesSerializer.stream_serialize(self.scriptPubKey, f)

    def is_valid(self):
        if not MoneyRange(self.nValue):
            return False
        if not self.scriptPubKey.is_valid():
            return False
        return True

    def __repr__(self):
        if self.nValue >= 0:
            return "CTxOut(%s*COIN, %r)" % (str_money_value(self.nValue), self.scriptPubKey)
        else:
            return "CTxOut(%d, %r)" % (self.nValue, self.scriptPubKey)

    @classmethod
    def from_txout(cls, txout):
        """Create an immutable copy of an existing TxOut

        If txout is already immutable (txout.__class__ is CTxOut) then it will
        be returned directly.
        """
        if txout.__class__ is CTxOut:
            return txout

        else:
            return cls(txout.nValue, txout.scriptPubKey)

@__make_mutable
class CMutableTxOut(CTxOut):
    """A mutable CTxOut"""
    __slots__ = []

    @classmethod
    def from_txout(cls, txout):
        """Create a fullly mutable copy of an existing TxOut"""
        return cls(txout.nValue, txout.scriptPubKey)


class CTxInWitness(ImmutableSerializable):
    """Witness data for a single transaction input"""
    __slots__ = ['scriptWitness']

    def __init__(self, scriptWitness=CScriptWitness()):
        object.__setattr__(self, 'scriptWitness', scriptWitness)

    def is_null(self):
        return self.scriptWitness.is_null()

    @classmethod
    def stream_deserialize(cls, f):
        scriptWitness = CScriptWitness.stream_deserialize(f)
        return cls(scriptWitness)

    def stream_serialize(self, f):
        self.scriptWitness.stream_serialize(f)

    def __repr__(self):
        return "CTxInWitness(%s)" % (repr(self.scriptWitness))

    @classmethod
    def from_txinwitness(cls, txinwitness):
        """Create an immutable copy of an existing TxInWitness

        If txin is already immutable (txin.__class__ is CTxIn) it is returned
        directly.
        """
        if txinwitness.__class__ is CTxInWitness:
            return txinwitness

        else:
            return cls(txinwitness.scriptWitness)

class CTxWitness(ImmutableSerializable):
    """Witness data for all inputs to a transaction"""
    __slots__ = ['vtxinwit']

    def __init__(self, vtxinwit=()):
        object.__setattr__(self, 'vtxinwit', vtxinwit)

    def is_null(self):
        for n in range(len(self.vtxinwit)):
            if not self.vtxinwit[n].is_null(): return False
        return True

    # FIXME this cannot be a @classmethod like the others because we need to
    # know how many items to deserialize, which comes from len(vin)
    def stream_deserialize(self, f):
        vtxinwit = tuple(CTxInWitness.stream_deserialize(f) for dummy in
                range(len(self.vtxinwit)))
        return CTxWitness(vtxinwit)

    def stream_serialize(self, f):
        for i in range(len(self.vtxinwit)):
            self.vtxinwit[i].stream_serialize(f)

    def __repr__(self):
        return "CTxWitness(%s)" % (','.join(repr(w) for w in self.vtxinwit))

    @classmethod
    def from_txwitness(cls, txwitness):
        """Create an immutable copy of an existing TxWitness

        If txwitness is already immutable (txwitness.__class__ is CTxWitness) it is returned
        directly.
        """
        if txwitness.__class__ is CTxWitness:
            return txwitness
        else:
            return cls(txwitness.vtxinwit)


class CTransaction(ImmutableSerializable):
    """A transaction"""
    __slots__ = ['nVersion', 'vin', 'vout', 'nLockTime', 'wit']

    def __init__(self, vin=(), vout=(), nLockTime=0, nVersion=1, witness=CTxWitness()):
        """Create a new transaction

        vin and vout are iterables of transaction inputs and outputs
        respectively. If their contents are not already immutable, immutable
        copies will be made.
        """
        if not (0 <= nLockTime <= 0xffffffff):
            raise ValueError('CTransaction: nLockTime must be in range 0x0 to 0xffffffff; got %x' % nLockTime)
        object.__setattr__(self, 'nLockTime', nLockTime)
        object.__setattr__(self, 'nVersion', nVersion)
        object.__setattr__(self, 'vin', tuple(CTxIn.from_txin(txin) for txin in vin))
        object.__setattr__(self, 'vout', tuple(CTxOut.from_txout(txout) for txout in vout))
        object.__setattr__(self, 'wit', CTxWitness.from_txwitness(witness))

    @classmethod
    def stream_deserialize(cls, f):
        """Deserialize transaction

        This implementation corresponds to Bitcoin's SerializeTransaction() and
        consensus behavior. Note that Bitcoin's DecodeHexTx() also has the
        option to attempt deserializing as a non-witness transaction first,
        falling back to the consensus behavior if it fails. The difference lies
        in transactions which have zero inputs: they are invalid but may be
        (de)serialized anyway for the purpose of signing them and adding
        inputs. If the behavior of DecodeHexTx() is needed it could be added,
        but not here.
        """
        # FIXME can't assume f is seekable
        nVersion = struct.unpack(b"<i", ser_read(f,4))[0]
        pos = f.tell()
        markerbyte = struct.unpack(b'B', ser_read(f, 1))[0]
        flagbyte = struct.unpack(b'B', ser_read(f, 1))[0]
        if markerbyte == 0 and flagbyte == 1:
            vin = VectorSerializer.stream_deserialize(CTxIn, f)
            vout = VectorSerializer.stream_deserialize(CTxOut, f)
            wit = CTxWitness(tuple(0 for dummy in range(len(vin))))
            wit = wit.stream_deserialize(f)
            nLockTime = struct.unpack(b"<I", ser_read(f,4))[0]
            return cls(vin, vout, nLockTime, nVersion, wit)
        else:
            f.seek(pos) # put marker byte back, since we don't have peek
            vin = VectorSerializer.stream_deserialize(CTxIn, f)
            vout = VectorSerializer.stream_deserialize(CTxOut, f)
            nLockTime = struct.unpack(b"<I", ser_read(f,4))[0]
            return cls(vin, vout, nLockTime, nVersion)


    def stream_serialize(self, f, include_witness=True):
        f.write(struct.pack(b"<i", self.nVersion))
        if include_witness and not self.wit.is_null():
            assert(len(self.wit.vtxinwit) <= len(self.vin))
            f.write(b'\x00') # Marker
            f.write(b'\x01') # Flag
            VectorSerializer.stream_serialize(CTxIn, self.vin, f)
            VectorSerializer.stream_serialize(CTxOut, self.vout, f)
            self.wit.stream_serialize(f)
        else:
            VectorSerializer.stream_serialize(CTxIn, self.vin, f)
            VectorSerializer.stream_serialize(CTxOut, self.vout, f)
        f.write(struct.pack(b"<I", self.nLockTime))

    def is_coinbase(self):
        return len(self.vin) == 1 and self.vin[0].prevout.is_null()

    def has_witness(self):
        """True if witness"""
        return not self.wit.is_null()

    def __repr__(self):
        return "CTransaction(%r, %r, %i, %i, %r)" % (self.vin, self.vout,
                self.nLockTime, self.nVersion, self.wit)

    @classmethod
    def from_tx(cls, tx):
        """Create an immutable copy of a pre-existing transaction

        If tx is already immutable (tx.__class__ is CTransaction) then it will
        be returned directly.
        """
        if tx.__class__ is CTransaction:
            return tx

        else:
            return cls(tx.vin, tx.vout, tx.nLockTime, tx.nVersion, tx.wit)

    def GetTxid(self):
        """Get the transaction ID.  This differs from the transactions hash as
            given by GetHash.  GetTxid excludes witness data, while GetHash
            includes it. """
        if self.wit != CTxWitness():
            txid = Hash(CTransaction(self.vin, self.vout, self.nLockTime,
                self.nVersion).serialize())
        else:
            txid = Hash(self.serialize())
        return txid

@__make_mutable
class CMutableTransaction(CTransaction):
    """A mutable transaction"""
    __slots__ = []

    def __init__(self, vin=None, vout=None, nLockTime=0, nVersion=1, witness=None):
        if not (0 <= nLockTime <= 0xffffffff):
            raise ValueError('CTransaction: nLockTime must be in range 0x0 to 0xffffffff; got %x' % nLockTime)
        self.nLockTime = nLockTime

        if vin is None:
            vin = []
        self.vin = vin

        if vout is None:
            vout = []
        self.vout = vout
        self.nVersion = nVersion

        if witness is None:
            witness = CTxWitness([CTxInWitness() for dummy in range(len(vin))])
        self.wit = witness

    @classmethod
    def from_tx(cls, tx):
        """Create a fully mutable copy of a pre-existing transaction"""
        vin = [CMutableTxIn.from_txin(txin) for txin in tx.vin]
        vout = [CMutableTxOut.from_txout(txout) for txout in tx.vout]

        return cls(vin, vout, tx.nLockTime, tx.nVersion, tx.wit)


class CBlockHeader(ImmutableSerializable):
    """A block header"""
    __slots__ = ['nVersion', 'hashPrevBlock', 'hashMerkleRoot', 'nTime', 'nBits', 'nNonce']

    def __init__(self, nVersion=2, hashPrevBlock=b'\x00'*32, hashMerkleRoot=b'\x00'*32, nTime=0, nBits=0, nNonce=0):
        object.__setattr__(self, 'nVersion', nVersion)
        assert len(hashPrevBlock) == 32
        object.__setattr__(self, 'hashPrevBlock', hashPrevBlock)
        assert len(hashMerkleRoot) == 32
        object.__setattr__(self, 'hashMerkleRoot', hashMerkleRoot)
        object.__setattr__(self, 'nTime', nTime)
        object.__setattr__(self, 'nBits', nBits)
        object.__setattr__(self, 'nNonce', nNonce)

    @classmethod
    def stream_deserialize(cls, f):
        nVersion = struct.unpack(b"<i", ser_read(f,4))[0]
        hashPrevBlock = ser_read(f,32)
        hashMerkleRoot = ser_read(f,32)
        nTime = struct.unpack(b"<I", ser_read(f,4))[0]
        nBits = struct.unpack(b"<I", ser_read(f,4))[0]
        nNonce = struct.unpack(b"<I", ser_read(f,4))[0]
        return cls(nVersion, hashPrevBlock, hashMerkleRoot, nTime, nBits, nNonce)

    def stream_serialize(self, f):
        f.write(struct.pack(b"<i", self.nVersion))
        assert len(self.hashPrevBlock) == 32
        f.write(self.hashPrevBlock)
        assert len(self.hashMerkleRoot) == 32
        f.write(self.hashMerkleRoot)
        f.write(struct.pack(b"<I", self.nTime))
        f.write(struct.pack(b"<I", self.nBits))
        f.write(struct.pack(b"<I", self.nNonce))

    @staticmethod
    def calc_difficulty(nBits):
        """Calculate difficulty from nBits target"""
        nShift = (nBits >> 24) & 0xff
        dDiff = float(0x0000ffff) / float(nBits & 0x00ffffff)
        while nShift < 29:
            dDiff *= 256.0
            nShift += 1
        while nShift > 29:
            dDiff /= 256.0
            nShift -= 1
        return dDiff
    difficulty = property(lambda self: CBlockHeader.calc_difficulty(self.nBits))

    def __repr__(self):
        return "%s(%i, lx(%s), lx(%s), %s, 0x%08x, 0x%08x)" % \
                (self.__class__.__name__, self.nVersion, b2lx(self.hashPrevBlock), b2lx(self.hashMerkleRoot),
                 self.nTime, self.nBits, self.nNonce)

class NoWitnessData(Exception):
    """The block does not have witness data"""

class CBlock(CBlockHeader):
    """A block including all transactions in it"""
    __slots__ = ['vtx', 'vMerkleTree', 'vWitnessMerkleTree']

    @staticmethod
    def build_merkle_tree_from_txids(txids):
        """Build a full CBlock merkle tree from txids

        txids - iterable of txids

        Returns a new merkle tree in deepest first order. The last element is
        the merkle root.

        WARNING! If you're reading this because you're learning about crypto
        and/or designing a new system that will use merkle trees, keep in mind
        that the following merkle tree algorithm has a serious flaw related to
        duplicate txids, resulting in a vulnerability. (CVE-2012-2459) Bitcoin
        has since worked around the flaw, but for new applications you should
        use something different; don't just copy-and-paste this code without
        understanding the problem first.
        """
        merkle_tree = list(txids)

        size = len(txids)
        j = 0
        while size > 1:
            for i in range(0, size, 2):
                i2 = min(i+1, size-1)
                merkle_tree.append(Hash(merkle_tree[j+i] + merkle_tree[j+i2]))

            j += size
            size = (size + 1) // 2

        return merkle_tree

    @staticmethod
    def build_merkle_tree_from_txs(txs):
        """Build a full merkle tree from transactions"""
        txids = [tx.GetTxid() for tx in txs]
        return CBlock.build_merkle_tree_from_txids(txids)

    def calc_merkle_root(self):
        """Calculate the merkle root

        The calculated merkle root is not cached; every invocation
        re-calculates it from scratch.
        """
        if not len(self.vtx):
            raise ValueError('Block contains no transactions')
        return self.build_merkle_tree_from_txs(self.vtx)[-1]

    @staticmethod
    def build_witness_merkle_tree_from_txs(txs):
        """Calculate the witness merkle tree from transactions"""
        has_witness = False
        hashes = []
        for tx in txs:
            hashes.append(tx.GetHash())
            has_witness |= tx.has_witness()
        if not has_witness:
            raise NoWitnessData
        hashes[0] = b'\x00' * 32
        return CBlock.build_merkle_tree_from_txids(hashes)

    def calc_witness_merkle_root(self):
        """Calculate the witness merkle root

        The calculated merkle root is not cached; every invocation
        re-calculates it from scratch.
        """
        if not len(self.vtx):
            raise ValueError('Block contains no transactions')
        return self.build_witness_merkle_tree_from_txs(self.vtx)[-1]

    def get_witness_commitment_index(self):
        """Find txout # of witness commitment in coinbase

        Return None or an index
        """
        if not len(self.vtx):
            raise ValueError('Block contains no transactions')
        commit_pos = None
        for index, out in enumerate(self.vtx[0].vout):
            script = out.scriptPubKey
            if len(script) >= 38 and script[:6] == WITNESS_COINBASE_SCRIPTPUBKEY_MAGIC:
                commit_pos = index
        if commit_pos is None:
            raise ValueError('The witness commitment is missed')
        return commit_pos

    def __init__(self, nVersion=2, hashPrevBlock=b'\x00'*32, hashMerkleRoot=b'\x00'*32, nTime=0, nBits=0, nNonce=0, vtx=()):
        """Create a new block"""
        if vtx:
            vMerkleTree = tuple(CBlock.build_merkle_tree_from_txs(vtx))
            if hashMerkleRoot == b'\x00'*32:
                hashMerkleRoot = vMerkleTree[-1]
            elif hashMerkleRoot != vMerkleTree[-1]:
                raise CheckBlockError("CBlock : hashMerkleRoot is not compatible with vtx")
        else:
            vMerkleTree = ()
        super(CBlock, self).__init__(nVersion, hashPrevBlock, hashMerkleRoot, nTime, nBits, nNonce)

        object.__setattr__(self, 'vMerkleTree', vMerkleTree)
        try:
            vWitnessMerkleTree = tuple(CBlock.build_witness_merkle_tree_from_txs(vtx))
        except NoWitnessData:
            vWitnessMerkleTree = ()
        object.__setattr__(self, 'vWitnessMerkleTree', vWitnessMerkleTree)
        object.__setattr__(self, 'vtx', tuple(CTransaction.from_tx(tx) for tx in vtx))

    @classmethod
    def stream_deserialize(cls, f):
        self = super(CBlock, cls).stream_deserialize(f)

        vtx = VectorSerializer.stream_deserialize(CTransaction, f)
        vMerkleTree = tuple(CBlock.build_merkle_tree_from_txs(vtx))
        object.__setattr__(self, 'vMerkleTree', vMerkleTree)
        try:
            vWitnessMerkleTree = tuple(CBlock.build_witness_merkle_tree_from_txs(vtx))
        except NoWitnessData:
            vWitnessMerkleTree = ()
        object.__setattr__(self, 'vWitnessMerkleTree', vWitnessMerkleTree)
        object.__setattr__(self, 'vtx', tuple(vtx))

        return self

    def stream_serialize(self, f, include_witness=True):
        super(CBlock, self).stream_serialize(f)
        VectorSerializer.stream_serialize(CTransaction, self.vtx, f, dict(include_witness=include_witness))

    def get_header(self):
        """Return the block header

        Returned header is a new object.
        """
        return CBlockHeader(nVersion=self.nVersion,
                            hashPrevBlock=self.hashPrevBlock,
                            hashMerkleRoot=self.hashMerkleRoot,
                            nTime=self.nTime,
                            nBits=self.nBits,
                            nNonce=self.nNonce)

    def GetHash(self):
        """Return the block hash

        Note that this is the hash of the header, not the entire serialized
        block.
        """
        try:
            return self._cached_GetHash
        except AttributeError:
            _cached_GetHash = self.get_header().GetHash()
            object.__setattr__(self, '_cached_GetHash', _cached_GetHash)
            return _cached_GetHash

    def GetWeight(self):
        """Return the block weight: (stripped_size * 3) + total_size"""
        return len(self.serialize(dict(include_witness=False))) * 3 + len(self.serialize())

class CoreChainParams(object):
    """Define consensus-critical parameters of a given instance of the Bitcoin system"""
    MAX_MONEY = None
    GENESIS_BLOCK = None
    PROOF_OF_WORK_LIMIT = None
    SUBSIDY_HALVING_INTERVAL = None
    NAME = None

class CoreMainParams(CoreChainParams):
    MAX_MONEY = 21000000 * COIN
    NAME = 'mainnet'
    GENESIS_BLOCK = CBlock.deserialize(x('0100000000000000000000000000000000000000000000000000000000000000000000003ba3edfd7a7b12b27ac72c3e67768f617fc81bc3888a51323a9fb8aa4b1e5e4a29ab5f49ffff001d1dac2b7c0101000000010000000000000000000000000000000000000000000000000000000000000000ffffffff4d04ffff001d0104455468652054696d65732030332f4a616e2f32303039204368616e63656c6c6f72206f6e206272696e6b206f66207365636f6e64206261696c6f757420666f722062616e6b73ffffffff0100f2052a01000000434104678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4cef38c4f35504e51ec112de5c384df7ba0b8d578a4c702b6bf11d5fac00000000'))
    SUBSIDY_HALVING_INTERVAL = 210000
    PROOF_OF_WORK_LIMIT = 2**256-1 >> 32

class CoreTestNetParams(CoreMainParams):
    NAME = 'testnet'
    GENESIS_BLOCK = CBlock.deserialize(x('0100000000000000000000000000000000000000000000000000000000000000000000003ba3edfd7a7b12b27ac72c3e67768f617fc81bc3888a51323a9fb8aa4b1e5e4adae5494dffff001d1aa4ae180101000000010000000000000000000000000000000000000000000000000000000000000000ffffffff4d04ffff001d0104455468652054696d65732030332f4a616e2f32303039204368616e63656c6c6f72206f6e206272696e6b206f66207365636f6e64206261696c6f757420666f722062616e6b73ffffffff0100f2052a01000000434104678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4cef38c4f35504e51ec112de5c384df7ba0b8d578a4c702b6bf11d5fac00000000'))

class CoreRegTestParams(CoreTestNetParams):
    NAME = 'regtest'
    GENESIS_BLOCK = CBlock.deserialize(x('0100000000000000000000000000000000000000000000000000000000000000000000003ba3edfd7a7b12b27ac72c3e67768f617fc81bc3888a51323a9fb8aa4b1e5e4adae5494dffff7f20020000000101000000010000000000000000000000000000000000000000000000000000000000000000ffffffff4d04ffff001d0104455468652054696d65732030332f4a616e2f32303039204368616e63656c6c6f72206f6e206272696e6b206f66207365636f6e64206261696c6f757420666f722062616e6b73ffffffff0100f2052a01000000434104678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4cef38c4f35504e51ec112de5c384df7ba0b8d578a4c702b6bf11d5fac00000000'))
    SUBSIDY_HALVING_INTERVAL = 150
    PROOF_OF_WORK_LIMIT = 2**256-1 >> 1

"""Master global setting for what core chain params we're using"""
coreparams = CoreMainParams()

def _SelectCoreParams(name):
    """Select the core chain parameters to use

    Don't use this directly, use bitcoin.SelectParams() instead so both
    consensus-critical and general parameters are set properly.
    """
    global coreparams
    if name == 'mainnet':
        coreparams = CoreMainParams()
    elif name == 'testnet':
        coreparams = CoreTestNetParams()
    elif name == 'regtest':
        coreparams = CoreRegTestParams()
    else:
        raise ValueError('Unknown chain %r' % name)


class CheckTransactionError(ValidationError):
    pass

def CheckTransaction(tx):
    """Basic transaction checks that don't depend on any context.

    Raises CheckTransactionError
    """
    global coreparams

    if not tx.vin:
        raise CheckTransactionError("CheckTransaction() : vin empty")
    if not tx.vout:
        raise CheckTransactionError("CheckTransaction() : vout empty")

    # Size limits
    base_tx = CTransaction(tx.vin, tx.vout, tx.nLockTime, tx.nVersion)
    if len(base_tx.serialize()) > MAX_BLOCK_SIZE:
        raise CheckTransactionError("CheckTransaction() : size limits failed")

    # Check for negative or overflow output values
    nValueOut = 0
    for txout in tx.vout:
        if txout.nValue < 0:
            raise CheckTransactionError("CheckTransaction() : txout.nValue negative")
        if txout.nValue > coreparams.MAX_MONEY:
            raise CheckTransactionError("CheckTransaction() : txout.nValue too high")
        nValueOut += txout.nValue
        if not MoneyRange(nValueOut):
            raise CheckTransactionError("CheckTransaction() : txout total out of range")

    # Check for duplicate inputs
    vin_outpoints = set()
    for txin in tx.vin:
        if txin.prevout in vin_outpoints:
            raise CheckTransactionError("CheckTransaction() : duplicate inputs")
        vin_outpoints.add(txin.prevout)

    if tx.is_coinbase():
        if not (2 <= len(tx.vin[0].scriptSig) <= 100):
            raise CheckTransactionError("CheckTransaction() : coinbase script size")

    else:
        for txin in tx.vin:
            if txin.prevout.is_null():
                raise CheckTransactionError("CheckTransaction() : prevout is null")





class CheckBlockHeaderError(ValidationError):
    pass

class CheckProofOfWorkError(CheckBlockHeaderError):
    pass

def CheckProofOfWork(hash, nBits):
    """Check a proof-of-work

    Raises CheckProofOfWorkError
    """
    target = uint256_from_compact(nBits)

    # Check range
    if not (0 < target <= coreparams.PROOF_OF_WORK_LIMIT):
        raise CheckProofOfWorkError("CheckProofOfWork() : nBits below minimum work")

    # Check proof of work matches claimed amount
    hash = uint256_from_str(hash)
    if hash > target:
        raise CheckProofOfWorkError("CheckProofOfWork() : hash doesn't match nBits")


def CheckBlockHeader(block_header, fCheckPoW = True, cur_time=None):
    """Context independent CBlockHeader checks.

    fCheckPoW - Check proof-of-work.

    cur_time  - Current time. Defaults to time.time()

    Raises CBlockHeaderError if block header is invalid.
    """
    if cur_time is None:
        cur_time = time.time()

    # Check proof-of-work matches claimed amount
    if fCheckPoW:
        CheckProofOfWork(block_header.GetHash(), block_header.nBits)

    # Check timestamp
    if block_header.nTime > cur_time + 2 * 60 * 60:
        raise CheckBlockHeaderError("CheckBlockHeader() : block timestamp too far in the future")


class CheckBlockError(CheckBlockHeaderError):
    pass

def GetLegacySigOpCount(tx):
    nSigOps = 0
    for txin in tx.vin:
        nSigOps += txin.scriptSig.GetSigOpCount(False)
    for txout in tx.vout:
        nSigOps += txout.scriptPubKey.GetSigOpCount(False)
    return nSigOps


def CheckBlock(block, fCheckPoW = True, fCheckMerkleRoot = True, cur_time=None):
    """Context independent CBlock checks.

    CheckBlockHeader() is called first, which may raise a CheckBlockHeader
    exception, followed the block tests. CheckTransaction() is called for every
    transaction.

    fCheckPoW        - Check proof-of-work.

    fCheckMerkleRoot - Check merkle root and witness merkle root matches transactions.
                     - Check witness commitment in coinbase

    cur_time         - Current time. Defaults to time.time()
    """

    # Block header checks
    CheckBlockHeader(block.get_header(), fCheckPoW=fCheckPoW, cur_time=cur_time)

    # Size limits
    if not block.vtx:
        raise CheckBlockError("CheckBlock() : vtx empty")
    if len(block.serialize(dict(include_witness=False))) > MAX_BLOCK_SIZE:
        raise CheckBlockError("CheckBlock() : block larger than MAX_BLOCK_SIZE")

    if block.GetWeight() > MAX_BLOCK_WEIGHT:
        raise CheckBlockError("CheckBlock() : block larger than MAX_BLOCK_WEIGHT")

    # First transaction must be coinbase
    if not block.vtx[0].is_coinbase():
        raise CheckBlockError("CheckBlock() : first tx is not coinbase")

    # Check rest of transactions. Note how we do things "all at once", which
    # could potentially be a consensus failure if there was some obscure bug.

    # For unique txid uniqueness testing. If coinbase tx is included twice
    # it'll be caught by the "more than one coinbase" test.
    unique_txids = set()
    nSigOps = 0
    for tx in block.vtx[1:]:
        if tx.is_coinbase():
            raise CheckBlockError("CheckBlock() : more than one coinbase")

        CheckTransaction(tx)

        txid = tx.GetTxid()
        if txid in unique_txids:
            raise CheckBlockError("CheckBlock() : duplicate transaction")
        unique_txids.add(txid)

        nSigOps += GetLegacySigOpCount(tx)
        if nSigOps > MAX_BLOCK_SIGOPS:
            raise CheckBlockError("CheckBlock() : out-of-bounds SigOpCount")

    # Check merkle root
    if fCheckMerkleRoot:
        if block.hashMerkleRoot != block.calc_merkle_root():
            raise CheckBlockError("CheckBlock() : hashMerkleRoot mismatch")
        if len(block.vWitnessMerkleTree):
            # At least 1 tx has witness: check witness merkle tree
            root = block.vWitnessMerkleTree[-1]
            # vtx[0]: coinbase
            # vtxinwit[0]: first input
            nonce_script = block.vtx[0].wit.vtxinwit[0].scriptWitness
            nonce = nonce_script.stack[0]
            if len(nonce_script.stack) != 1 or len(nonce) != 32:
                raise CheckBlockError("CheckBlock() : invalid coinbase witnessScript")
            try:
                index = block.get_witness_commitment_index()
            except ValueError as e:
                raise CheckBlockError("CheckBlock() : " + str(e))
            commit_script = block.vtx[0].vout[index].scriptPubKey
            if not (6 + 32 <= len(commit_script) <= 6 + 32 + 1):
                raise CheckBlockError("CheckBlock() : invalid segwit commitment length")
            commitment = commit_script[6:6 + 32]
            commit = commit_script[6:6 + 32]
            if commit != Hash(root + nonce):
                raise CheckBlockError("CheckBlock() : invalid segwit commitment")

__all__ = (
        'Hash',
        'Hash160',
        'COIN',
        'MAX_BLOCK_SIZE',
        'MAX_BLOCK_SIGOPS',
        'MoneyRange',
        'x',
        'b2x',
        'lx',
        'b2lx',
        'str_money_value',
        'ValidationError',
        'COutPoint',
        'CMutableOutPoint',
        'CTxIn',
        'CMutableTxIn',
        'CTxOut',
        'CMutableTxOut',
        'CTransaction',
        'CMutableTransaction',
        'CTxWitness',
        'CTxInWitness',
        'CBlockHeader',
        'CBlock',
        'CoreChainParams',
        'CoreMainParams',
        'CoreTestNetParams',
        'CoreRegTestParams',
        'CheckTransactionError',
        'CheckTransaction',
        'CheckBlockHeaderError',
        'CheckProofOfWorkError',
        'CheckProofOfWork',
        'CheckBlockHeader',
        'CheckBlockError',
        'GetLegacySigOpCount',
        'CheckBlock',
)
