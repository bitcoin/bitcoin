#!/usr/bin/env python3
# Copyright (c) 2015-2018 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test node responses to invalid transactions.

In this test we connect to one node over p2p, and test tx requests."""
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    connect_nodes,
    disconnect_nodes,
)
import time

class GetDataFailureTest(BitcoinTestFramework):
    NUM_ADDR = 25

    def set_test_params(self):
        self.num_nodes = 2
        self.setup_clean_chain = True

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def peerinfo_interesting(self, node):
        pi = node.getpeerinfo()
        mpi = node.getmempoolinfo()
        return mpi["size"],pi[0]["tx_inflight"], pi[0]["tx_process"]

    def run_test(self):
        node_work,node_miss = self.nodes  # convenience reference to the nodes

        mt = int(time.time())
        node_miss.setmocktime(mt)
        node_work.setmocktime(mt)

        node_work.generate(2) # 100 BTC mature
        self.sync_all()
        node_miss.generate(100)

        self.log.info('Setup %d wallet addresses' % self.NUM_ADDR)
        addr = [node_work.getnewaddress() for i in range(self.NUM_ADDR)]
        self.sync_all()

        self.log.info('Setup with %d utxos to spend' % (5*self.NUM_ADDR))
        assert(5*self.NUM_ADDR*0.351 < 50)
        node_work.sendmany("", {a: 0.351 for a in addr})
        node_work.sendmany("", {a: 0.351 for a in addr})
        node_work.sendmany("", {a: 0.351 for a in addr})
        node_work.sendmany("", {a: 0.351 for a in addr})
        node_work.sendmany("", {a: 0.351 for a in addr})

        self.sync_all()
        node_miss.generate(1)
        self.sync_all()
        assert_equal(node_work.getmempoolinfo()["size"], 0)
        assert_equal(node_miss.getmempoolinfo()["size"], 0)

        self.log.info('Disconnect node_miss, create 100 transactions')
        disconnect_nodes(self.nodes[0], 1)
        utxos = []
        for i in range(100):
            rawtx = node_work.createrawtransaction( [], [{addr[0]: 0.2}] )
            rawtxf = node_work.fundrawtransaction(rawtx)
            vout = 1-rawtxf["changepos"]
            rawtxs = node_work.signrawtransactionwithwallet(rawtxf["hex"])
            txid = node_work.sendrawtransaction(rawtxs["hex"])
            utxos.append( (txid, vout) )

        self.log.info('Bump time by 20 minutes, reconnect nodes')
        mt += 20*60 # bump time by 20 minutes
        node_miss.setmocktime(mt)
        node_work.setmocktime(mt)

        connect_nodes(self.nodes[0], 1)

        self.log.info('Create %d txs with parents in mempool for >15m' % (len(utxos)))
        for txid,vout in utxos:
            rawtx = node_work.createrawtransaction( [{"txid":txid, "vout":vout}], [{addr[0]: 0.199}] )
            rawtxs = node_work.signrawtransactionwithwallet(rawtx)
            txid = node_work.sendrawtransaction(rawtxs["hex"])

        # time to sync
        time.sleep(25)

        self.log.info("Check node_miss didn't see any transactions, but isn't confused either")
        assert_equal(self.peerinfo_interesting(node_miss), (0, 0, 0))
        assert_equal(self.peerinfo_interesting(node_work), (200, 0, 0))

        txid = node_miss.sendtoaddress(addr[0], 0.5)
        self.log.info("Check node_miss transactions still work (%s)" % (txid))
        time.sleep(25)
        assert_equal(self.peerinfo_interesting(node_miss), (1, 0, 0))
        assert_equal(self.peerinfo_interesting(node_work), (201, 0, 0))

        txid = node_work.sendtoaddress(addr[0], 30)
        self.log.info("Check legit node_work transactions still work too (%s)" % (txid))
        time.sleep(25)
        assert_equal(self.peerinfo_interesting(node_miss), (2, 0, 0))
        assert_equal(self.peerinfo_interesting(node_work), (202, 0, 0))

if __name__ == '__main__':
    GetDataFailureTest().main()
