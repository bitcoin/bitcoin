#!/usr/bin/env python3
# Copyright (c) 2019-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

""" Test node eviction logic

When the number of peers has reached the limit of maximum connections,
the next connecting inbound peer will trigger the eviction mechanism.
We cannot currently test the parts of the eviction logic that are based on
address/netgroup since in the current framework, all peers are connecting from
the same local address. See Issue #14210 for more info.
Therefore, this test is limited to the remaining protection criteria.
"""
import time

from test_framework.blocktools import (
    create_block,
)
from test_framework.messages import (
    msg_pong,
    msg_tx,
)
from test_framework.p2p import (
    P2PDataStore,
    P2PInterface,
)
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_greater_than,
)
from test_framework.wallet import MiniWallet


class DelayedPongMixin:
    """Hold back the pong to the node's ping, so the test can answer it after bumping mocktime."""
    ping_nonce = None

    def on_ping(self, message):
        self.ping_nonce = message.nonce


class DelayedPongP2PInterface(DelayedPongMixin, P2PInterface):
    pass


class DelayedPongP2PDataStore(DelayedPongMixin, P2PDataStore):
    pass


class P2PEvict(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        # The choice of maxconnections=53 results in a maximum of 21 tx-relaying inbound connections
        # (53 - 10 outbound - 1 feeler) * 0.5 = 21. The other inbound slots are reserved for block-relay-only
        # peers that don't play a role in this test.
        # 20 inbound peers are protected from eviction:
        # 4 by netgroup, 4 that sent us blocks, 4 that sent us transactions and 8 via lowest ping time
        self.extra_args = [['-maxconnections=53']]

    def add_peer(self, peer, ping_time):
        """Connect peer with the given min ping time and return its index in node.p2ps and getpeerinfo."""
        node = self.nodes[0]
        node.add_p2p_connection(peer)
        peer_id = len(node.p2ps) - 1
        if ping_time > 0:
            node.bumpmocktime(ping_time)
            peer.send_without_ping(msg_pong(peer.ping_nonce))
        self.wait_until(lambda: "minping" in node.getpeerinfo()[peer_id])
        assert_equal(node.getpeerinfo()[peer_id]["minping"], ping_time)
        # Give every peer a distinct connection time, so the order of connections is well defined
        node.bumpmocktime(1)
        return peer_id

    def run_test(self):
        protected_peers = set()  # peers that we expect to be protected from eviction
        node = self.nodes[0]
        node.setmocktime(int(time.time()))
        self.wallet = MiniWallet(node)

        # Netgroup protection picks 4 arbitrary peers here (all peers share one netgroup,
        # so the choice depends on the std::sort implementation). Whatever it takes from
        # another group is made up from the eviction candidates: they connect first, so
        # tx- and block-time protection fall back to them as the oldest connections, and
        # they ping faster than the tx and block peers. Removing any protection then gets
        # one of its peers evicted, unless netgroup protection covered all of them.
        self.log.info("Create 5 peers that will be eviction candidates")
        for _ in range(5):
            self.add_peer(DelayedPongP2PInterface(), ping_time=1)

        self.log.info("Create 4 peers and protect them from eviction by sending us a tx")
        for _ in range(4):
            txpeer = DelayedPongP2PDataStore()
            peer_id = self.add_peer(txpeer, ping_time=2)
            txpeer.sync_with_ping()

            tx = self.wallet.create_self_transfer()['tx']
            txpeer.send_and_ping(msg_tx(tx))
            assert_greater_than(node.getpeerinfo()[peer_id]["last_transaction"], 0)
            protected_peers.add(peer_id)

        self.log.info("Create 4 peers and protect them from eviction by sending us a block")
        for _ in range(4):
            block_peer = DelayedPongP2PDataStore()
            peer_id = self.add_peer(block_peer, ping_time=2)
            block_peer.sync_with_ping()
            best_block = node.getbestblockhash()
            tip = int(best_block, 16)
            best_block_time = node.getblock(best_block)['time']
            block = create_block(tip, height=node.getblockcount() + 1, ntime=best_block_time + 1)
            block.solve()
            block_peer.send_blocks_and_test([block], node, success=True)
            assert_greater_than(node.getpeerinfo()[peer_id]["last_block"], 0)
            protected_peers.add(peer_id)

        self.log.info("Create 8 peers and protect them from eviction by having faster pings")
        for _ in range(8):
            protected_peers.add(self.add_peer(P2PInterface(), ping_time=0))

        self.log.info("Create peer that triggers the eviction mechanism")
        node.add_p2p_connection(P2PInterface())

        # One of the non-protected peers must be evicted. We can't be sure which one because
        # 4 peers are protected via netgroup, which is identical for all peers,
        # and the eviction mechanism doesn't preserve the order of identical elements.
        def get_evicted_peers():
            return [i for i, peer in enumerate(node.p2ps) if not peer.is_connected]

        self.wait_until(lambda: len(get_evicted_peers()) >= 1)  # wait for socket close
        evicted_peers = get_evicted_peers()

        self.log.info("Test that one peer was evicted")
        self.log.debug("{} evicted peer: {}".format(len(evicted_peers), set(evicted_peers)))
        assert_equal(len(evicted_peers), 1)

        self.log.info("Test that no peer expected to be protected was evicted")
        self.log.debug("{} protected peers: {}".format(len(protected_peers), protected_peers))
        assert evicted_peers[0] not in protected_peers


if __name__ == '__main__':
    P2PEvict(__file__).main()
