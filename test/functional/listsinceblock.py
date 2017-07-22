#!/usr/bin/env python3
# Copyright (c) 2017 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the listsincelast RPC."""

from test_framework.test_framework import BitcoinTestFramework
from decimal import Decimal
from test_framework.util import assert_equal, start_nodes, sync_mempools

# Sequence number that is BIP 125 opt-in and BIP 68-compliant
BIP125_SEQUENCE_NUMBER = 0xfffffffd

class ListSinceBlockTest (BitcoinTestFramework):

    def __init__(self):
        super().__init__()
        self.setup_clean_chain = True
        self.num_nodes = 4

    def run_test (self):
        self.list_since_reorg()
        self.double_spends_filtered()

    def list_since_reorg (self):
        '''
        `listsinceblock` did not behave correctly when handed a block that was
        no longer in the main chain:

             ab0
          /       \
        aa1 [tx0]   bb1
         |           |
        aa2         bb2
         |           |
        aa3         bb3
                     |
                    bb4

        Consider a client that has only seen block `aa3` above. It asks the node
        to `listsinceblock aa3`. But at some point prior the main chain switched
        to the bb chain.

        Previously: listsinceblock would find height=4 for block aa3 and compare
        this to height=5 for the tip of the chain (bb4). It would then return
        results restricted to bb3-bb4.

        Now: listsinceblock finds the fork at ab0 and returns results in the
        range bb1-bb4.

        This test only checks that [tx0] is present.
        '''

        self.nodes[2].generate(101)
        self.sync_all()

        assert_equal(self.nodes[0].getbalance(), 0)
        assert_equal(self.nodes[1].getbalance(), 0)
        assert_equal(self.nodes[2].getbalance(), 50)
        assert_equal(self.nodes[3].getbalance(), 0)

        # Split network into two
        self.split_network()

        # send to nodes[0] from nodes[2]
        senttx = self.nodes[2].sendtoaddress(self.nodes[0].getnewaddress(), 1)

        # generate on both sides
        lastblockhash = self.nodes[1].generate(6)[5]
        self.nodes[2].generate(7)
        self.log.info('lastblockhash=%s' % (lastblockhash))

        self.sync_all([self.nodes[:2], self.nodes[2:]])

        self.join_network()

        # listsinceblock(lastblockhash) should now include tx, as seen from nodes[0]
        lsbres = self.nodes[0].listsinceblock(lastblockhash)
        found = False
        for tx in lsbres['transactions']:
            if tx['txid'] == senttx:
                found = True
                break
        assert_equal(found, True)

    def double_spends_filtered(self):
        '''
        `listsinceblock` was returning conflicted transactions even if they 
        occurred before the specified cutoff blockhash
        '''
        spending_node = self.nodes[2];
        dest_address = spending_node.getnewaddress()

        tx_input = dict(
            sequence=BIP125_SEQUENCE_NUMBER, **next(u for u in spending_node.listunspent()))
        rawtx = spending_node.createrawtransaction(
            [tx_input], {dest_address: tx_input["amount"] - Decimal("0.00051000"),
                         spending_node.getrawchangeaddress(): Decimal("0.00050000")})
        signedtx = spending_node.signrawtransaction(rawtx)
        orig_tx_id = spending_node.sendrawtransaction(signedtx["hex"])
        original_tx = spending_node.gettransaction(orig_tx_id)

        double_tx = spending_node.bumpfee(orig_tx_id);

        # check that both transactions exist
        block_hash = spending_node.listsinceblock(
            spending_node.getblockhash(spending_node.getblockcount()))
        original_found = False;
        double_found = False
        for tx in block_hash['transactions']:
            if tx['txid'] == original_tx['txid']:
                original_found = True
            if tx['txid'] == double_tx['txid']:
                double_found = True
        assert_equal(original_found, True)
        assert_equal(double_found, True)

        lastblockhash = spending_node.generate(6)[5]

        # check that neither transaction exists
        block_hash = spending_node.listsinceblock(lastblockhash)
        original_found = False;
        double_found = False
        for tx in block_hash['transactions']:
            if tx['txid'] == original_tx['txid']:
                original_found = True
            if tx['txid'] == double_tx['txid']:
                double_found = True
        assert_equal(original_found, False)
        assert_equal(double_found, False)

if __name__ == '__main__':
    ListSinceBlockTest().main()
