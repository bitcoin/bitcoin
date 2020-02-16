# Copyright (C) 2015-2016 The bitcoin-blockchain-parser developers
#
# This file is part of bitcoin-blockchain-parser.
#
# It is subject to the license terms in the LICENSE file found in the top-level
# directory of this distribution.
#
# No part of bitcoin-blockchain-parser, including this file, may be copied,
# modified, propagated, or distributed except according to the terms contained
# in the LICENSE file.

from binascii import hexlify
import hashlib
import struct


def btc_ripemd160(data):
    h1 = hashlib.sha256(data).digest()
    r160 = hashlib.new("ripemd160")
    r160.update(h1)
    return r160.digest()


def double_sha256(data):
    return hashlib.sha256(hashlib.sha256(data).digest()).digest()


def format_hash(hash_):
    return str(hexlify(hash_[::-1]).decode("utf-8"))


def decode_uint32(data):
    assert(len(data) == 4)
    return struct.unpack("<I", data)[0]


def decode_uint64(data):
    assert(len(data) == 8)
    return struct.unpack("<Q", data)[0]


def decode_varint(data):
    assert(len(data) > 0)
    size = int(data[0])
    assert(size <= 255)

    if size < 253:
        return size, 1

    if size == 253:
        format_ = '<H'
    elif size == 254:
        format_ = '<I'
    elif size == 255:
        format_ = '<Q'
    else:
        # Should never be reached
        assert 0, "unknown format_ for size : %s" % size

    size = struct.calcsize(format_)
    return struct.unpack(format_, data[1:size+1])[0], size + 1
