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

from bitcoin.core.script import *
from binascii import b2a_hex


def is_public_key(hex_data):
    """Given a bytes string, returns whether its is probably a bitcoin
    public key or not. It doesn't actually ensures that the data is a valid
    public key, justs that it looks like one
    """
    if type(hex_data) != bytes:
        return False

    # Uncompressed public key
    if len(hex_data) == 65 and int(hex_data[0]) == 4:
        return True

    # Compressed public key
    if len(hex_data) == 33 and int(hex_data[0]) in [2, 3]:
        return True

    return False


class Script(object):
    """Represents a bitcoin script contained in an input or output"""

    def __init__(self, raw_hex):
        self.hex = raw_hex
        self._script = None
        self._type = None
        self._value = None
        self._operations = None
        self._addresses = None

    @classmethod
    def from_hex(cls, hex_):
        return cls(hex_)

    def __repr__(self):
        return "Script(%s)" % self.value

    @property
    def script(self):
        """Returns the underlying CScript object"""
        if self._script is None:
            self._script = CScript(self.hex)

        return self._script

    @property
    def operations(self):
        """Returns the list of operations done by this script,
        an operation is one of :
           - a CScriptOP
           - bytes data pushed to the stack
           - an int pushed to the stack
        If the script is invalid (some coinbase scripts are), an exception is
        thrown
        """
        if self._operations is None:
            # Some coinbase scripts are garbage, they could not be valid
            self._operations = list(self.script)

        return self._operations

    @property
    def value(self):
        """Returns a string representation of the script"""
        if self._value is None:
            parts = []
            try:
                for operation in self.operations:
                    if isinstance(operation, bytes):
                        parts.append(b2a_hex(operation).decode("ascii"))
                    else:
                        parts.append(str(operation))

                self._value = " ".join(parts)
            except CScriptInvalidError:
                self._value = "INVALID_SCRIPT"

        return self._value

    def is_return(self):
        return self.script.is_unspendable()

    def is_p2sh(self):
        return self.script.is_p2sh()

    def is_pubkey(self):
        return len(self.operations) == 2 \
            and self.operations[-1] == OP_CHECKSIG \
            and is_public_key(self.operations[0])

    def is_pubkeyhash(self):
        return len(self.hex) == 25 \
            and self.operations[0] == OP_DUP \
            and self.operations[1] == OP_HASH160 \
            and self.operations[-2] == OP_EQUALVERIFY \
            and self.operations[-1] == OP_CHECKSIG

    def is_multisig(self):
        if len(self.operations) < 4:
            return False
        m = self.operations[0]

        if not isinstance(m, int):
            return False

        for i in range(m):
            if not is_public_key(self.operations[1+i]):
                return False

        n = self.operations[-2]
        if not isinstance(n, int) or n < m \
                or self.operations[-1] != OP_CHECKMULTISIG:
            return False

        return True

    def is_unknown(self):
        return not self.is_pubkeyhash() and not self.is_pubkey() \
            and not self.is_p2sh() and not self.is_multisig() \
            and not self.is_return()
