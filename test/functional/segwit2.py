#!/usr/bin/env python3
# Copyright (c) 2016 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the SegWit changeover logic."""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import *
from test_framework.mininode import sha256, CTransaction, CTxIn, COutPoint, CTxOut, COIN, ToHex, FromHex
from test_framework.address import script_to_p2sh, key_to_p2pkh
from test_framework.script import CScript, OP_HASH160, OP_CHECKSIG, OP_0, hash160, OP_EQUAL, OP_DUP, OP_EQUALVERIFY, OP_1, OP_2, OP_CHECKMULTISIG, OP_TRUE
from io import BytesIO

class SegWitTest2(BitcoinTestFramework):

    def __init__(self):
        super().__init__()
        self.setup_clean_chain = True
        self.num_nodes = 1
        self.extra_args = [["-walletprematurewitness", "-prematurewitness"]]

    def run_test(self):
        self.nodes[0].generate(101)
        sync_blocks(self.nodes)

        use_p2wsh = False

        transaction = ""
        scriptPubKey = ""

        if use_p2wsh:
            scriptPubKey = "00203a59f3f56b713fdcf5d1a57357f02c44342cbf306ffe0c4741046837bf90561a"
            transaction = "01000000000100e1f505000000002200203a59f3f56b713fdcf5d1a57357f02c44342cbf306ffe0c4741046837bf90561a00000000"
        else:
            scriptPubKey = "a9142f8c469c2f0084c48e11f998ffbe7efa7549f26d87"
            transaction = "01000000000100e1f5050000000017a9142f8c469c2f0084c48e11f998ffbe7efa7549f26d8700000000"
            
        self.nodes[0].importaddress(scriptPubKey, "", False)
        rawtxfund = self.nodes[0].fundrawtransaction(transaction)['hex']

        rawtxfund = self.nodes[0].signrawtransaction(rawtxfund)["hex"]
        txid = self.nodes[0].decoderawtransaction(rawtxfund)["txid"]

        self.nodes[0].sendrawtransaction(rawtxfund)

        assert(self.nodes[0].gettransaction(txid, True)["txid"]  == txid)
        assert(self.nodes[0].listtransactions("*", 1, 0, True)[0]["txid"]  == txid)
        
        # Assert it is properly saved
        self.stop_nodes()
        self.nodes = self.start_nodes(self.num_nodes, self.options.tmpdir, self.extra_args)

        assert(self.nodes[0].gettransaction(txid, True)["txid"]  == txid)
        assert(self.nodes[0].listtransactions("*", 1, 0, True)[0]["txid"]  == txid)

if __name__ == '__main__':
    SegWitTest2().main()
