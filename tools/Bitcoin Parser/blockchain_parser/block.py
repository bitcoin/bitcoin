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

from .transaction import Transaction
from .block_header import BlockHeader
from .utils import format_hash, decode_varint, double_sha256


def get_block_transactions(raw_hex):
    """Given the raw hexadecimal representation of a block,
    yields the block's transactions
    """
    # Skipping the header
    transaction_data = raw_hex[80:]

    # Decoding the number of transactions, offset is the size of
    # the varint (1 to 9 bytes)
    n_transactions, offset = decode_varint(transaction_data)

    for i in range(n_transactions):
        # Try from 1024 (1KiB) -> 1073741824 (1GiB) slice widths
        for j in range(0, 20):
            try:
                offset_e = offset + (1024 * 2 ** j)
                transaction = Transaction.from_hex(
                    transaction_data[offset:offset_e])
                yield transaction
                break
            except:
                continue

        # Skipping to the next transaction
        offset += transaction.size


class Block(object):
    """
    Represents a Bitcoin block, contains its header and its transactions.
    """

    def __init__(self, raw_hex, height=None):
        self.hex = raw_hex
        self._hash = None
        self._transactions = None
        self._header = None
        self._n_transactions = None
        self.size = len(raw_hex)
        self.height = height

    def __repr__(self):
        return "Block(%s)" % self.hash

    @classmethod
    def from_hex(cls, raw_hex):
        """Builds a block object from its bytes representation"""
        return cls(raw_hex)

    @property
    def hash(self):
        """Returns the block's hash (double sha256 of its 80 bytes header"""
        if self._hash is None:
            self._hash = format_hash(double_sha256(self.hex[:80]))
        return self._hash

    @property
    def n_transactions(self):
        """Return the number of transactions contained in this block,
        it is faster to use this than to use len(block.transactions)
        as there's no need to parse all transactions to get this information
        """
        if self._n_transactions is None:
            self._n_transactions = decode_varint(self.hex[80:])[0]

        return self._n_transactions

    @property
    def transactions(self):
        """Returns a list of the block's transactions represented
        as Transaction objects"""
        if self._transactions is None:
            self._transactions = list(get_block_transactions(self.hex))

        return self._transactions

    @property
    def header(self):
        """Returns a BlockHeader object corresponding to this block"""
        if self._header is None:
            self._header = BlockHeader.from_hex(self.hex[:80])
        return self._header
