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

import sys
from blockchain_parser.blockchain import Blockchain
from blockchain_parser.script import CScriptInvalidError


def is_ascii_text(op):
    return all(32 <= x <= 127 for x in op)


blockchain = Blockchain(sys.argv[1])
for block in blockchain.get_unordered_blocks():
    for transaction in block.transactions:
        coinbase = transaction.inputs[0]

        # Some coinbase scripts are not valid scripts
        try:
            script_operations = coinbase.script.operations
        except CScriptInvalidError:
            break

        # An operation is a CScriptOP or pushed bytes
        for operation in script_operations:
            if type(operation) == bytes and len(operation) > 3 \
                    and is_ascii_text(operation):
                print(block.header.timestamp, operation.decode("ascii"))
        break
