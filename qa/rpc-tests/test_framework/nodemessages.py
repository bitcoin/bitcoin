import socket
import struct
import random
import hashlib
from binascii import hexlify, unhexlify
import time
from codecs import encode
from threading import RLock
from io import BytesIO
MY_VERSION = 60001  # past bip-31 for ping/pong
MY_SUBVERSION = b"/python-mininode-tester:0.0.3/"

COIN = 100000000  # 1 btc in satoshis

# One lock for synchronizing all data access between the networking thread (see
# NetworkThread below) and the thread running the test logic.  For simplicity,
# NodeConn acquires this lock whenever delivering a message to to a NodeConnCB,
# and whenever adding anything to the send buffer (in send_message()).  This
# lock should be acquired in the thread running the test logic to synchronize
# access to any data shared with the NodeConnCB or NodeConn.
mininode_lock = RLock()

# Helper function


def wait_until(predicate, attempts=float('inf'), timeout=float('inf')):
    attempt = 0
    elapsed = 0

    while attempt < attempts and elapsed < timeout:
        with mininode_lock:
            if predicate():
                return True
        attempt += 1
        elapsed += 0.05
        time.sleep(0.05)

    return False


# Serialization/deserialization tools
def sha256(s):
    """Return the sha256 hash of the passed binary data

    >>> hexlify(sha256("e hat eye pie plus one is O".encode()))
    b'c5b94099f454a3807377724eb99a33fbe9cb5813006cadc03e862a89d410eaf0'
    """
    return hashlib.new('sha256', s).digest()


def hash256(s):
    """Return the double SHA256 hash (what bitcoin typically uses) of the passed binary data

    >>> hexlify(hash256("There was a terrible ghastly silence".encode()))
    b'730ac30b1e7f4061346277ab639d7a68c6686aeba4cc63280968b903024a0a40'
    """
    return sha256(sha256(s))


def deser_string(f):
    """Convert an array of bytes in the bitcoin P2P protocol format into a string

    >>> import io
    >>> deser_string(io.BytesIO(ser_string("The grid bug bites!  You get zapped!".encode()))).decode()
    'The grid bug bites!  You get zapped!'
    """
    nit = struct.unpack("<B", f.read(1))[0]
    if nit == 253:
        nit = struct.unpack("<H", f.read(2))[0]
    elif nit == 254:
        nit = struct.unpack("<I", f.read(4))[0]
    elif nit == 255:
        nit = struct.unpack("<Q", f.read(8))[0]
    return f.read(nit)


def ser_string(s):
    """convert a string into an array of bytes (in the bitcoin network format)

       >>> ser_string("The grid bug bites!  You get zapped!".encode())
       b'$The grid bug bites!  You get zapped!'
    """
    if len(s) < 253:
        return struct.pack("B", len(s)) + s
    elif len(s) < 0x10000:
        return struct.pack("<BH", 253, len(s)) + s
    elif len(s) < 0x100000000:
        return struct.pack("<BI", 254, len(s)) + s
    return struct.pack("<BQ", 255, len(s)) + s


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
    nit = struct.unpack("<B", f.read(1))[0]
    if nit == 253:
        nit = struct.unpack("<H", f.read(2))[0]
    elif nit == 254:
        nit = struct.unpack("<I", f.read(4))[0]
    elif nit == 255:
        nit = struct.unpack("<Q", f.read(8))[0]
    r = []
    for i in range(nit):
        t = c()
        t.deserialize(f)
        r.append(t)
    return r


def ser_vector(l):
    r = b""
    if len(l) < 253:
        r = struct.pack("B", len(l))
    elif len(l) < 0x10000:
        r = struct.pack("<BH", 253, len(l))
    elif len(l) < 0x100000000:
        r = struct.pack("<BI", 254, len(l))
    else:
        r = struct.pack("<BQ", 255, len(l))
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
    for i in range(nit):
        t = deser_uint256(f)
        r.append(t)
    return r


def ser_uint256_vector(l):
    r = b""
    if len(l) < 253:
        r = struct.pack("B", len(l))
    elif len(l) < 0x10000:
        r = struct.pack("<BH", 253, len(l))
    elif len(l) < 0x100000000:
        r = struct.pack("<BI", 254, len(l))
    else:
        r = struct.pack("<BQ", 255, len(l))
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
    for i in range(nit):
        t = deser_string(f)
        r.append(t)
    return r


def ser_string_vector(l):
    r = b""
    if len(l) < 253:
        r = struct.pack("B", len(l))
    elif len(l) < 0x10000:
        r = struct.pack("<BH", 253, len(l))
    elif len(l) < 0x100000000:
        r = struct.pack("<BI", 254, len(l))
    else:
        r = struct.pack("<BQ", 255, len(l))
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
    for i in range(nit):
        t = struct.unpack("<i", f.read(4))[0]
        r.append(t)
    return r


def ser_int_vector(l):
    r = b""
    if len(l) < 253:
        r = struct.pack("B", len(l))
    elif len(l) < 0x10000:
        r = struct.pack("<BH", 253, len(l))
    elif len(l) < 0x100000000:
        r = struct.pack("<BI", 254, len(l))
    else:
        r = struct.pack("<BQ", 255, len(l))
    for i in l:
        r += struct.pack("<i", i)
    return r

# Deserialize from a hex string representation (eg from RPC)


def FromHex(obj, hex_string):
    obj.deserialize(BytesIO(unhexlify(hex_string.encode('ascii'))))
    return obj

# Convert a binary-serializable object to hex (eg for submission via RPC)


def ToHex(obj):
    return hexlify(obj.serialize()).decode('ascii')


# Objects that map to bitcoind objects, which can be serialized/deserialized

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
    MSG_TX = 1
    MSG_BLOCK = 2
    MSG_FILTERED_BLOCK = 3
    MSG_THINBLOCK = 4
    MSG_XTHINBLOCK = 5
    typemap = {
        0: "Error",
        1: "TX",
        2: "Block",
        3: "FilteredBlock",
        4: "ThinBlock",
        5: "XThinBlock",
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
            % (self.typemap[self.type], self.hash)


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
        if type(hash) is str:
            t = bytearray(unhexlify(hash))
            t.reverse()
            hash = uint256_from_str(t)
        if type(hash) is bytes:
            hash = uint256_from_str(hash)
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
            % (repr(self.prevout), hexlify(self.scriptSig),
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
        r += struct.pack("<q", int(self.nValue))
        r += ser_string(self.scriptPubKey)
        return r

    def __repr__(self):
        return "CTxOut(nValue=%i.%08i scriptPubKey=%s)" \
            % (self.nValue // COIN, self.nValue % COIN,
               hexlify(self.scriptPubKey))


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
        r = b""
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
        self.hash = encode(hash256(self.serialize())[::-1], 'hex_codec').decode('ascii')

    def is_valid(self):
        self.calc_sha256()
        for tout in self.vout:
            if tout.nValue < 0 or tout.nValue > 21000000 * COIN:
                return False
        return True

    def summary(self):
        self.calc_sha256()
        s = ["Transaction: %064x\n" % self.sha256]
        s.append("%d inputs\n" % len(self.vin))
        for vin in self.vin:
            s.append("  %064x.%d\n" % (vin.prevout.hash, vin.prevout.n))
        s.append("%d outputs\n" % len(self.vout))
        for vout in self.vout:
            s.append("  %12d %s\n" % (vout.nValue, hexlify((vout.scriptPubKey))))
        return "".join(s)

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
            self.sha256 = uint256_from_str(hash256(r))
            self.hash = encode(hash256(r)[::-1], 'hex_codec').decode('ascii')

    def gethash(self):
        self.calc_sha256()
        return self.sha256

    def rehash(self):
        self.sha256 = None
        self.calc_sha256()
        return self.sha256

    def summary(self):
        s = []
        s.append("Block:  %064x  Time:%s  Version:0x%x Bits:0x%08x\n" %
                 (self.gethash(), time.ctime(self.nTime), self.nVersion, self.nBits))
        s.append("Parent: %064x  Merkle: %064x" % (self.hashPrevBlock, self.hashMerkleRoot))
        return "".join(s)

    def __str__(self):
        return "CBlockHeader(hash=%064x nVersion=%i hashPrevBlock=%064x hashMerkleRoot=%064x nTime=%s nBits=%08x nNonce=%08x)" \
            % (self.gethash(), self.nVersion, self.hashPrevBlock, self.hashMerkleRoot, time.ctime(self.nTime), self.nBits, self.nNonce)

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

    def calc_merkle_root(self):
        hashes = []
        for tx in self.vtx:
            tx.calc_sha256()
            hashes.append(ser_uint256(tx.sha256))
        while len(hashes) > 1:
            newhashes = []
            for i in range(0, len(hashes), 2):
                i2 = min(i + 1, len(hashes) - 1)
                newhashes.append(hash256(hashes[i] + hashes[i2]))
            hashes = newhashes
        if hashes:
            return uint256_from_str(hashes[0])
        return 0

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

    def __str__(self):
        return "CBlock(nVersion=%i hashPrevBlock=%064x hashMerkleRoot=%064x nTime=%s nBits=%08x nNonce=%08x vtx_len=%d)" \
            % (self.nVersion, self.hashPrevBlock, self.hashMerkleRoot,
               time.ctime(self.nTime), self.nBits, self.nNonce, len(self.vtx))

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
        self.strComment = b""
        self.strStatusBar = b""
        self.strReserved = b""

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
        r = b""
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
        self.vchMsg = b""
        self.vchSig = b""

    def deserialize(self, f):
        self.vchMsg = deser_string(f)
        self.vchSig = deser_string(f)

    def serialize(self):
        r = b""
        r += ser_string(self.vchMsg)
        r += ser_string(self.vchSig)
        return r

    def __repr__(self):
        return "CAlert(vchMsg.sz %d, vchSig.sz %d)" \
            % (len(self.vchMsg), len(self.vchSig))


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
        r = b""
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


class msg_alert(object):
    command = b"alert"

    def __init__(self):
        self.alert = CAlert()

    def deserialize(self, f):
        self.alert = CAlert()
        self.alert.deserialize(f)

    def serialize(self):
        r = b""
        r += self.alert.serialize()
        return r

    def __repr__(self):
        return "msg_alert(alert=%s)" % (repr(self.alert), )


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
        if inv is None:
            self.inv = []
        elif type(inv) == list:
            self.inv = inv
        else:
            self.inv = [inv]

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

    def __str__(self):
        return "msg_block(block=%s)" % (str(self.block))

    def __repr__(self):
        return "msg_block(block=%s)" % (repr(self.block))


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



class msg_getheaders(object):
    """
    getheaders message has
    locator: CBlockLocator object that identifies what block to start with
    hash_stop: hash of last desired block header, 0 to get as many as possible
    """
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


class msg_headers(object):
    """
    headers message has
    <count> <vector of block headers>
    """
    command = b"headers"

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



def Test():
    import doctest
    import sys
    print(doctest.testmod(sys.modules[__name__],verbose=True))
