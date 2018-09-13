#!/usr/bin/env python3
# Copyright (c) 2014-2018 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test gettxoutproof and verifytxoutproof RPCs."""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import *
from test_framework.mininode import FromHex, ToHex
from test_framework.messages import CMerkleBlock

class MerkleBlockTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 4
        self.setup_clean_chain = True
        # Nodes 0/1 are "wallet" nodes, Nodes 2/3 are used for testing
        self.extra_args = [[], [], [], ["-txindex"]]

    def setup_network(self):
        self.setup_nodes()
        connect_nodes(self.nodes[0], 1)
        connect_nodes(self.nodes[0], 2)
        connect_nodes(self.nodes[0], 3)

        self.sync_all()

    def run_test(self):
        self.log.info("Mining blocks...")
        self.nodes[0].generate(105)
        self.sync_all()

        chain_height = self.nodes[1].getblockcount()
        assert_equal(chain_height, 105)
        assert_equal(self.nodes[1].getbalance(), 0)
        assert_equal(self.nodes[2].getbalance(), 0)

        node0utxos = self.nodes[0].listunspent(1)
        tx1 = self.nodes[0].createrawtransaction([node0utxos.pop()], {self.nodes[1].getnewaddress(): 49.99})
        txid1 = self.nodes[0].sendrawtransaction(self.nodes[0].signrawtransactionwithwallet(tx1)["hex"])
        tx2 = self.nodes[0].createrawtransaction([node0utxos.pop()], {self.nodes[1].getnewaddress(): 49.99})
        txid2 = self.nodes[0].sendrawtransaction(self.nodes[0].signrawtransactionwithwallet(tx2)["hex"])
        # This will raise an exception because the transaction is not yet in a block
        assert_raises_rpc_error(-5, "Transaction not yet in block", self.nodes[0].gettxoutproof, [txid1])

        self.nodes[0].generate(1)
        blockhash = self.nodes[0].getblockhash(chain_height + 1)
        self.sync_all()

        txlist = []
        blocktxn = self.nodes[0].getblock(blockhash, True)["tx"]
        txlist.append(blocktxn[1])
        txlist.append(blocktxn[2])

        assert_equal(self.nodes[2].verifytxoutproof(self.nodes[2].gettxoutproof([txid1])), [txid1])
        assert_equal(self.nodes[2].verifytxoutproof(self.nodes[2].gettxoutproof([txid1, txid2])), txlist)
        assert_equal(self.nodes[2].verifytxoutproof(self.nodes[2].gettxoutproof([txid1, txid2], blockhash)), txlist)

        txin_spent = self.nodes[1].listunspent(1).pop()
        tx3 = self.nodes[1].createrawtransaction([txin_spent], {self.nodes[0].getnewaddress(): 49.98})
        txid3 = self.nodes[0].sendrawtransaction(self.nodes[1].signrawtransactionwithwallet(tx3)["hex"])
        self.nodes[0].generate(1)
        self.sync_all()

        txid_spent = txin_spent["txid"]
        txid_unspent = txid1 if txin_spent["txid"] != txid1 else txid2

        # We can't find the block from a fully-spent tx
        assert_raises_rpc_error(-5, "Transaction not yet in block", self.nodes[2].gettxoutproof, [txid_spent])
        # We can get the proof if we specify the block
        assert_equal(self.nodes[2].verifytxoutproof(self.nodes[2].gettxoutproof([txid_spent], blockhash)), [txid_spent])
        # We can't get the proof if we specify a non-existent block
        assert_raises_rpc_error(-5, "Block not found", self.nodes[2].gettxoutproof, [txid_spent], "00000000000000000000000000000000")
        # We can get the proof if the transaction is unspent
        assert_equal(self.nodes[2].verifytxoutproof(self.nodes[2].gettxoutproof([txid_unspent])), [txid_unspent])
        # We can get the proof if we provide a list of transactions and one of them is unspent. The ordering of the list should not matter.
        assert_equal(sorted(self.nodes[2].verifytxoutproof(self.nodes[2].gettxoutproof([txid1, txid2]))), sorted(txlist))
        assert_equal(sorted(self.nodes[2].verifytxoutproof(self.nodes[2].gettxoutproof([txid2, txid1]))), sorted(txlist))
        # We can always get a proof if we have a -txindex
        assert_equal(self.nodes[2].verifytxoutproof(self.nodes[3].gettxoutproof([txid_spent])), [txid_spent])
        # We can't get a proof if we specify transactions from different blocks
        assert_raises_rpc_error(-5, "Not all transactions found in specified or retrieved block", self.nodes[2].gettxoutproof, [txid1, txid3])

        # Now we'll try tweaking a proof.
        proof = self.nodes[3].gettxoutproof([txid1, txid2])
        assert txid1 in self.nodes[0].verifytxoutproof(proof)
        assert txid2 in self.nodes[1].verifytxoutproof(proof)

        tweaked_proof = FromHex(CMerkleBlock(), proof)

        # Make sure that our serialization/deserialization is working
        assert txid1 in self.nodes[2].verifytxoutproof(ToHex(tweaked_proof))

        # Check to see if we can go up the merkle tree and pass this off as a
        # single-transaction block
        tweaked_proof.txn.nTransactions = 1
        tweaked_proof.txn.vHash = [tweaked_proof.header.hashMerkleRoot]
        tweaked_proof.txn.vBits = [True] + [False]*7

        for n in self.nodes:
            assert not n.verifytxoutproof(ToHex(tweaked_proof))

        # TODO: try more variants, eg transactions at different depths, and
        # verify that the proofs are invalid

if __name__ == '__main__':
    MerkleBlockTest().main()
