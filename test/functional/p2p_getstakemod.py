#!/usr/bin/env python3
"""Test getstakemod and stakemod P2P messages."""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.messages import NODE_POS, msg_getstakemod
from test_framework.p2p import P2PInterface, P2P_SERVICES
from test_framework.util import assert_equal

class RawMsg:
    def __init__(self, msgtype, payload=b""):
        self.msgtype = msgtype
        self._payload = payload
    def serialize(self):
        return self._payload

class GetStakeModTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.extra_args = [["-whitelist=127.0.0.1"]]

    def run_test(self):
        services = P2P_SERVICES | NODE_POS
        node = self.nodes[0]

        p2p = node.add_outbound_p2p_connection(P2PInterface(), p2p_idx=0, services=services)
        genesis = int(node.getblockhash(0), 16)
        req = msg_getstakemod(genesis)
        p2p.send_message(req)
        p2p.sync_with_ping()
        p2p.wait_for(lambda: "stakemod" in p2p.last_message)
        assert_equal(p2p.last_message["stakemod"].block_hash, genesis)

        # malformed getstakemod: empty payload
        bad = node.add_outbound_p2p_connection(P2PInterface(), p2p_idx=1, services=services)
        bad.send_message(RawMsg(b"getstakemod"))
        bad.wait_for_disconnect()

        # malformed stakemod: short payload
        bad2 = node.add_outbound_p2p_connection(P2PInterface(), p2p_idx=2, services=services)
        bad2.send_message(RawMsg(b"stakemod", b"\x00"))
        bad2.wait_for_disconnect()

if __name__ == '__main__':
    GetStakeModTest(__file__).main()
