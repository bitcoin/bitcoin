#!/usr/bin/env python3
# Copyright (c) 2015-2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test p2p permission message.

Test that permissions are correctly calculated and applied
"""

from test_framework.messages import (
    SEQUENCE_FINAL,
)
from test_framework.p2p import P2PDataStore
from test_framework.test_node import ErrorMatch
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    p2p_port,
)
from test_framework.wallet import MiniWallet


class P2PPermissionsTests(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 2

    def run_test(self):
        self.wallet = MiniWallet(self.nodes[0])

        self.check_tx_relay()

        self.checkpermission(
            # default permissions (no specific permissions)
            ["-whitelist=127.0.0.1"],
            # Make sure the default values in the command line documentation match the ones here
            ["relay", "noban", "mempool", "download"])

        self.checkpermission(
            # no permission (even with forcerelay)
            ["-whitelist=@127.0.0.1", "-whitelistforcerelay=1"],
            [])

        self.checkpermission(
            # relay permission removed (no specific permissions)
            ["-whitelist=127.0.0.1", "-whitelistrelay=0"],
            ["noban", "mempool", "download"])

        self.checkpermission(
            # forcerelay and relay permission added
            # Legacy parameter interaction which set whitelistrelay to true
            # if whitelistforcerelay is true
            ["-whitelist=127.0.0.1", "-whitelistforcerelay"],
            ["forcerelay", "relay", "noban", "mempool", "download"])

        # Let's make sure permissions are merged correctly
        # For this, we need to use whitebind instead of bind
        # by modifying the configuration file.
        ip_port = "127.0.0.1:{}".format(p2p_port(1))
        self.nodes[1].replace_in_config([("bind=127.0.0.1", "whitebind=bloomfilter,forcerelay@" + ip_port)])
        self.checkpermission(
            ["-whitelist=noban@127.0.0.1"],
            # Check parameter interaction forcerelay should activate relay
            ["noban", "bloomfilter", "forcerelay", "relay", "download"])
        self.nodes[1].replace_in_config([("whitebind=bloomfilter,forcerelay@" + ip_port, "bind=127.0.0.1")])

        self.checkpermission(
            # legacy whitelistrelay should be ignored
            ["-whitelist=noban,mempool@127.0.0.1", "-whitelistrelay"],
            ["noban", "mempool", "download"])

        self.checkpermission(
            # legacy whitelistforcerelay should be ignored
            ["-whitelist=noban,mempool@127.0.0.1", "-whitelistforcerelay"],
            ["noban", "mempool", "download"])

        self.checkpermission(
            # missing mempool permission to be considered legacy whitelisted
            ["-whitelist=noban@127.0.0.1"],
            ["noban", "download"])

        self.checkpermission(
            # all permission added
            ["-whitelist=all@127.0.0.1"],
            ["forcerelay", "noban", "mempool", "bloomfilter", "relay", "download", "addr"])

        for flag, permissions in [(["-whitelist=noban,out@127.0.0.1"], ["noban", "download"]), (["-whitelist=noban@127.0.0.1"], [])]:
            self.restart_node(0, flag)
            self.connect_nodes(0, 1)
            peerinfo = self.nodes[0].getpeerinfo()[0]
            assert_equal(peerinfo['permissions'], permissions)

        self.stop_node(1)
        self.nodes[1].assert_start_raises_init_error(["-whitelist=in,out@127.0.0.1"], "Only direction was set, no permissions", match=ErrorMatch.PARTIAL_REGEX)
        self.nodes[1].assert_start_raises_init_error(["-whitelist=oopsie@127.0.0.1"], "Invalid P2P permission", match=ErrorMatch.PARTIAL_REGEX)
        self.nodes[1].assert_start_raises_init_error(["-whitelist=noban@127.0.0.1:230"], "Invalid netmask specified in", match=ErrorMatch.PARTIAL_REGEX)
        self.nodes[1].assert_start_raises_init_error(["-whitebind=noban@127.0.0.1/10"], "Cannot resolve -whitebind address", match=ErrorMatch.PARTIAL_REGEX)
        self.nodes[1].assert_start_raises_init_error(["-whitebind=noban@127.0.0.1", "-bind=127.0.0.1", "-listen=0"], "Cannot set -bind or -whitebind together with -listen=0", match=ErrorMatch.PARTIAL_REGEX)

    def check_tx_relay(self):
        self.log.debug("Create a connection from a forcerelay peer that rebroadcasts raw txs")
        # A test framework p2p connection is needed to send the raw transaction directly. If a full node was used, it could only
        # rebroadcast via the inv-getdata mechanism. However, even for forcerelay connections, a full node would
        # currently not request a txid that is already in the mempool.
        self.restart_node(1, extra_args=["-whitelist=forcerelay@127.0.0.1"])
        p2p_rebroadcast_wallet = self.nodes[1].add_p2p_connection(P2PDataStore())

        self.log.debug("Send a tx from the wallet initially")
        tx = self.wallet.create_self_transfer(sequence=SEQUENCE_FINAL)['tx']
        txid = tx.rehash()

        self.log.debug("Wait until tx is in node[1]'s mempool")
        p2p_rebroadcast_wallet.send_txs_and_test([tx], self.nodes[1])

        self.log.debug("Check that node[1] will send the tx to node[0] even though it is already in the mempool")
        self.connect_nodes(1, 0)
        with self.nodes[1].assert_debug_log(["Force relaying tx {} (wtxid={}) from peer=0".format(txid, tx.getwtxid())]):
            p2p_rebroadcast_wallet.send_txs_and_test([tx], self.nodes[1])
            self.wait_until(lambda: txid in self.nodes[0].getrawmempool())

        self.log.debug("Check that node[1] will not send an invalid tx to node[0]")
        tx.vout[0].nValue += 1
        # add dust to cause policy rejection but no disconnection
        tx.vout.append(tx.vout[0])
        tx.vout[-1].nValue = 0
        txid = tx.rehash()
        # Send the transaction twice. The first time, it'll be rejected by ATMP because it conflicts
        # with a mempool transaction. The second time, it'll be in the m_lazy_recent_rejects filter.
        p2p_rebroadcast_wallet.send_txs_and_test(
            [tx],
            self.nodes[1],
            success=False,
            reject_reason='{} (wtxid={}) from peer=0 was not accepted: dust'.format(txid, tx.getwtxid())
        )

        p2p_rebroadcast_wallet.send_txs_and_test(
            [tx],
            self.nodes[1],
            success=False,
            reject_reason='Not relaying non-mempool transaction {} (wtxid={}) from forcerelay peer=0'.format(txid, tx.getwtxid())
        )

    def checkpermission(self, args, expectedPermissions):
        self.restart_node(1, args)
        self.connect_nodes(0, 1)
        peerinfo = self.nodes[1].getpeerinfo()[0]
        assert_equal(len(expectedPermissions), len(peerinfo['permissions']))
        for p in expectedPermissions:
            if p not in peerinfo['permissions']:
                raise AssertionError("Expected permissions %r is not granted." % p)


if __name__ == '__main__':
    P2PPermissionsTests(__file__).main()
