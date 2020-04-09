# Copyright (C) 2013-2014 The python-bitcoinlib developers
#
# This file is part of python-bitcoinlib.
#
# It is subject to the license terms in the LICENSE file found in the top-level
# directory of this distribution.
#
# No part of python-bitcoinlib, including this file, may be copied, modified,
# propagated, or distributed except according to the terms contained in the
# LICENSE file.

import unittest

from bitcoin.net import CAddress, CAlert, CUnsignedAlert, CInv

# Py3 compatibility
import sys

if sys.version > '3':
    from io import BytesIO as _BytesIO
else:
    from cStringIO import StringIO as _BytesIO


class TestUnsignedAlert(unittest.TestCase):
    def test_serialization(self):
        alert = CUnsignedAlert()
        alert.setCancel = [1, 2, 3]
        alert.strComment = b"Comment"
        stream = _BytesIO()

        alert.stream_serialize(stream)
        serialized = _BytesIO(stream.getvalue())

        deserialized = CUnsignedAlert.stream_deserialize(serialized)
        self.assertEqual(deserialized, alert)


class TestCAlert(unittest.TestCase):
    def test_serialization(self):
        alert = CAlert()
        alert.setCancel = [1, 2, 3]
        alert.strComment = b"Comment"
        stream = _BytesIO()

        alert.stream_serialize(stream)
        serialized = _BytesIO(stream.getvalue())

        deserialized = CAlert.stream_deserialize(serialized)
        self.assertEqual(deserialized, alert)


class TestCInv(unittest.TestCase):
    def test_serialization(self):
        inv = CInv()
        inv.type = 123
        inv.hash = b"0" * 32
        stream = _BytesIO()

        inv.stream_serialize(stream)
        serialized = _BytesIO(stream.getvalue())

        deserialized = CInv.stream_deserialize(serialized)
        self.assertEqual(deserialized, inv)


class TestCAddress(unittest.TestCase):
    def test_serializationSimple(self):
        c = CAddress()
        cSerialized = c.serialize()
        cDeserialized = CAddress.deserialize(cSerialized)
        cSerializedTwice = cDeserialized.serialize()
        self.assertEqual(cSerialized, cSerializedTwice)

    def test_serializationIPv4(self):
        c = CAddress()
        c.ip = "1.1.1.1"
        c.port = 8333
        c.nTime = 1420576401

        cSerialized = c.serialize()
        cDeserialized = CAddress.deserialize(cSerialized)

        self.assertEqual(c, cDeserialized)

        cSerializedTwice = cDeserialized.serialize()
        self.assertEqual(cSerialized, cSerializedTwice)

    def test_serializationIPv6(self):
        c = CAddress()
        c.ip = "1234:ABCD:1234:ABCD:1234:00:ABCD:1234"
        c.port = 8333
        c.nTime = 1420576401

        cSerialized = c.serialize()
        cDeserialized = CAddress.deserialize(cSerialized)

        self.assertEqual(c, cDeserialized)

        cSerializedTwice = cDeserialized.serialize()
        self.assertEqual(cSerialized, cSerializedTwice)

    def test_serializationDiff(self):
        # Sanity check that the serialization code preserves differences
        c1 = CAddress()
        c1.ip = "1.1.1.1"
        c1.port = 8333
        c1.nTime = 1420576401

        c2 = CAddress()
        c2.ip = "1.1.1.2"
        c2.port = 8333
        c2.nTime = 1420576401

        self.assertNotEqual(c1, c2)

        c1Serialized = c1.serialize()
        c2Serialized = c2.serialize()

        self.assertNotEqual(c1Serialized, c2Serialized)

        c1Deserialized = CAddress.deserialize(c1Serialized)
        c2Deserialized = CAddress.deserialize(c2Serialized)

        self.assertNotEqual(c1Deserialized, c2Deserialized)
