# Copyright (C) 2012-2015 The python-bitcoinlib developers
#
# This file is part of python-bitcoinlib.
#
# It is subject to the license terms in the LICENSE file found in the top-level
# directory of this distribution.
#
# No part of python-bitcoinlib, including this file, may be copied, modified,
# propagated, or distributed except according to the terms contained in the
# LICENSE file.

from __future__ import absolute_import, division, print_function, unicode_literals

import hashlib
import random
import struct
import time

# Py3 compatibility
import sys

if sys.version > '3':
    _bchr = lambda x: bytes([x])
    _bord = lambda x: x[0]
    from io import BytesIO as _BytesIO
else:
    _bchr = chr
    _bord = ord
    from cStringIO import StringIO as _BytesIO

# Bad practice, so we have a __all__ at the end; this should be cleaned up
# later.
from bitcoin.core import *
from bitcoin.core.serialize import *
from bitcoin.net import *
import bitcoin

MSG_WITNESS_FLAG = 1 << 30
MSG_TYPE_MASK = 0xffffffff >> 2

MSG_TX = 1
MSG_BLOCK = 2
MSG_FILTERED_BLOCK = 3
MSG_CMPCT_BLOCK = 4
MSG_WITNESS_BLOCK = MSG_BLOCK | MSG_WITNESS_FLAG,
MSG_WITNESS_TX = MSG_TX | MSG_WITNESS_FLAG,
MSG_FILTERED_WITNESS_BLOCK = MSG_FILTERED_BLOCK | MSG_WITNESS_FLAG,


class MsgSerializable(Serializable):
    def __init__(self, protover=PROTO_VERSION):
        self.protover = protover

    def msg_ser(self, f):
        raise NotImplementedError

    @classmethod
    def msg_deser(cls, f, protover=PROTO_VERSION):
        raise NotImplementedError

    def to_bytes(self):
        f = _BytesIO()
        self.msg_ser(f)
        body = f.getvalue()
        res = bitcoin.params.MESSAGE_START
        res += self.command
        res += b"\x00" * (12 - len(self.command))
        res += struct.pack(b"<I", len(body))

        # add checksum
        th = hashlib.sha256(body).digest()
        h = hashlib.sha256(th).digest()
        res += h[:4]

        res += body
        return res

    @classmethod
    def from_bytes(cls, b, protover=PROTO_VERSION):
        f = _BytesIO(b)
        return MsgSerializable.stream_deserialize(f, protover=protover)

    @classmethod
    def stream_deserialize(cls, f, protover=PROTO_VERSION):
        recvbuf = ser_read(f, 4 + 12 + 4 + 4)

        # check magic
        if recvbuf[:4] != bitcoin.params.MESSAGE_START:
            raise ValueError("Invalid message start '%s', expected '%s'" %
                             (b2x(recvbuf[:4]), b2x(bitcoin.params.MESSAGE_START)))

        # remaining header fields: command, msg length, checksum
        command = recvbuf[4:4+12].split(b"\x00", 1)[0]
        msglen = struct.unpack(b"<i", recvbuf[4+12:4+12+4])[0]
        checksum = recvbuf[4+12+4:4+12+4+4]

        # read message body
        recvbuf += ser_read(f, msglen)

        msg = recvbuf[4+12+4+4:4+12+4+4+msglen]
        th = hashlib.sha256(msg).digest()
        h = hashlib.sha256(th).digest()
        if checksum != h[:4]:
            raise ValueError("got bad checksum %s" % repr(recvbuf))
            recvbuf = recvbuf[4+12+4+4+msglen:]

        if command in messagemap:
            cls = messagemap[command]
            #        print("Going to deserialize '%s'" % msg)
            return cls.msg_deser(_BytesIO(msg))
        else:
            print("Command '%s' not in messagemap" % repr(command))
            return None

    def stream_serialize(self, f):
        data = self.to_bytes()
        f.write(data)


class msg_version(MsgSerializable):
    command = b"version"

    def __init__(self, protover=PROTO_VERSION):
        super(msg_version, self).__init__(protover)
        self.nVersion = protover
        self.nServices = 1
        self.nTime = int(time.time())
        self.addrTo = CAddress(PROTO_VERSION)
        self.addrFrom = CAddress(PROTO_VERSION)
        self.nNonce = random.getrandbits(64)
        self.strSubVer = (b'/python-bitcoinlib:' +
                          bitcoin.__version__.encode('ascii') + b'/')
        self.nStartingHeight = -1
        self.fRelay = True

    @classmethod
    def msg_deser(cls, f, protover=PROTO_VERSION):
        c = cls()
        c.nVersion = struct.unpack(b"<i", ser_read(f, 4))[0]
        if c.nVersion == 10300:
            c.nVersion = 300
        c.nServices = struct.unpack(b"<Q", ser_read(f, 8))[0]
        c.nTime = struct.unpack(b"<q", ser_read(f, 8))[0]
        c.addrTo = CAddress.stream_deserialize(f, True)
        if c.nVersion >= 106:
            c.addrFrom = CAddress.stream_deserialize(f, True)
            c.nNonce = struct.unpack(b"<Q", ser_read(f, 8))[0]
            c.strSubVer = VarStringSerializer.stream_deserialize(f)
            if c.nVersion >= 209:
                c.nStartingHeight = struct.unpack(b"<i", ser_read(f,4))[0]
            else:
                c.nStartingHeight = None
        else:
            c.addrFrom = None
            c.nNonce = None
            c.strSubVer = None
            c.nStartingHeight = None
        if c.nVersion >= 70001:
            c.fRelay = struct.unpack(b"<B", ser_read(f,1))[0]
        else:
            c.fRelay = True
        return c
 
    def msg_ser(self, f):
        f.write(struct.pack(b"<i", self.nVersion))
        f.write(struct.pack(b"<Q", self.nServices))
        f.write(struct.pack(b"<q", self.nTime))
        self.addrTo.stream_serialize(f, True)
        self.addrFrom.stream_serialize(f, True)
        f.write(struct.pack(b"<Q", self.nNonce))
        VarStringSerializer.stream_serialize(self.strSubVer, f)
        f.write(struct.pack(b"<i", self.nStartingHeight))
        f.write(struct.pack(b"<B", self.fRelay))

    def __repr__(self):
        return "msg_version(nVersion=%i nServices=%i nTime=%s addrTo=%s addrFrom=%s nNonce=0x%016X strSubVer=%s nStartingHeight=%i fRelay=%r)" % (self.nVersion, self.nServices, time.ctime(self.nTime), repr(self.addrTo), repr(self.addrFrom), self.nNonce, self.strSubVer, self.nStartingHeight, self.fRelay)


class msg_verack(MsgSerializable):
    command = b"verack"

    def __init__(self, protover=PROTO_VERSION):
        super(msg_verack, self).__init__(protover)
        self.protover = protover

    @classmethod
    def msg_deser(cls, f, protover=PROTO_VERSION):
        return cls()

    def msg_ser(self, f):
        f.write(b"")

    def __repr__(self):
        return "msg_verack()"


class msg_addr(MsgSerializable):
    command = b"addr"

    def __init__(self, protover=PROTO_VERSION):
        super(msg_addr, self).__init__(protover)
        self.addrs = []

    @classmethod
    def msg_deser(cls, f, protover=PROTO_VERSION):
        c = cls()
        c.addrs = VectorSerializer.stream_deserialize(CAddress, f)
        return c

    def msg_ser(self, f):
        VectorSerializer.stream_serialize(CAddress, self.addrs, f)

    def __repr__(self):
        return "msg_addr(addrs=%s)" % (repr(self.addrs))


class msg_alert(MsgSerializable):
    command = b"alert"

    def __init__(self, protover=PROTO_VERSION):
        super(msg_alert, self).__init__(protover)
        self.alert = CAlert()

    @classmethod
    def msg_deser(cls, f, protover=PROTO_VERSION):
        c = cls()
        c.alert = CAlert.stream_deserialize(f)
        return c

    def msg_ser(self, f):
        self.alert.stream_serialize(f)

    def __repr__(self):
        return "msg_alert(alert=%s)" % (repr(self.alert), )


class msg_inv(MsgSerializable):
    command = b"inv"

    def __init__(self, protover=PROTO_VERSION):
        super(msg_inv, self).__init__(protover)
        self.inv = []

    @classmethod
    def msg_deser(cls, f, protover=PROTO_VERSION):
        c = cls()
        c.inv = VectorSerializer.stream_deserialize(CInv, f)
        return c

    def msg_ser(self, f):
        VectorSerializer.stream_serialize(CInv, self.inv, f)

    def __repr__(self):
        return "msg_inv(inv=%s)" % (repr(self.inv))


class msg_getdata(MsgSerializable):
    command = b"getdata"

    def __init__(self, protover=PROTO_VERSION):
        super(msg_getdata, self).__init__(protover)
        self.inv = []

    @classmethod
    def msg_deser(cls, f, protover=PROTO_VERSION):
        c = cls()
        c.inv = VectorSerializer.stream_deserialize(CInv, f)
        return c

    def msg_ser(self, f):
        VectorSerializer.stream_serialize(CInv, self.inv, f)

    def __repr__(self):
        return "msg_getdata(inv=%s)" % (repr(self.inv))

class msg_notfound(MsgSerializable):
    command = b"notfound"

    def __init__(self, protover=PROTO_VERSION):
        super(msg_notfound, self).__init__(protover)
        self.inv = []

    @classmethod
    def msg_deser(cls, f, protover=PROTO_VERSION):
        c = cls()
        c.inv = VectorSerializer.stream_deserialize(CInv, f)
        return c

    def msg_ser(self, f):
        VectorSerializer.stream_serialize(CInv, self.inv, f)

    def __repr__(self):
        return "msg_notfound(inv=%s)" % (repr(self.inv))


class msg_getblocks(MsgSerializable):
    command = b"getblocks"

    def __init__(self, protover=PROTO_VERSION):
        super(msg_getblocks, self).__init__(protover)
        self.locator = CBlockLocator()
        self.hashstop = b'\x00'*32

    @classmethod
    def msg_deser(cls, f, protover=PROTO_VERSION):
        c = cls()
        c.locator = CBlockLocator.stream_deserialize(f)
        c.hashstop = ser_read(f, 32)
        return c

    def msg_ser(self, f):
        self.locator.stream_serialize(f)
        f.write(self.hashstop)

    def __repr__(self):
        return "msg_getblocks(locator=%s hashstop=%s)" % (repr(self.locator), b2x(self.hashstop))


class msg_getheaders(MsgSerializable):
    command = b"getheaders"

    def __init__(self, protover=PROTO_VERSION):
        super(msg_getheaders, self).__init__(protover)
        self.locator = CBlockLocator()
        self.hashstop = b'\x00'*32

    @classmethod
    def msg_deser(cls, f, protover=PROTO_VERSION):
        c = cls()
        c.locator = CBlockLocator.stream_deserialize(f)
        c.hashstop = ser_read(f, 32)
        return c

    def msg_ser(self, f):
        self.locator.stream_serialize(f)
        f.write(self.hashstop)

    def __repr__(self):
        return "msg_getheaders(locator=%s hashstop=%s)" % (repr(self.locator), b2x(self.hashstop))


class msg_headers(MsgSerializable):
    command = b"headers"

    def __init__(self, protover=PROTO_VERSION):
        super(msg_headers, self).__init__(protover)
        self.headers = []

    @classmethod
    def msg_deser(cls, f, protover=PROTO_VERSION):
        c = cls()
        c.headers = VectorSerializer.stream_deserialize(CBlockHeader, f)
        return c

    def msg_ser(self, f):
        VectorSerializer.stream_serialize(CBlockHeader, self.headers, f)

    def __repr__(self):
        return "msg_headers(headers=%s)" % (repr(self.headers))


class msg_tx(MsgSerializable):
    command = b"tx"

    def __init__(self, protover=PROTO_VERSION):
        super(msg_tx, self).__init__(protover)
        self.tx = CTransaction()

    @classmethod
    def msg_deser(cls, f, protover=PROTO_VERSION):
        c = cls()
        c.tx = CTransaction.stream_deserialize(f)
        return c

    def msg_ser(self, f):
        self.tx.stream_serialize(f)

    def __repr__(self):
        return "msg_tx(tx=%s)" % (repr(self.tx))


class msg_block(MsgSerializable):
    command = b"block"

    def __init__(self, protover=PROTO_VERSION):
        super(msg_block, self).__init__(protover)
        self.block = CBlock()

    @classmethod
    def msg_deser(cls, f, protover=PROTO_VERSION):
        c = cls()
        c.block = CBlock.stream_deserialize(f)
        return c

    def msg_ser(self, f):
        self.block.stream_serialize(f)

    def __repr__(self):
        return "msg_block(block=%s)" % (repr(self.block))


class msg_getaddr(MsgSerializable):
    command = b"getaddr"

    def __init__(self, protover=PROTO_VERSION):
        super(msg_getaddr, self).__init__(protover)

    @classmethod
    def msg_deser(cls, f, protover=PROTO_VERSION):
        return cls()

    def msg_ser(self, f):
        pass

    def __repr__(self):
        return "msg_getaddr()"


class msg_ping(MsgSerializable):
    command = b"ping"

    def __init__(self, protover=PROTO_VERSION, nonce=0):
        super(msg_ping, self).__init__(protover)
        self.nonce = nonce

    @classmethod
    def msg_deser(cls, f, protover=PROTO_VERSION):
        c = cls()
        c.nonce = struct.unpack(b"<Q", ser_read(f, 8))[0]
        return c

    def msg_ser(self, f):
        f.write(struct.pack(b"<Q", self.nonce))

    def __repr__(self):
        return "msg_ping(0x%x)" % (self.nonce,)


class msg_pong(MsgSerializable):
    command = b"pong"

    def __init__(self, protover=PROTO_VERSION, nonce=0):
        super(msg_pong, self).__init__(protover)
        self.nonce = nonce

    @classmethod
    def msg_deser(cls, f, protover=PROTO_VERSION):
        c = cls()
        c.nonce = struct.unpack(b"<Q", ser_read(f,8))[0]
        return c

    def msg_ser(self, f):
        f.write(struct.pack(b"<Q", self.nonce))

    def __repr__(self):
        return "msg_pong(0x%x)" % (self.nonce,)


class msg_reject(MsgSerializable):
    command = b"reject"

    def __init__(self, protover=PROTO_VERSION):
        super(msg_reject, self).__init__(protover)
        self.message = b'(Message Unitialized)'
        self.ccode = b'\0'
        self.reason = b'(Reason Unitialized)'

    @classmethod
    def msg_deser(cls, f, protover=PROTO_VERSION):
        c = cls()
        c.message = VarStringSerializer.stream_deserialize(f)
        c.ccode = struct.unpack(b"<c", ser_read(f,1))[0]
        c.reason = VarStringSerializer.stream_deserialize(f)
        return c

    def msg_ser(self, f):
        VarStringSerializer.stream_serialize(self.message, f)
        f.write(struct.pack(b"<c", self.ccode))
        VarStringSerializer.stream_serialize(self.reason, f)

    def __repr__(self):
        return "msg_reject(messsage=%s, ccode=%s, reason=%s)" % (self.message, self.ccode, self.reason)


class msg_mempool(MsgSerializable):
    command = b"mempool"

    def __init__(self, protover=PROTO_VERSION):
        super(msg_mempool, self).__init__(protover)

    @classmethod
    def msg_deser(cls, f, protover=PROTO_VERSION):
        return cls()

    def msg_ser(self, f):
        pass

    def __repr__(self):
        return "msg_mempool()"

msg_classes = [msg_version, msg_verack, msg_addr, msg_alert, msg_inv,
               msg_getdata, msg_notfound, msg_getblocks, msg_getheaders,
               msg_headers, msg_tx, msg_block, msg_getaddr, msg_ping,
               msg_pong, msg_reject, msg_mempool]

messagemap = {}
for cls in msg_classes:
    messagemap[cls.command] = cls


__all__ = (
        'MSG_TX',
        'MSG_BLOCK',
        'MSG_FILTERED_BLOCK',
        'MSG_CMPCT_BLOCK',
        'MSG_TYPE_MASK',
        'MSG_WITNESS_TX',
        'MSG_WITNESS_BLOCK',
        'MSG_WITNESS_FLAG',
        'MSG_FILTERED_WITNESS_BLOCK',
        'MsgSerializable',
        'msg_version',
        'msg_verack',
        'msg_addr',
        'msg_alert',
        'msg_inv',
        'msg_getdata',
        'msg_getblocks',
        'msg_getheaders',
        'msg_headers',
        'msg_tx',
        'msg_block',
        'msg_getaddr',
        'msg_ping',
        'msg_pong',
        'msg_mempool',
        'msg_classes',
        'messagemap',
)
