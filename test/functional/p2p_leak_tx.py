#!/usr/bin/env python3
# Copyright (c) 2017-2018 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test that we don't leak txs to inbound peers that we haven't yet announced to"""

from test_framework.messages import msg_getdata, CInv
from test_framework.mininode import P2PDataStore
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
)

import time


class P2PNode(P2PDataStore):
    def on_inv(self, msg):
        pass


class P2PLeakTxTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        gen_node = self.nodes[0]  # The block and tx generating node
        gen_node.generate(1)
        self.sync_all()

        inbound_peer = self.nodes[0].add_p2p_connection(P2PNode())  # An "attacking" inbound peer

        REPEATS = 10
        self.log.info('Start test for {} repeats'.format(REPEATS))
        for i in range(REPEATS):
            self.log.debug('Run {}/{}'.format(i, REPEATS))
            txid = gen_node.sendtoaddress(gen_node.getnewaddress(), 0.033)

            time.sleep(5)

            want_tx = msg_getdata()
            want_tx.inv.append(CInv(t=1, h=int(txid, 16)))
            inbound_peer.send_message(want_tx)
            inbound_peer.sync_with_ping()

            if inbound_peer.last_message.get('notfound'):
                self.log.debug('tx {} was not yet announced to us.'.format(txid))
                assert_equal(inbound_peer.last_message['notfound'].vec[0].hash, int(txid, 16))
                inbound_peer.last_message.pop('notfound')
            else:
                self.log.debug('tx {} was announced to us.'.format(txid))
                assert int(txid, 16) in [inv.hash for inv in inbound_peer.last_message['inv'].inv]


if __name__ == '__main__':
    P2PLeakTxTest().main()
