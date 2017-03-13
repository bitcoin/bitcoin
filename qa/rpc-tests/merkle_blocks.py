#!/usr/bin/env python3
# Copyright (c) 2014-2016 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test gettxoutproof and verifytxoutproof RPCs."""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import *

class MerkleBlockTest(BitcoinTestFramework):

    def __init__(self):
        super().__init__()
        self.setup_clean_chain = True
        self.num_nodes = 4

    def setup_network(self):
        self.nodes = []
        # Nodes 0/1 are "wallet" nodes
        self.nodes.append(start_node(0, self.options.tmpdir))
        self.nodes.append(start_node(1, self.options.tmpdir))
        # Nodes 2/3 are used for testing
        self.nodes.append(start_node(2, self.options.tmpdir))
        self.nodes.append(start_node(3, self.options.tmpdir, ["-txindex"]))
        connect_nodes(self.nodes[0], 1)
        connect_nodes(self.nodes[0], 2)
        connect_nodes(self.nodes[0], 3)

        self.is_network_split = False
        self.sync_all()

    def run_test(self):
        self.log.info("Mining blocks...")
        self.nodes[0].generate(nblocks=105)
        self.sync_all()

        chain_height = self.nodes[1].getblockcount()
        assert_equal(chain_height, 105)
        assert_equal(self.nodes[1].getbalance(), 0)
        assert_equal(self.nodes[2].getbalance(), 0)

        node0utxos = self.nodes[0].listunspent(minconf=1)
        tx1 = self.nodes[0].createrawtransaction(transactions=[node0utxos.pop()], outputs={self.nodes[1].getnewaddress(): 49.99})
        txid1 = self.nodes[0].sendrawtransaction(hexstring=self.nodes[0].signrawtransaction(hexstring=tx1)["hex"])
        tx2 = self.nodes[0].createrawtransaction(transactions=[node0utxos.pop()], outputs={self.nodes[1].getnewaddress(): 49.99})
        txid2 = self.nodes[0].sendrawtransaction(hexstring=self.nodes[0].signrawtransaction(hexstring=tx2)["hex"])
        assert_raises(JSONRPCException, self.nodes[0].gettxoutproof, txids=[txid1])

        self.nodes[0].generate(nblocks=1)
        blockhash = self.nodes[0].getblockhash(height=chain_height + 1)
        self.sync_all()

        txlist = []
        blocktxn = self.nodes[0].getblock(blockhash=blockhash, verbose=True)["tx"]
        txlist.append(blocktxn[1])
        txlist.append(blocktxn[2])

        assert_equal(self.nodes[2].verifytxoutproof(proof=self.nodes[2].gettxoutproof(txids=[txid1])), [txid1])
        assert_equal(self.nodes[2].verifytxoutproof(proof=self.nodes[2].gettxoutproof(txids=[txid1, txid2])), txlist)
        assert_equal(self.nodes[2].verifytxoutproof(proof=self.nodes[2].gettxoutproof(txids=[txid1, txid2], blockhash=blockhash)), txlist)

        txin_spent = self.nodes[1].listunspent(minconf=1).pop()
        tx3 = self.nodes[1].createrawtransaction(transactions=[txin_spent], outputs={self.nodes[0].getnewaddress(): 49.98})
        self.nodes[0].sendrawtransaction(hexstring=self.nodes[1].signrawtransaction(hexstring=tx3)["hex"])
        self.nodes[0].generate(nblocks=1)
        self.sync_all()

        txid_spent = txin_spent["txid"]
        txid_unspent = txid1 if txin_spent["txid"] != txid1 else txid2

        # We can't find the block from a fully-spent tx
        assert_raises(JSONRPCException, self.nodes[2].gettxoutproof, txids=[txid_spent])
        # ...but we can if we specify the block
        assert_equal(self.nodes[2].verifytxoutproof(proof=self.nodes[2].gettxoutproof(txids=[txid_spent], blockhash=blockhash)), [txid_spent])
        # ...or if the first tx is not fully-spent
        assert_equal(self.nodes[2].verifytxoutproof(proof=self.nodes[2].gettxoutproof(txids=[txid_unspent])), [txid_unspent])
        try:
            assert_equal(self.nodes[2].verifytxoutproof(proof=self.nodes[2].gettxoutproof(txids=[txid1, txid2])), txlist)
        except JSONRPCException:
            assert_equal(self.nodes[2].verifytxoutproof(proof=self.nodes[2].gettxoutproof(txids=[txid2, txid1])), txlist)
        # ...or if we have a -txindex
        assert_equal(self.nodes[2].verifytxoutproof(proof=self.nodes[3].gettxoutproof(txids=[txid_spent])), [txid_spent])

if __name__ == '__main__':
    MerkleBlockTest().main()
