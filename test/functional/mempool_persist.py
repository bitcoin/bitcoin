#!/usr/bin/env python3
# Copyright (c) 2014-2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test mempool persistence.

By default, bitcoind will dump mempool on shutdown and
then reload it on startup. This can be overridden with
the -persistmempool=0 command line option.

Test is as follows:

  - start node0, node1 and node2. node1 has -persistmempool=0
  - create 5 transactions on node2 to its own address. Note that these
    are not sent to node0 or node1 addresses because we don't want
    them to be saved in the wallet.
  - check that node0 and node1 have 5 transactions in their mempools
  - shutdown all nodes.
  - startup node0. Verify that it still has 5 transactions
    in its mempool. Shutdown node0. This tests that by default the
    mempool is persistent.
  - startup node1. Verify that its mempool is empty. Shutdown node1.
    This tests that with -persistmempool=0, the mempool is not
    dumped to disk when the node is shut down.
  - Restart node0 with -persistmempool=0. Verify that its mempool is
    empty. Shutdown node0. This tests that with -persistmempool=0,
    the mempool is not loaded from disk on start up.
  - Restart node0 with -persistmempool. Verify that it has 5
    transactions in its mempool. This tests that -persistmempool=0
    does not overwrite a previously valid mempool stored on disk.
  - Remove node0 mempool.dat and verify savemempool RPC recreates it
    and verify that node1 can load it and has 5 transactions in its
    mempool.
  - Verify that savemempool throws when the RPC is called if
    node1 can't write to disk.

"""
from decimal import Decimal
import os
import time

from test_framework.p2p import P2PTxInvStore
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_greater_than_or_equal,
    assert_raises_rpc_error,
)


class MempoolPersistTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 3
        self.extra_args = [[], ["-persistmempool=0"], []]

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        self.log.debug("Send 5 transactions from node2 (to its own address)")
        tx_creation_time_lower = int(time.time())
        for _ in range(5):
            last_txid = self.nodes[2].sendtoaddress(self.nodes[2].getnewaddress(), Decimal("10"))
        node2_balance = self.nodes[2].getbalance()
        self.sync_all()
        tx_creation_time_higher = int(time.time())

        self.log.debug("Verify that node0 and node1 have 5 transactions in their mempools")
        assert_equal(len(self.nodes[0].getrawmempool()), 5)
        assert_equal(len(self.nodes[1].getrawmempool()), 5)

        total_fee_old = self.nodes[0].getmempoolinfo()['total_fee']

        self.log.debug("Prioritize a transaction on node0")
        fees = self.nodes[0].getmempoolentry(txid=last_txid)['fees']
        assert_equal(fees['base'], fees['modified'])
        self.nodes[0].prioritisetransaction(txid=last_txid, fee_delta=1000)
        fees = self.nodes[0].getmempoolentry(txid=last_txid)['fees']
        assert_equal(fees['base'] + Decimal('0.00001000'), fees['modified'])

        self.log.info('Check the total base fee is unchanged after prioritisetransaction')
        assert_equal(total_fee_old, self.nodes[0].getmempoolinfo()['total_fee'])
        assert_equal(total_fee_old, sum(v['fees']['base'] for k, v in self.nodes[0].getrawmempool(verbose=True).items()))

        tx_creation_time = self.nodes[0].getmempoolentry(txid=last_txid)['time']
        assert_greater_than_or_equal(tx_creation_time, tx_creation_time_lower)
        assert_greater_than_or_equal(tx_creation_time_higher, tx_creation_time)

        # disconnect nodes & make a txn that remains in the unbroadcast set.
        self.disconnect_nodes(0, 1)
        assert(len(self.nodes[0].getpeerinfo()) == 0)
        assert(len(self.nodes[0].p2ps) == 0)
        self.nodes[0].sendtoaddress(self.nodes[2].getnewaddress(), Decimal("12"))
        self.connect_nodes(0, 2)

        self.log.debug("Stop-start the nodes. Verify that node0 has the transactions in its mempool and node1 does not. Verify that node2 calculates its balance correctly after loading wallet transactions.")
        self.stop_nodes()
        # Give this node a head-start, so we can be "extra-sure" that it didn't load anything later
        # Also don't store the mempool, to keep the datadir clean
        self.start_node(1, extra_args=["-persistmempool=0"])
        self.start_node(0)
        self.start_node(2)
        assert self.nodes[0].getmempoolinfo()["loaded"]  # start_node is blocking on the mempool being loaded
        assert self.nodes[2].getmempoolinfo()["loaded"]
        assert_equal(len(self.nodes[0].getrawmempool()), 6)
        assert_equal(len(self.nodes[2].getrawmempool()), 5)
        # The others have loaded their mempool. If node_1 loaded anything, we'd probably notice by now:
        assert_equal(len(self.nodes[1].getrawmempool()), 0)

        self.log.debug('Verify prioritization is loaded correctly')
        fees = self.nodes[0].getmempoolentry(txid=last_txid)['fees']
        assert_equal(fees['base'] + Decimal('0.00001000'), fees['modified'])

        self.log.debug('Verify time is loaded correctly')
        assert_equal(tx_creation_time, self.nodes[0].getmempoolentry(txid=last_txid)['time'])

        # Verify accounting of mempool transactions after restart is correct
        self.nodes[2].syncwithvalidationinterfacequeue()  # Flush mempool to wallet
        assert_equal(node2_balance, self.nodes[2].getbalance())

        # start node0 with wallet disabled so wallet transactions don't get resubmitted
        self.log.debug("Stop-start node0 with -persistmempool=0. Verify that it doesn't load its mempool.dat file.")
        self.stop_nodes()
        self.start_node(0, extra_args=["-persistmempool=0", "-disablewallet"])
        assert self.nodes[0].getmempoolinfo()["loaded"]
        assert_equal(len(self.nodes[0].getrawmempool()), 0)

        self.log.debug("Stop-start node0. Verify that it has the transactions in its mempool.")
        self.stop_nodes()
        self.start_node(0)
        assert self.nodes[0].getmempoolinfo()["loaded"]
        assert_equal(len(self.nodes[0].getrawmempool()), 6)

        mempooldat0 = os.path.join(self.nodes[0].datadir, self.chain, 'mempool.dat')
        mempooldat1 = os.path.join(self.nodes[1].datadir, self.chain, 'mempool.dat')
        self.log.debug("Remove the mempool.dat file. Verify that savemempool to disk via RPC re-creates it")
        os.remove(mempooldat0)
        self.nodes[0].savemempool()
        assert os.path.isfile(mempooldat0)

        self.log.debug("Stop nodes, make node1 use mempool.dat from node0. Verify it has 6 transactions")
        os.rename(mempooldat0, mempooldat1)
        self.stop_nodes()
        self.start_node(1, extra_args=[])
        assert self.nodes[1].getmempoolinfo()["loaded"]
        assert_equal(len(self.nodes[1].getrawmempool()), 6)

        self.log.debug("Prevent bitcoind from writing mempool.dat to disk. Verify that `savemempool` fails")
        # to test the exception we are creating a tmp folder called mempool.dat.new
        # which is an implementation detail that could change and break this test
        mempooldotnew1 = mempooldat1 + '.new'
        os.mkdir(mempooldotnew1)
        assert_raises_rpc_error(-1, "Unable to dump mempool to disk", self.nodes[1].savemempool)
        os.rmdir(mempooldotnew1)

        self.test_persist_unbroadcast()

    def test_persist_unbroadcast(self):
        node0 = self.nodes[0]
        self.start_node(0)

        # clear out mempool
        node0.generate(1, sync_fun=None)

        # ensure node0 doesn't have any connections
        # make a transaction that will remain in the unbroadcast set
        assert(len(node0.getpeerinfo()) == 0)
        assert(len(node0.p2ps) == 0)
        node0.sendtoaddress(self.nodes[1].getnewaddress(), Decimal("12"))

        # shutdown, then startup with wallet disabled
        self.stop_nodes()
        self.start_node(0, extra_args=["-disablewallet"])

        # check that txn gets broadcast due to unbroadcast logic
        conn = node0.add_p2p_connection(P2PTxInvStore())
        node0.mockscheduler(16*60) # 15 min + 1 for buffer
        self.wait_until(lambda: len(conn.get_invs()) == 1)

if __name__ == '__main__':
    MempoolPersistTest().main()
