# Copyright (C) 2012-2014 The python-bitcoinlib developers
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

from bitcoin.core.serialize import *

# Py3 compatibility
import sys

if sys.version > '3':
    from io import BytesIO as _BytesIO
else:
    from cStringIO import StringIO as _BytesIO


class DERSignature(ImmutableSerializable):
    __slots__ = ['length', 'r', 's']

    def __init__(self, r, s, length):
        object.__setattr__(self, 'r', r)
        object.__setattr__(self, 's', s)
        object.__setattr__(self, 'length', length)

    @classmethod
    def stream_deserialize(cls, f):
        assert ser_read(f, 1) == b"\x30"
        rs = BytesSerializer.stream_deserialize(f)
        f = _BytesIO(rs)
        assert ser_read(f, 1) == b"\x02"
        r = BytesSerializer.stream_deserialize(f)
        assert ser_read(f, 1) == b"\x02"
        s = BytesSerializer.stream_deserialize(f)
        return cls(r, s, len(r + s))

    def stream_serialize(self, f):
        f.write(b"\x30")
        f.write(b"\x02")
        BytesSerializer.stream_serialize(self.r, f)
        f.write(b"\x30")
        BytesSerializer.stream_serialize(self.s, f)

    def __repr__(self):
        return 'DERSignature(%s, %s)' % (self.r, self.s)


__all__ = (
    'DERSignature',
)
