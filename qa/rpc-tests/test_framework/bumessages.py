from .nodemessages import *


class msg_buversion(object):
    command = b"buversion"

    def __init__(self, addrFromPort=None):
        self.addrFromPort = addrFromPort
        pass

    def deserialize(self, f):
        self.addrFromPort = struct.unpack("<H", f.read(2))[0]
        return self

    def serialize(self):
        r = b""
        r += struct.pack("<H", self.addrFromPort)
        return r

    def __repr__(self):
        return "msg_buversion(addrFromPort=%d)" % (self.addrFromPort)


class msg_buverack(object):
    command = b"buverack"

    def __init__(self):
        pass

    def deserialize(self, f):
        return self

    def serialize(self):
        r = b""
        return r

    def __repr__(self):
        return "msg_buverack()"


class QHash(object):
    """quarter hash"""

    def __init__(self, shortHash=None):
        self.hash = shortHash

    def deserialize(self, f):
        self.hash = struct.unpack("<Q", f.read(8))[0]
        return self

    def serialize(self):
        r = b""
        r += struct.pack("<Q", self.hash)
        return r

    def __repr__(self):
        return "QHash(0x%016x)" % (self.hash)


class Hash(object):
    """sha256 hash"""

    def __init__(self, hash=None):
        self.hash = hash

    def deserialize(self, f):
        self.hash = deser_uint256(f)
        return self

    def serialize(self):
        r = b""
        r += ser_uint256(self.hash)
        return r

    def __str__(self):
        return "%064x" % self.hash

    def __repr__(self):
        return "Hash(%064x)" % self.hash


class CXThinBlock(CBlockHeader):
    def __init__(self, header=None, vTxHashes=None, vMissingTx=None):
        super(CXThinBlock, self).__init__(header)
        self.vTxHashes = vTxHashes
        self.vMissingTx = vMissingTx

    def deserialize(self, f):
        super(CXThinBlock, self).deserialize(f)
        self.vTxHashes = deser_vector(f, QHash)
        self.vMissingTx = deser_vector(f, CTransaction)
        return self

    def serialize(self):
        r = b""
        r += super(CXThinBlock, self).serialize()
        r += ser_vector(self.vTxHashes)
        r += ser_vector(self.vMissingTx)
        return r

    def summary(self):
        s = []
        s.append(super(self.__class__, self).summary())
        s.append("\nQuarter Hashes")
        count = 0
        for qh in self.vTxHashes:
            if (count % 5) == 0:
                s.append("\n%4d: " % count)
            s.append("%016x " % qh.hash)
            count += 1

        s.append("\nFull Transactions\n")
        count = 0
        for tx in self.vMissingTx:
            s.append("%4d: %s\n" % (count, tx.summary()))
            count += 1
        return "".join(s)

    def __str__(self):
        return "CXThinBlock(nVersion=%i hashPrevBlock=%064x hashMerkleRoot=%064x nTime=%s nBits=%08x nNonce=%08x vTxHashes_len=%d vMissingTx_len=%d)" \
            % (self.nVersion, self.hashPrevBlock, self.hashMerkleRoot, time.ctime(self.nTime), self.nBits, self.nNonce, len(self.vTxHashes), len(self.vMissingTx))

    # For normal "mainnet" blocks, this function produces a painfully large single line output.
    # It is so large, you may be forced to kill your python shell just to get it to stop.
    # But it is easy to accidentally call repr from the python interactive shell or pdb.  There is no current
    # use and removing this function call makes interactive sessions easier to use.
    # However, the function shall be left commented out for symmetry with the other objects and in case
    # it is needed.
    # def __repr__(self):
    #    return "CXThinBlock(nVersion=%i hashPrevBlock=%064x hashMerkleRoot=%064x nTime=%s nBits=%08x nNonce=%08x vTxHashes=%s vMissingTx=%s)" \
    #        % (self.nVersion, self.hashPrevBlock, self.hashMerkleRoot,
    #           time.ctime(self.nTime), self.nBits, self.nNonce, repr(self.vTxHashes), repr(self.vMissingTx))


class CThinBlock(CBlockHeader):
    def __init__(self, header=None):
        super(self.__class__, self).__init__(header)
        self.vTxHashes = []
        self.vMissingTx = []

    def deserialize(self, f):
        super(self.__class__, self).deserialize(f)
        self.vTxHashes = deser_vector(f, Hash)
        self.vMissingTx = deser_vector(f, CTransaction)
        return self

    def serialize(self):
        r = b""
        r += super(self.__class__, self).serialize()
        r += ser_vector(self.vTxHashes)
        r += ser_vector(self.vMissingTx)
        return r

    def __str__(self):
        return "CThinBlock(nVersion=%i hashPrevBlock=%064x hashMerkleRoot=%064x nTime=%s nBits=%08x nNonce=%08x vTxHashes_len=%d vMissingTx_len=%d)" \
            % (self.nVersion, self.hashPrevBlock, self.hashMerkleRoot, time.ctime(self.nTime), self.nBits, self.nNonce, len(self.vTxHashes), len(self.vMissingTx))

    # For normal "mainnet" blocks, this function produces a painfully large single line output.
    # It is so large, you may be forced to kill your python shell just to get it to stop.
    # But it is easy to accidentally call repr from the python interactive shell or pdb.  There is no current
    # use and removing this function call makes interactive sessions easier to use.
    # However, the function shall be left commented out for symmetry with the other objects and in case
    # it is needed.
    # def __repr__(self):
    #    return "CThinBlock(nVersion=%i hashPrevBlock=%064x hashMerkleRoot=%064x nTime=%s nBits=%08x nNonce=%08x vTxHashes=%s vMissingTx=%s)" \
    #        % (self.nVersion, self.hashPrevBlock, self.hashMerkleRoot,
    #           time.ctime(self.nTime), self.nBits, self.nNonce, repr(self.vTxHashes), repr(self.vMissingTx))


class CBloomFilter:
    def __init__(self, vData=None):
        self.vData = vData
        self.nHashFuncs = None
        self.nTweak = None
        self.nFlags = None

    def deserialize(self, f):
        self.vData = deser_string(f)
        self.nHashFuncs = struct.unpack("<I", f.read(4))[0]
        self.nTweak = struct.unpack("<I", f.read(4))[0]
        self.nFlags = struct.unpack("<B", f.read(1))[0]
        return self

    def serialize(self):
        r = b""
        r += ser_string(f, self.vData)
        r += struct.pack("<I", self.nHashFuncs)
        r += struct.pack("<I", self.nTweak)
        r += struct.pack("<B", self.nFlags)
        return r

    def __repr__(self):
        return "%s(vData=%s)" % (self.__class__.__name__, self.vData)


class msg_thinblock(object):
    command = b"thinblock"

    def __init__(self, block=None):
        if block is None:
            self.block = CThinBlock()
        else:
            self.block = block

    def deserialize(self, f):
        self.block.deserialize(f)
        return self

    def serialize(self):
        return self.block.serialize()

    def __str__(self):
        return "msg_thinblock(block=%s)" % (str(self.block))

    def __repr__(self):
        return "msg_thinblock(block=%s)" % (repr(self.block))


class msg_xthinblock(object):
    command = b"xthinblock"

    def __init__(self, block=None):
        if block is None:
            self.block = CXThinBlock()
        else:
            self.block = block

    def deserialize(self, f):
        self.block.deserialize(f)
        return self

    def serialize(self):
        return self.block.serialize()

    def __str__(self):
        return "msg_xthinblock(block=%s)" % (str(self.block))

    def __repr__(self):
        return "msg_xthinblock(block=%s)" % (repr(self.block))


class msg_Xb(object):
    """Expedited block message"""
    command = b"Xb"
    EXPEDITED_MSG_HDR = 1
    EXPEDITED_MSG_XTHIN = 2

    def __init__(self, block=None, hops=0, msgType=EXPEDITED_MSG_XTHIN):
        self.msgType = msgType
        self.hops = hops
        self.block = block

    def deserialize(self, f):
        self.msgType = struct.unpack("<B", f.read(1))[0]
        self.hops = struct.unpack("<B", f.read(1))[0]
        if self.msgType == EXPEDITED_MSG_XTHIN:
            self.block = CXThinBlock()
            self.block.deserialize(f)
        else:
            self.block = None
        return self

    def serialize(self):
        r = b""
        r += struct.pack("<B", self.msgType)
        r += struct.pack("<B", self.hops)
        if self.msgType == EXPEDITED_MSG_XTHIN:
            r += self.block.serialize()
        return r

    def __str__(self):
        return "msg_Xb(block=%s)" % (str(self.block))

    def __repr__(self):
        return "msg_Xb(block=%s)" % (repr(self.block))


class msg_get_xthin(object):
    command = b"get_xthin"

    def __init__(self, inv=None, filter=None):
        self.inv = inv
        self.filter = filter

    def deserialize(self, f):
        self.inv = CInv()
        self.inv.deserialize(f)
        self.filter = CBloomFilter()
        self.filter.deserialize(f)
        return self

    def serialize(self):
        r = b""
        r += self.inv.serialize()
        r += self.filter.serialize()
        return r

    def __repr__(self):
        return "%s(inv=%s,filter=%s)" % (self.__class__.__name__, repr(self.inv), repr(self.filter))


class msg_filterload(object):
    command = b"filterload"

    def __init__(self, inv=None, filter=None):
        self.filter = filter

    def deserialize(self, f):
        self.filter = CBloomFilter()
        self.filter.deserialize(f)
        return self

    def serialize(self):
        r = b""
        r += self.filter.serialize()
        return r

    def __repr__(self):
        return "%s(filter=%s)" % (self.__class__.__name__, repr(self.filter))


class msg_filteradd(object):
    command = b"filteradd"

    def __init__(self, inv=None, filter=None):
        self.filter = filter

    def deserialize(self, f):
        self.filter = deser_string(f)
        return self

    def serialize(self):
        r = b""
        r += ser_string(f, self.filter)
        return r

    def __repr__(self):
        return "%s(filteradd=%s)" % (self.__class__.__name__, repr(self.filter))


class msg_filterclear(object):
    command = b"filterclear"

    def __init__(self):
        pass

    def deserialize(self, f):
        return self

    def serialize(self):
        r = b""
        return r

    def __repr__(self):
        return "msg_filterclear()"


class msg_get_xblocktx(object):
    command = b"get_xblocktx"

    def __init__(self, blockhash=None, qhashes=None):
        self.blockhash = blockhash
        self.setCheapHashesToRequest = qhashes

    def deserialize(self, f):
        self.blockhash = deser_uint256(f)
        self.setCheapHashesToRequest = deser_vector(f, QHash)
        return self

    def serialize(self):
        r = b""
        r += ser_uint256(self.blockhash)
        r += ser_vector(self.setCheapHashesToRequest)
        return r

    def __repr__(self):
        return "%s(blockhash=%s,qhash=%s)" % (self.__class__.__name__, repr(self.blockhash), repr(self.setCheapHashesToRequest))


class msg_req_xpedited(object):
    """request expedited blocks"""
    command = b"req_xpedited"
    EXPEDITED_STOP = 1
    EXPEDITED_BLOCKS = 2
    EXPEDITED_TXNS = 4

    def __init__(self, options=None):
        self.options = options

    def deserialize(self, f):
        self.options = struct.unpack("<Q", f.read(8))[0]
        return self

    def serialize(self):
        r = b""
        r += struct.pack("<Q", self.options)
        return r

    def __repr__(self):
        return "%s(0x%x)" % (self.__class__.__name__, self.options)


bumessagemap = {
    msg_buversion.command: msg_buversion,
    msg_buverack.command: msg_buverack,
    msg_xthinblock.command: msg_xthinblock,
    msg_thinblock.command: msg_thinblock,
    msg_get_xthin.command: msg_get_xthin,
    msg_get_xblocktx.command: msg_get_xblocktx,
    msg_filterload.command: msg_filterload,
    msg_filteradd.command: msg_filteradd,
    msg_filterclear.command: msg_filterclear,
    msg_Xb.command: msg_Xb,
    msg_req_xpedited.command: msg_req_xpedited,
}
