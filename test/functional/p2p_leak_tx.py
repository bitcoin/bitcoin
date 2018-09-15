#!/usr/bin/env python3
# Copyright (c) 2017-2018 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test that we don't leak txs to inbound peers that we haven't yet announced to"""

from test_framework.messages import msg_getdata, CInv
from test_framework.mininode import mininode_lock, P2PDataStore
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_greater_than,
    wait_until,
)


class P2PLeakTxTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 2

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        gen_node = self.nodes[0]  # The block and tx generating node
        gen_node.generate(1)
        self.sync_all()

        inbound_peer = self.nodes[0].add_p2p_connection(P2PDataStore())  # An "attacking" inbound peer
        outbound_peer = self.nodes[1]  # Our outbound peer

        # In an adversiarial setting we can generally assume that inbound peers
        # are more likely to spy on us than outbound peers. Thus, on average,
        # we announce transactions first to outbound peers, then to (all)
        # inbound peers. Inbound peers must not be able to request the
        # transaction they haven't yet received the announcement for it.
        #
        # With only one outbound peer, we expect that a tx is first announced
        # to (all) inbound peers (and thus present a potential leak) in 28.5% of
        # the cases.
        #
        # Probability( time_ann_inbound < time_ann_outbound )                 =
        # ∫ f_in(x)                           * F_out(x)                   dx =
        # ∫ (lambda_in * exp(-lambda_in * x)) * (1 - exp(-lambda_out * x)) dx =
        # 0.285714
        #
        # Where,
        # * f_in is the pdf of the exponential distribution for inbound peers,
        #   with lambda_in = 1 / INVENTORY_BROADCAST_INTERVAL = 1/5
        # * F_out is the cdf of the expon. distribuiton for outbound peers,
        #   with lambda_out = 1 / (INVENTORY_BROADCAST_INTERVAL >> 1) = 1/2
        #
        # Due to measurement delays, the actual monte-carlo leak is a bit
        # higher. Assume a total delay of 0.6 s (Includes network delays and
        # rpc delay to poll the raw mempool)
        #
        # Probability( time_ann_inbound < time_ann_outbound + 0.6 )           =
        # ∫ f_in(x)                           * F_out(x + 0.6)             dx =
        # ∫ (lambda_in * exp(-lambda_in * x)) * (1 - exp(-lambda_out * (x+.6))) dx =
        # 0.366485
        EXPECTED_MEASURED_LEAK = .366485

        REPEATS = 100
        measured_leak = 0
        self.log.info('Start simulation for {} repeats'.format(REPEATS))
        for i in range(REPEATS):
            self.log.debug('Run {}/{}'.format(i, REPEATS))
            txid = gen_node.sendtoaddress(gen_node.getnewaddress(), 0.033)
            want_tx = msg_getdata()
            want_tx.inv.append(CInv(t=1, h=int(txid, 16)))

            wait_until(lambda: txid in outbound_peer.getrawmempool(), lock=mininode_lock)
            inbound_peer.send_message(want_tx)
            inbound_peer.sync_with_ping()

            if inbound_peer.last_message.get('notfound'):
                assert_equal(inbound_peer.last_message['notfound'].vec[0].hash, int(txid, 16))
                inbound_peer.last_message.pop('notfound')
            else:
                measured_leak += 1

        measured_leak /= REPEATS
        self.log.info('Measured leak of {}'.format(measured_leak))

        assert_greater_than(EXPECTED_MEASURED_LEAK, measured_leak)


if __name__ == '__main__':
    P2PLeakTxTest().main()
