#!/usr/bin/env python3
# Copyright (c) 2014-2015 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

#
# Test txindex generation and fetching
#

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import *
from test_framework.script import *
from test_framework.mininode import *
import binascii

class TxIndexTest(BitcoinTestFramework):

    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 4

    def setup_network(self):
        self.add_nodes(self.num_nodes)
        # Nodes 0/1 are "wallet" nodes
        self.start_node(0)
        self.start_node(1, ["-txindex"])
        # Nodes 2/3 are used for testing
        self.start_node(2, ["-txindex"])
        self.start_node(3, ["-txindex"])
        connect_nodes(self.nodes[0], 1)
        connect_nodes(self.nodes[0], 2)
        connect_nodes(self.nodes[0], 3)

        self.is_network_split = False
        self.sync_all()

    def run_test(self):
        self.log.info("Mining blocks...")
        self.nodes[0].generate(105)
        self.sync_all()

        chain_height = self.nodes[1].getblockcount()
        assert_equal(chain_height, 105)

        self.log.info("Testing transaction index...")

        addressHash = binascii.unhexlify("C5E4FB9171C22409809A3E8047A29C83886E325D")
        scriptPubKey = CScript([OP_DUP, OP_HASH160, addressHash, OP_EQUALVERIFY, OP_CHECKSIG])
        unspent = self.nodes[0].listunspent()
        tx = CTransaction()
        tx_fee_sat = 1000
        amount = int(unspent[0]["amount"] * 100000000) - tx_fee_sat
        tx.vin = [CTxIn(COutPoint(int(unspent[0]["txid"], 16), unspent[0]["vout"]))]
        tx.vout = [CTxOut(amount, scriptPubKey)]
        tx.rehash()

        signed_tx = self.nodes[0].signrawtransactionwithwallet(binascii.hexlify(tx.serialize()).decode("utf-8"))
        txid = self.nodes[0].sendrawtransaction(signed_tx["hex"], True)
        self.nodes[0].generate(1)
        self.sync_all()

        # Check verbose raw transaction results
        verbose = self.nodes[3].getrawtransaction(txid, 1)
        assert_equal(verbose["vout"][0]["valueSat"], 50000000000 - tx_fee_sat)
        assert_equal(verbose["vout"][0]["value"] * 100000000, 50000000000 - tx_fee_sat)

        self.log.info("Passed")


if __name__ == '__main__':
    TxIndexTest().main()
