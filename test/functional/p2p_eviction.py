#!/usr/bin/env python3
# Copyright (c) 2019 The Bitcoin Core developers
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

from test_framework.blocktools import create_block, create_coinbase
from test_framework.messages import CTransaction, FromHex, msg_pong, msg_tx
from test_framework.p2p import P2PDataStore, P2PInterface
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal


class SlowP2PDataStore(P2PDataStore):
    def on_ping(self, message):
        time.sleep(0.1)
        self.send_message(msg_pong(message.nonce))

class SlowP2PInterface(P2PInterface):
    def on_ping(self, message):
        time.sleep(0.1)
        self.send_message(msg_pong(message.nonce))

class P2PEvict(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1
        # The choice of maxconnections=32 results in a maximum of 21 inbound connections
        # (32 - 10 outbound - 1 feeler). 20 inbound peers are protected from eviction:
        # 4 by netgroup, 4 that sent us blocks, 4 that sent us transactions and 8 via lowest ping time
        self.extra_args = [['-maxconnections=32']]

    def run_test(self):
        protected_peers = set()  # peers that we expect to be protected from eviction
        current_peer = -1
        node = self.nodes[0]
        node.generatetoaddress(101, node.get_deterministic_priv_key().address)

        self.log.info("Create 4 peers and protect them from eviction by sending us a block")
        for _ in range(4):
            block_peer = node.add_p2p_connection(SlowP2PDataStore())
            current_peer += 1
            block_peer.sync_with_ping()
            best_block = node.getbestblockhash()
            tip = int(best_block, 16)
            best_block_time = node.getblock(best_block)['time']
            block = create_block(tip, create_coinbase(node.getblockcount() + 1), best_block_time + 1, version=0x20000000)
            block.solve()
            block_peer.send_blocks_and_test([block], node, success=True)
            protected_peers.add(current_peer)

        self.log.info("Create 5 slow-pinging peers, making them eviction candidates")
        for _ in range(5):
            node.add_p2p_connection(SlowP2PInterface())
            current_peer += 1

        self.log.info("Create 4 peers and protect them from eviction by sending us a tx")
        for i in range(4):
            txpeer = node.add_p2p_connection(SlowP2PInterface())
            current_peer += 1
            txpeer.sync_with_ping()

            prevtx = node.getblock(node.getblockhash(i + 1), 2)['tx'][0]
            rawtx = node.createrawtransaction(
                inputs=[{'txid': prevtx['txid'], 'vout': 0}],
                outputs=[{node.get_deterministic_priv_key().address: 50 - 0.00125}],
            )
            sigtx = node.signrawtransactionwithkey(
                hexstring=rawtx,
                privkeys=[node.get_deterministic_priv_key().key],
                prevtxs=[{
                    'txid': prevtx['txid'],
                    'vout': 0,
                    'scriptPubKey': prevtx['vout'][0]['scriptPubKey']['hex'],
                }],
            )['hex']
            txpeer.send_message(msg_tx(FromHex(CTransaction(), sigtx)))
            protected_peers.add(current_peer)

        self.log.info("Create 8 peers and protect them from eviction by having faster pings")
        for _ in range(8):
            fastpeer = node.add_p2p_connection(P2PInterface())
            current_peer += 1
            self.wait_until(lambda: "ping" in fastpeer.last_message, timeout=10)

        # Make sure by asking the node what the actual min pings are
        peerinfo = node.getpeerinfo()
        pings = {}
        for i in range(len(peerinfo)):
            pings[i] = peerinfo[i]['minping'] if 'minping' in peerinfo[i] else 1000000
        sorted_pings = sorted(pings.items(), key=lambda x: x[1])

        # Usually the 8 fast peers are protected. In rare case of unreliable pings,
        # one of the slower peers might have a faster min ping though.
        for i in range(8):
            protected_peers.add(sorted_pings[i][0])

        self.log.info("Create peer that triggers the eviction mechanism")
        node.add_p2p_connection(SlowP2PInterface())

        # One of the non-protected peers must be evicted. We can't be sure which one because
        # 4 peers are protected via netgroup, which is identical for all peers,
        # and the eviction mechanism doesn't preserve the order of identical elements.
        evicted_peers = []
        for i in range(len(node.p2ps)):
            if not node.p2ps[i].is_connected:
                evicted_peers.append(i)

        self.log.info("Test that one peer was evicted")
        self.log.debug("{} evicted peer: {}".format(len(evicted_peers), set(evicted_peers)))
        assert_equal(len(evicted_peers), 1)

        self.log.info("Test that no peer expected to be protected was evicted")
        self.log.debug("{} protected peers: {}".format(len(protected_peers), protected_peers))
        assert evicted_peers[0] not in protected_peers

if __name__ == '__main__':
    P2PEvict().main()
