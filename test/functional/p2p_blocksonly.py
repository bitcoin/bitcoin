#!/usr/bin/env python3
# Copyright (c) 2019-2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test p2p blocksonly mode & block-relay-only connections."""

import time

from test_framework.messages import msg_tx, msg_inv, CInv, MSG_WTX
from test_framework.p2p import P2PInterface, P2PTxInvStore
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal
from test_framework.wallet import MiniWallet


class P2PBlocksOnly(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.extra_args = [["-blocksonly"]]

    def run_test(self):
        self.miniwallet = MiniWallet(self.nodes[0])

        self.blocksonly_mode_tests()
        self.blocks_relay_conn_tests()

    def blocksonly_mode_tests(self):
        self.log.info("Tests with node running in -blocksonly mode")
        assert_equal(self.nodes[0].getnetworkinfo()['localrelay'], False)

        self.nodes[0].add_p2p_connection(P2PInterface())
        tx, txid, wtxid, tx_hex = self.check_p2p_tx_violation()

        self.log.info('Check that tx invs also violate the protocol')
        self.nodes[0].add_p2p_connection(P2PInterface())
        with self.nodes[0].assert_debug_log(['transaction (0000000000000000000000000000000000000000000000000000000000001234) inv sent in violation of protocol, disconnecting peer']):
            self.nodes[0].p2ps[0].send_without_ping(msg_inv([CInv(t=MSG_WTX, h=0x1234)]))
            self.nodes[0].p2ps[0].wait_for_disconnect()
            del self.nodes[0].p2ps[0]

        self.log.info('Check that txs from rpc are not rejected and relayed to other peers')
        tx_relay_peer = self.nodes[0].add_p2p_connection(P2PInterface())
        assert_equal(self.nodes[0].getpeerinfo()[0]['relaytxes'], True)

        assert_equal(self.nodes[0].testmempoolaccept([tx_hex])[0]['allowed'], True)
        with self.nodes[0].assert_debug_log(['received getdata for: wtx {} peer'.format(wtxid)]):
            self.nodes[0].sendrawtransaction(tx_hex)
            tx_relay_peer.wait_for_tx(txid)
            assert_equal(self.nodes[0].getmempoolinfo()['size'], 1)

        self.log.info("Restarting node 0 with relay permission and blocksonly")
        self.restart_node(0, ["-persistmempool=0", "-whitelist=relay@127.0.0.1", "-blocksonly"])
        assert_equal(self.nodes[0].getrawmempool(), [])
        first_peer = self.nodes[0].add_p2p_connection(P2PInterface())
        second_peer = self.nodes[0].add_p2p_connection(P2PInterface())
        peer_1_info = self.nodes[0].getpeerinfo()[0]
        assert_equal(peer_1_info['permissions'], ['relay'])
        assert_equal(first_peer.relay, 1)
        peer_2_info = self.nodes[0].getpeerinfo()[1]
        assert_equal(peer_2_info['permissions'], ['relay'])
        assert_equal(self.nodes[0].testmempoolaccept([tx_hex])[0]['allowed'], True)

        self.log.info('Check that the tx from first_peer with relay-permission is relayed to others (ie.second_peer)')
        with self.nodes[0].assert_debug_log(["received getdata"]):
            # Note that normally, first_peer would never send us transactions since we're a blocksonly node.
            # By activating blocksonly, we explicitly tell our peers that they should not send us transactions,
            # and Bitcoin Core respects that choice and will not send transactions.
            # But if, for some reason, first_peer decides to relay transactions to us anyway, we should relay them to
            # second_peer since we gave relay permission to first_peer.
            # See https://github.com/bitcoin/bitcoin/issues/19943 for details.
            first_peer.send_without_ping(msg_tx(tx))
            self.log.info('Check that the peer with relay-permission is still connected after sending the transaction')
            assert_equal(first_peer.is_connected, True)
            second_peer.wait_for_tx(txid)
            assert_equal(self.nodes[0].getmempoolinfo()['size'], 1)
        self.log.info("Relay-permission peer's transaction is accepted and relayed")

        self.nodes[0].disconnect_p2ps()
        self.generate(self.nodes[0], 1)

    def blocks_relay_conn_tests(self):
        self.log.info('Tests with node in normal mode with block-relay-only connections')
        self.restart_node(0, ["-noblocksonly"])  # disables blocks only mode
        assert_equal(self.nodes[0].getnetworkinfo()['localrelay'], True)

        # Ensure we disconnect if a block-relay-only connection sends us a transaction
        self.nodes[0].add_outbound_p2p_connection(P2PInterface(), p2p_idx=0, connection_type="block-relay-only")
        assert_equal(self.nodes[0].getpeerinfo()[0]['relaytxes'], False)
        _, txid, _, tx_hex = self.check_p2p_tx_violation()

        self.log.info("Tests with node in normal mode with block-relay-only connection, sending an inv")
        conn = self.nodes[0].add_outbound_p2p_connection(P2PInterface(), p2p_idx=0, connection_type="block-relay-only")
        assert_equal(self.nodes[0].getpeerinfo()[0]['relaytxes'], False)
        self.check_p2p_inv_violation(conn)

        self.log.info("Check that txs from RPC are not sent to blockrelay connection")
        conn = self.nodes[0].add_outbound_p2p_connection(P2PTxInvStore(), p2p_idx=1, connection_type="block-relay-only")

        self.nodes[0].sendrawtransaction(tx_hex)

        # Bump time forward to ensure m_next_inv_send_time timer pops
        self.nodes[0].setmocktime(int(time.time()) + 60)

        conn.sync_with_ping()
        assert int(txid, 16) not in conn.get_invs()

    def check_p2p_inv_violation(self, peer):
        self.log.info("Check that tx-invs from P2P are rejected and result in disconnect")
        with self.nodes[0].assert_debug_log(["inv sent in violation of protocol, disconnecting peer"]):
            peer.send_without_ping(msg_inv([CInv(t=MSG_WTX, h=0x12345)]))
            peer.wait_for_disconnect()
        self.nodes[0].disconnect_p2ps()

    def check_p2p_tx_violation(self):
        self.log.info('Check that txs from P2P are rejected and result in disconnect')
        spendtx = self.miniwallet.create_self_transfer()

        with self.nodes[0].assert_debug_log(['transaction sent in violation of protocol, disconnecting peer=0']):
            self.nodes[0].p2ps[0].send_without_ping(msg_tx(spendtx['tx']))
            self.nodes[0].p2ps[0].wait_for_disconnect()
            assert_equal(self.nodes[0].getmempoolinfo()['size'], 0)
        self.nodes[0].disconnect_p2ps()

        return spendtx['tx'], spendtx['txid'], spendtx['wtxid'], spendtx['hex']


if __name__ == '__main__':
    P2PBlocksOnly(__file__).main()
