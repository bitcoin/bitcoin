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

from .utils import decode_varint, decode_uint64
from .script import Script
from .address import Address


class Output(object):
    """Represents a Transaction output"""

    def __init__(self, raw_hex):
        self._value = None
        self._script = None
        self._addresses = None

        script_length, varint_size = decode_varint(raw_hex[8:])
        script_start = 8 + varint_size

        self._script_hex = raw_hex[script_start:script_start+script_length]
        self.size = script_start + script_length
        self._value_hex = raw_hex[:8]

    @classmethod
    def from_hex(cls, hex_):
        return cls(hex_)

    def __repr__(self):
        return "Output(satoshis=%d)" % self.value

    @property
    def value(self):
        """Returns the value of the output exprimed in satoshis"""
        if self._value is None:
            self._value = decode_uint64(self._value_hex)
        return self._value

    @property
    def script(self):
        """Returns the output's script as a Script object"""
        if self._script is None:
            self._script = Script.from_hex(self._script_hex)
        return self._script

    @property
    def addresses(self):
        """Returns a list containinng all the addresses mentionned
        in the output's script
        """
        if self._addresses is None:
            self._addresses = []
            if self.type == "pubkey":
                address = Address.from_public_key(self.script.operations[0])
                self._addresses.append(address)
            elif self.type == "pubkeyhash":
                address = Address.from_ripemd160(self.script.operations[2])
                self._addresses.append(address)
            elif self.type == "p2sh":
                address = Address.from_ripemd160(self.script.operations[1],
                                                 type="p2sh")
                self._addresses.append(address)
            elif self.type == "multisig":
                n = self.script.operations[-2]
                for operation in self.script.operations[1:1+n]:
                    self._addresses.append(Address.from_public_key(operation))

        return self._addresses

    def is_return(self):
        return self.script.is_return()

    def is_p2sh(self):
        return self.script.is_p2sh()

    def is_pubkey(self):
        return self.script.is_pubkey()

    def is_pubkeyhash(self):
        return self.script.is_pubkeyhash()

    def is_multisig(self):
        return self.script.is_multisig()

    def is_unknown(self):
        return self.script.is_unknown()

    @property
    def type(self):
        """Returns the output's script type as a string"""
        if self.is_pubkeyhash():
            return "pubkeyhash"

        if self.is_pubkey():
            return "pubkey"

        if self.is_p2sh():
            return "p2sh"

        if self.is_multisig():
            return "multisig"

        if self.is_return():
            return "OP_RETURN"

        return "unknown"
