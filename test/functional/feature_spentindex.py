#!/usr/bin/env python3
# Copyright (c) 2014-2015 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

#
# Test addressindex generation and fetching
#

import binascii
from decimal import Decimal

from test_framework.messages import COIN, COutPoint, CTransaction, CTxIn, CTxOut
from test_framework.script import CScript, OP_CHECKSIG, OP_DUP, OP_EQUALVERIFY, OP_HASH160
from test_framework.test_node import ErrorMatch
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal, connect_nodes


class SpentIndexTest(BitcoinTestFramework):

    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 4

    def setup_network(self):
        self.add_nodes(self.num_nodes)
        # Nodes 0/1 are "wallet" nodes
        self.start_node(0)
        self.start_node(1, ["-spentindex"])
        # Nodes 2/3 are used for testing
        self.start_node(2, ["-spentindex"])
        self.start_node(3, ["-spentindex", "-txindex"])
        connect_nodes(self.nodes[0], 1)
        connect_nodes(self.nodes[0], 2)
        connect_nodes(self.nodes[0], 3)

        self.sync_all()

    def run_test(self):
        self.log.info("Test that settings can't be changed without -reindex...")
        self.stop_node(1)
        self.nodes[1].assert_start_raises_init_error(["-spentindex=0"], "You need to rebuild the database using -reindex to change -spentindex", match=ErrorMatch.PARTIAL_REGEX)
        self.start_node(1, ["-spentindex=0", "-reindex"])
        connect_nodes(self.nodes[0], 1)
        self.sync_all()
        self.stop_node(1)
        self.nodes[1].assert_start_raises_init_error(["-spentindex"], "You need to rebuild the database using -reindex to change -spentindex", match=ErrorMatch.PARTIAL_REGEX)
        self.start_node(1, ["-spentindex", "-reindex"])
        connect_nodes(self.nodes[0], 1)
        self.sync_all()

        self.log.info("Mining blocks...")
        self.nodes[0].generate(105)
        self.sync_all()

        chain_height = self.nodes[1].getblockcount()
        assert_equal(chain_height, 105)

        # Check that
        self.log.info("Testing spent index...")

        privkey = "cU4zhap7nPJAWeMFu4j6jLrfPmqakDAzy8zn8Fhb3oEevdm4e5Lc"
        addressHash = binascii.unhexlify("C5E4FB9171C22409809A3E8047A29C83886E325D")
        scriptPubKey = CScript([OP_DUP, OP_HASH160, addressHash, OP_EQUALVERIFY, OP_CHECKSIG])
        unspent = self.nodes[0].listunspent()
        tx = CTransaction()
        tx_fee = Decimal('0.00001')
        tx_fee_sat = int(tx_fee * COIN)
        amount = int(unspent[0]["amount"] * COIN) - tx_fee_sat
        tx.vin = [CTxIn(COutPoint(int(unspent[0]["txid"], 16), unspent[0]["vout"]))]
        tx.vout = [CTxOut(amount, scriptPubKey)]
        tx.rehash()

        signed_tx = self.nodes[0].signrawtransactionwithwallet(binascii.hexlify(tx.serialize()).decode("utf-8"))
        txid = self.nodes[0].sendrawtransaction(signed_tx["hex"], True)
        self.nodes[0].generate(1)
        self.sync_all()

        self.log.info("Testing getspentinfo method...")

        # Check that the spentinfo works standalone
        info = self.nodes[1].getspentinfo({"txid": unspent[0]["txid"], "index": unspent[0]["vout"]})
        assert_equal(info["txid"], txid)
        assert_equal(info["index"], 0)
        assert_equal(info["height"], 106)

        self.log.info("Testing getrawtransaction method...")

        # Check that verbose raw transaction includes spent info
        txVerbose = self.nodes[3].getrawtransaction(unspent[0]["txid"], 1)
        assert_equal(txVerbose["vout"][unspent[0]["vout"]]["spentTxId"], txid)
        assert_equal(txVerbose["vout"][unspent[0]["vout"]]["spentIndex"], 0)
        assert_equal(txVerbose["vout"][unspent[0]["vout"]]["spentHeight"], 106)

        # Check that verbose raw transaction includes input values
        txVerbose2 = self.nodes[3].getrawtransaction(txid, 1)
        assert_equal(txVerbose2["vin"][0]["value"], Decimal(unspent[0]["amount"]))
        assert_equal(txVerbose2["vin"][0]["valueSat"] - tx_fee_sat, amount)

        # Check that verbose raw transaction includes address values and input values
        address2 = "yeMpGzMj3rhtnz48XsfpB8itPHhHtgxLc3"
        addressHash2 = binascii.unhexlify("C5E4FB9171C22409809A3E8047A29C83886E325D")
        scriptPubKey2 = CScript([OP_DUP, OP_HASH160, addressHash2, OP_EQUALVERIFY, OP_CHECKSIG])
        tx2 = CTransaction()
        tx2.vin = [CTxIn(COutPoint(int(txid, 16), 0))]
        tx2.vout = [CTxOut(amount - int(COIN / 10), scriptPubKey2)]
        tx2.rehash()
        self.nodes[0].importprivkey(privkey)
        signed_tx2 = self.nodes[0].signrawtransactionwithwallet(binascii.hexlify(tx2.serialize()).decode("utf-8"))
        txid2 = self.nodes[0].sendrawtransaction(signed_tx2["hex"], True)

        # Check the mempool index
        self.sync_all()
        txVerbose3 = self.nodes[1].getrawtransaction(txid2, 1)
        assert_equal(txVerbose3["vin"][0]["address"], address2)
        assert_equal(txVerbose3["vin"][0]["value"], Decimal(unspent[0]["amount"]) - tx_fee)
        assert_equal(txVerbose3["vin"][0]["valueSat"], amount)

        # Check the database index
        self.nodes[0].generate(1)
        self.sync_all()

        txVerbose4 = self.nodes[3].getrawtransaction(txid2, 1)
        assert_equal(txVerbose4["vin"][0]["address"], address2)
        assert_equal(txVerbose4["vin"][0]["value"], Decimal(unspent[0]["amount"]) - tx_fee)
        assert_equal(txVerbose4["vin"][0]["valueSat"], amount)

        self.log.info("Passed")


if __name__ == '__main__':
    SpentIndexTest().main()
