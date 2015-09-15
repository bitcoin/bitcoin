#!/usr/bin/env python2
# Copyright (c) 2014 The Bitcoin Core developers
# Distributed under the MIT/X11 software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.


from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import *

class RemoveTest(BitcoinTestFramework):
    def setup_chain(self):
        print("Initializing test directory "+self.options.tmpdir)
        initialize_chain_clean(self.options.tmpdir, 3)

    def setup_network(self, split=False):
        self.nodes = start_nodes(3, self.options.tmpdir, [["-relaypriority=false"], ["-relaypriority=false"], ["-relaypriority=false"]])
        connect_nodes(self.nodes[0], 1)
        connect_nodes(self.nodes[1], 2)
        self.is_network_split = False
        self.sync_all()

    def run_test (self):
        print("Mining blocks...")
        self.nodes[2].generate(105)
        self.nodes[2].sendtoaddress(self.nodes[1].getnewaddress(), 11)
        self.nodes[2].sendtoaddress(self.nodes[1].getnewaddress(), 42)
        self.nodes[2].sendtoaddress(self.nodes[1].getnewaddress(), 15)
        self.nodes[2].generate(1)
        self.sync_all()

        node1utxos = self.nodes[1].listunspent(1)

        utxo = node1utxos.pop()
        utxo2 = node1utxos.pop()
        txto1_1_raw = self.nodes[1].createrawtransaction([{"txid": utxo["txid"], "vout": utxo["vout"]}, {"txid": utxo2["txid"], "vout": utxo2["vout"]}], {self.nodes[1].getnewaddress(): utxo["amount"] + utxo2["amount"]})
        txto1_1_raw = self.nodes[1].signrawtransaction(txto1_1_raw)["hex"]
        txto1_1_hash = self.nodes[1].sendrawtransaction(txto1_1_raw)

        utxo3 = node1utxos.pop()
        txto1_2_addr = self.nodes[1].getnewaddress()
        txto1_2_raw = self.nodes[1].createrawtransaction([{"txid": txto1_1_hash, "vout": 0}, {"txid": utxo3["txid"], "vout": utxo3["vout"]}], {txto1_2_addr: utxo["amount"], self.nodes[1].getnewaddress(): utxo2["amount"], self.nodes[2].getnewaddress(): utxo3["amount"]})
        txto1_2_raw = self.nodes[1].signrawtransaction(txto1_2_raw)["hex"]
        txto1_2_hash = self.nodes[1].sendrawtransaction(txto1_2_raw)

        txto1_2_vout = 0 if self.nodes[1].getrawtransaction(txto1_2_hash, 1)["vout"][0]["scriptPubKey"]["addresses"][0] == txto1_2_addr else 1
        txto1_2_vout = txto1_2_vout if self.nodes[1].getrawtransaction(txto1_2_hash, 1)["vout"][2]["scriptPubKey"]["addresses"][0] != txto1_2_addr else 2
        txto0_1_raw = self.nodes[1].createrawtransaction([{"txid": txto1_2_hash, "vout": txto1_2_vout}], {self.nodes[0].getnewaddress(): utxo["amount"]})
        txto0_1_raw = self.nodes[1].signrawtransaction(txto0_1_raw)["hex"]
        txto0_1_hash = self.nodes[1].sendrawtransaction(txto0_1_raw, True)

        self.sync_all()

        txto0_2_raw = self.nodes[0].createrawtransaction([{"txid": txto0_1_hash, "vout": 0}], {self.nodes[0].getnewaddress(): utxo["amount"] - 1, self.nodes[1].getnewaddress(): 1})
        txto0_2_raw = self.nodes[0].signrawtransaction(txto0_2_raw)["hex"]
        txto0_2_hash = self.nodes[0].sendrawtransaction(txto0_2_raw)

        self.sync_all()
        # txto0_2_raw should be untrusted as it depends on untrusted, unconfirmed inputs
        #XXX: assert_equal(self.nodes[0].getbalance(), 0)
        #XXX: assert_equal(self.nodes[0].getunconfirmedbalance(), utxo["amount"] - 1)
        assert_equal(self.nodes[1].getbalance(), utxo2["amount"])
        assert_equal(self.nodes[1].getunconfirmedbalance(), 1)

        stop_node(self.nodes[0],0)
        self.nodes[0] = start_node(0, self.options.tmpdir, ["-debug", "-relaypriority=false"])
        stop_node(self.nodes[1],1)
        self.nodes[1] = start_node(1, self.options.tmpdir, ["-debug", "-relaypriority=false"])
        stop_node(self.nodes[2],2)
        self.nodes[2] = start_node(2, self.options.tmpdir, ["-debug", "-relaypriority=false"])

        self.nodes[0].getbalance()
        self.nodes[0].getunconfirmedbalance()
        self.nodes[1].getbalance()
        self.nodes[1].getunconfirmedbalance()

        txto1_1_ds_node2_addr = self.nodes[2].getnewaddress()
        txto1_1_ds_raw = self.nodes[1].createrawtransaction([{"txid": utxo["txid"], "vout": utxo["vout"]}, {"txid": utxo3["txid"], "vout": utxo3["vout"]}], {self.nodes[1].getnewaddress(): utxo["amount"], txto1_1_ds_node2_addr: utxo3["amount"]})
        txto1_1_ds_raw = self.nodes[1].signrawtransaction(txto1_1_ds_raw)["hex"]
        txto1_1_ds_hash = self.nodes[2].sendrawtransaction(txto1_1_ds_raw)
        self.nodes[2].generate(1)

        connect_nodes(self.nodes[1], 2)
        connect_nodes(self.nodes[0], 1)
        self.sync_all()

        # We are now entirely conflicted and have no spendable outputs
        assert_equal(self.nodes[0].getbalance(), 0)
        assert_equal(self.nodes[0].getunconfirmedbalance(), 0)
        assert_equal(self.nodes[0].gettransaction(txto0_1_hash)["confirmations"], -1)
        assert_equal(self.nodes[0].gettransaction(txto0_2_hash)["confirmations"], -1)

        assert_equal(self.nodes[1].getbalance(), utxo["amount"] + utxo2["amount"])
        assert_equal(self.nodes[1].getunconfirmedbalance(), 0)

        stop_node(self.nodes[2],2)
        self.nodes[2] = start_node(2, self.options.tmpdir, ["-debug", "-relaypriority=false"])

        txto1_1_ds_vout = 1 if self.nodes[1].getrawtransaction(txto1_1_ds_hash, 1)["vout"][0]["scriptPubKey"]["addresses"][0] == txto1_1_ds_node2_addr else 0
        txto0_3_raw = self.nodes[1].createrawtransaction([{"txid": txto1_1_ds_hash, "vout": txto1_1_ds_vout}], {self.nodes[0].getnewaddress(): utxo["amount"]})
        txto0_3_raw = self.nodes[1].signrawtransaction(txto0_3_raw)["hex"]
        txto0_3_ds_raw = self.nodes[1].createrawtransaction([{"txid": txto1_1_ds_hash, "vout": txto1_1_ds_vout}], {self.nodes[1].getnewaddress(): utxo["amount"]})
        txto0_3_ds_raw = self.nodes[1].signrawtransaction(txto0_3_ds_raw)["hex"]

        txto0_3_hash = self.nodes[1].sendrawtransaction(txto0_3_raw)
        self.nodes[1].generate(1)

        assert_equal(self.nodes[1].getbalance(), utxo2["amount"])
        assert_equal(self.nodes[1].getunconfirmedbalance(), 0)

        sync_blocks([self.nodes[0], self.nodes[1]])
        txto1_3_hash = self.nodes[0].sendtoaddress(self.nodes[1].getnewaddress(), 1)
        #self.nodes[0].generate(1)
        assert(self.nodes[0].getbalance() > utxo["amount"] - 2 and self.nodes[0].getbalance() < utxo["amount"] - 1)
        assert_equal(self.nodes[0].getunconfirmedbalance(), 0)

        txto0_3_ds_hash = self.nodes[2].sendrawtransaction(txto0_3_ds_raw)
        self.nodes[2].generate(2)

        connect_nodes(self.nodes[1], 2)
        self.sync_all()

        # We are now entirely conflicted and have no spendable outputs
        assert_equal(self.nodes[0].getbalance(), 0)
        assert_equal(self.nodes[0].getunconfirmedbalance(), 0)
        assert_equal(self.nodes[0].gettransaction(txto0_3_hash)["confirmations"], -1)
        assert_equal(self.nodes[0].gettransaction(txto1_3_hash)["confirmations"], -1)

        assert_equal(self.nodes[1].getbalance(), utxo["amount"] + utxo2["amount"])
        assert_equal(self.nodes[1].getunconfirmedbalance(), 0)


if __name__ == '__main__':
    RemoveTest().main ()
