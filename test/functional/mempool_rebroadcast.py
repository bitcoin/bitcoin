#!/usr/bin/env python3
# Copyright (c) 2009-2019 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test mempool rebroadcast logic.

"""

from test_framework.mininode import P2PTxInvStore
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
        assert_equal,
        assert_greater_than,
        wait_until,
        disconnect_nodes,
        connect_nodes,
        gen_return_txouts,
        create_confirmed_utxos,
        create_lots_of_big_transactions,
)
import time

# Constant from txmempool.h
MAX_REBROADCAST_WEIGHT = 3000000

class MempoolRebroadcastTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 2
        self.extra_args = [[
            "-acceptnonstdtxn=1",
            "-blockmaxweight=3000000",
            "-whitelist=127.0.0.1"
            ]] * self.num_nodes

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        self.test_simple_rebroadcast()
        self.test_rebroadcast_top_txns()
        self.test_recency_filter()

    # helper method that uses getblocktemplate with node arg
    # set to MAX_REBROADCAST_WEIGHT to find txns expected to
    # be rebroadcast
    def find_top_txns(self, node):
        tmpl = node.getblocktemplate({'rules': ['segwit']})

        tx_hshs = []
        for tx in tmpl['transactions']:
            tx_hshs.append(tx['hash'])

        return tx_hshs

    def compare_txns_to_invs(self, txn_hshs, invs):
        tx_ids = [int(txhsh, 16) for txhsh in txn_hshs]

        assert_equal(len(tx_ids), len(invs))
        assert_equal(tx_ids.sort(), invs.sort())

    def test_simple_rebroadcast(self):
        self.log.info("Test simplest rebroadcast case")

        node0 = self.nodes[0]
        node1 = self.nodes[1]

        # generate mempool transactions that both nodes know about
        for _ in range(3):
            node0.sendtoaddress(node1.getnewaddress(), 4)

        self.sync_all()

        # generate mempool transactions that only node0 knows about
        disconnect_nodes(node0, 1)

        for _ in range(3):
            node0.sendtoaddress(node1.getnewaddress(), 5)

        # check that mempools are different
        assert_equal(len(node0.getrawmempool()), 6)
        assert_equal(len(node1.getrawmempool()), 3)

        # reconnect the nodes
        connect_nodes(node0, 1)

        # rebroadcast will only occur if there has been a block since the
        # last run of CacheMinRebroadcastFee. when we connect a new peer, rebroadcast
        # will be skipped on the first run, but caching will trigger.
        # have node1 generate so there are still mempool txns that need to be synched.
        node1.generate(1)

        assert_equal(len(node1.getrawmempool()), 0)
        wait_until(lambda: len(node0.getrawmempool()) == 3)

        # bump time to hit rebroadcast interval
        mocktime = int(time.time()) + 300 * 60
        node0.setmocktime(mocktime)
        node1.setmocktime(mocktime)

        # check that node1 got txns bc rebroadcasting
        wait_until(lambda: len(node1.getrawmempool()) == 3, timeout=30)

    def test_rebroadcast_top_txns(self):
        self.log.info("Testing that only txns with top fee rate get rebroadcast")

        node = self.nodes[0]
        node.setmocktime(0)

        # mine a block to clear out the mempool
        node.generate(1)
        assert_equal(len(node.getrawmempool()), 0)

        conn1 = node.add_p2p_connection(P2PTxInvStore())

        # create txns
        min_relay_fee = node.getnetworkinfo()["relayfee"]
        txouts = gen_return_txouts()
        utxo_count = 90
        utxos = create_confirmed_utxos(min_relay_fee, node, utxo_count)
        base_fee = min_relay_fee*100 # our transactions are smaller than 100kb
        txids = []

        # Create 3 batches of transactions at 3 different fee rate levels
        range_size = utxo_count // 3

        for i in range(3):
            start_range = i * range_size
            end_range = start_range + range_size
            txids.append(create_lots_of_big_transactions(node, txouts, utxos[start_range:end_range], end_range - start_range, (i+1)*base_fee))

        # 90 transactions should be created
        # confirm the invs were sent (initial broadcast)
        assert_equal(len(node.getrawmempool()), 90)
        wait_until(lambda: len(conn1.tx_invs_received) == 90)

        # confirm txns are more than max rebroadcast amount
        assert_greater_than(node.getmempoolinfo()['bytes'], MAX_REBROADCAST_WEIGHT)

        # age txns to ensure they won't be excluded due to recency filter
        mocktime = int(time.time()) + 31 * 60
        node.setmocktime(mocktime)

        # add another p2p connection since txns aren't rebroadcast to the same peer (see filterInventoryKnown)
        conn2 = node.add_p2p_connection(P2PTxInvStore())

        # trigger rebroadcast to occur
        mocktime += 300 * 60 # seconds
        node.setmocktime(mocktime)
        time.sleep(1) # ensure send message thread runs so invs get sent

        # `nNextInvSend` delay on `setInventoryTxToSend
        wait_until(lambda: conn2.get_invs(), timeout=30)

        global global_mocktime
        global_mocktime = mocktime

    def test_recency_filter(self):
        self.log.info("Test recent txns don't get rebroadcast")

        node = self.nodes[0]
        node1 = self.nodes[1]

        node.setmocktime(0)

        # mine blocks to clear out the mempool
        node.generate(10)
        assert_equal(len(node.getrawmempool()), 0)

        # add p2p connection
        conn = node.add_p2p_connection(P2PTxInvStore())

        # create old txn
        node.sendtoaddress(node.getnewaddress(), 2)
        assert_equal(len(node.getrawmempool()), 1)
        wait_until(lambda: conn.get_invs(), timeout=30)

        # bump mocktime to ensure the txn is old
        mocktime = int(time.time()) + 31 * 60 # seconds
        node.setmocktime(mocktime)

        delta_time = 28 * 60 # seconds
        while True:
            # create a recent transaction
            new_tx = node1.sendtoaddress(node1.getnewaddress(), 2)
            new_tx_id = int(new_tx, 16)

            # ensure node0 has the transaction
            wait_until(lambda: new_tx in node.getrawmempool())

            # add another p2p connection since txns aren't rebroadcast
            # to the same peer (see filterInventoryKnown)
            new_conn = node.add_p2p_connection(P2PTxInvStore())

            # bump mocktime to try to get rebroadcast,
            # but not so much that the txn would be old
            mocktime += delta_time
            node.setmocktime(mocktime)

            time.sleep(1.1)

            # once we get any rebroadcasts, ensure the most recent txn is not included
            if new_conn.get_invs():
                assert(new_tx_id not in new_conn.get_invs())
                break

if __name__ == '__main__':
    MempoolRebroadcastTest().main()

