#!/usr/bin/env python3
# Copyright (c) 2025 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test handling of reconciliation messages by a node without reconciliation support
"""

from test_framework.messages import (
    msg_reconcildiff,
    msg_reqtxrcncl,
    msg_reqsketchext,
    msg_sketch
)
from test_framework.p2p import P2PInterface
from test_framework.test_framework import BitcoinTestFramework


class TxReconDisabledTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        # No -txreconciliation

    def assert_disconnect_on_msg(self, message, msgname):
        self.log.info(f"{msgname} from a peer while we do not have reconciliation enabled. Disconnecting")
        peer = self.nodes[0].add_p2p_connection(P2PInterface())
        peer.send_without_ping(message)
        peer.wait_for_disconnect()
        # The node disconnected the peer rather than crashing on the absent tracker.
        assert self.nodes[0].process.poll() is None, f"node crashed handling {msgname}"

    def test_reqtxrcncl(self):
        self.assert_disconnect_on_msg(msg_reqtxrcncl(), "REQTXRCNCL")

    def test_sketch(self):
        self.assert_disconnect_on_msg(msg_sketch(), "SKETCH")

    def test_reqsketchext(self):
        self.assert_disconnect_on_msg(msg_reqsketchext(), "REQSKETCHEXT")

    def test_reconcildiff(self):
        self.assert_disconnect_on_msg(msg_reconcildiff(), "RECONCILDIFF")

    def run_test(self):
        self.test_reqtxrcncl()
        self.test_sketch()
        self.test_reqsketchext()
        self.test_reconcildiff()


if __name__ == '__main__':
    TxReconDisabledTest(__file__).main()
