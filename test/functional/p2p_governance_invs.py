#!/usr/bin/env python3
# Copyright (c) 2024 The Dash Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

"""
Test inv expiration for governance objects/votes
"""

from test_framework.messages import (
    CInv,
    msg_inv,
    MSG_GOVERNANCE_OBJECT,
    MSG_GOVERNANCE_OBJECT_VOTE,
)
from test_framework.p2p import P2PInterface
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import force_finish_mnsync

RELIABLE_PROPAGATION_TIME = 60  # src/governance/governance.cpp
DATA_CLEANUP_TIME = 5 * 60      # src/init.cpp
MSG_INV_ADDED = 'CGovernanceManager::ConfirmInventoryRequest added {} inv hash to m_requested_hash_time'

class GovernanceInvsTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1

    def run_test(self):
        node = self.nodes[0]
        force_finish_mnsync(node)
        inv = msg_inv([CInv(MSG_GOVERNANCE_OBJECT, 1)])
        self.test_request_expiration(inv, "object")
        inv = msg_inv([CInv(MSG_GOVERNANCE_OBJECT_VOTE, 2)])
        self.test_request_expiration(inv, "vote")

    def test_request_expiration(self, inv, name):
        msg = MSG_INV_ADDED.format(name)
        node = self.nodes[0]
        peer = node.add_p2p_connection(P2PInterface())
        self.log.info(f"Send dummy governance {name} inv and make sure it's added to requested map")
        with node.assert_debug_log([msg]):
            peer.send_message(inv)
        self.log.info(f"Send dummy governance {name} inv again and make sure it's not added because we know about it already")
        with node.assert_debug_log([], [msg]):
            peer.send_message(inv)
        self.log.info("Force internal cleanup")
        with node.assert_debug_log(['UpdateCachesAndClean']):
            node.mockscheduler(DATA_CLEANUP_TIME + 1)
        self.log.info(f"Send dummy governance {name} inv again and make sure it's not added because we still know about it")
        with node.assert_debug_log([], [msg]):
            peer.send_message(inv)
        self.log.info(f"Bump mocktime, force internal cleanup, send dummy governance {name} inv again and make sure it's accepted again")
        self.bump_mocktime(RELIABLE_PROPAGATION_TIME + 1, nodes=[node])
        with node.assert_debug_log(['UpdateCachesAndClean']):
            node.mockscheduler(DATA_CLEANUP_TIME + 1)
        with node.assert_debug_log([msg]):
            peer.send_message(inv)
        node.disconnect_p2ps()


if __name__ == '__main__':
    GovernanceInvsTest().main()
