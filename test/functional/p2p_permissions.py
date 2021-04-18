#!/usr/bin/env python3
# Copyright (c) 2015-2020 The Widecoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test p2p permission message.

Test that permissions are correctly calculated and applied
"""

from test_framework.address import ADDRESS_BCRT1_P2WSH_OP_TRUE
from test_framework.messages import (
    CTransaction,
    CTxInWitness,
    FromHex,
)
from test_framework.p2p import P2PDataStore
from test_framework.script import (
    CScript,
    OP_TRUE,
)
from test_framework.test_node import ErrorMatch
from test_framework.test_framework import WidecoinTestFramework
from test_framework.util import (
    assert_equal,
    p2p_port,
)


class P2PPermissionsTests(WidecoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 2
        self.setup_clean_chain = True

    def run_test(self):
        self.check_tx_relay()

        self.checkpermission(
            # default permissions (no specific permissions)
            ["-whitelist=127.0.0.1"],
            # Make sure the default values in the command line documentation match the ones here
            ["relay", "noban", "mempool", "download"],
            True)

        self.checkpermission(
            # check without deprecatedrpc=whitelisted
            ["-whitelist=127.0.0.1"],
            # Make sure the default values in the command line documentation match the ones here
            ["relay", "noban", "mempool", "download"],
            None)

        self.checkpermission(
            # no permission (even with forcerelay)
            ["-whitelist=@127.0.0.1", "-whitelistforcerelay=1"],
            [],
            False)

        self.checkpermission(
            # relay permission removed (no specific permissions)
            ["-whitelist=127.0.0.1", "-whitelistrelay=0"],
            ["noban", "mempool", "download"],
            True)

        self.checkpermission(
            # forcerelay and relay permission added
            # Legacy parameter interaction which set whitelistrelay to true
            # if whitelistforcerelay is true
            ["-whitelist=127.0.0.1", "-whitelistforcerelay"],
            ["forcerelay", "relay", "noban", "mempool", "download"],
            True)

        # Let's make sure permissions are merged correctly
        # For this, we need to use whitebind instead of bind
        # by modifying the configuration file.
        ip_port = "127.0.0.1:{}".format(p2p_port(1))
        self.replaceinconfig(1, "bind=127.0.0.1", "whitebind=bloomfilter,forcerelay@" + ip_port)
        self.checkpermission(
            ["-whitelist=noban@127.0.0.1"],
            # Check parameter interaction forcerelay should activate relay
            ["noban", "bloomfilter", "forcerelay", "relay", "download"],
            False)
        self.replaceinconfig(1, "whitebind=bloomfilter,forcerelay@" + ip_port, "bind=127.0.0.1")

        self.checkpermission(
            # legacy whitelistrelay should be ignored
            ["-whitelist=noban,mempool@127.0.0.1", "-whitelistrelay"],
            ["noban", "mempool", "download"],
            False)

        self.checkpermission(
            # check without deprecatedrpc=whitelisted
            ["-whitelist=noban,mempool@127.0.0.1", "-whitelistrelay"],
            ["noban", "mempool", "download"],
            None)

        self.checkpermission(
            # legacy whitelistforcerelay should be ignored
            ["-whitelist=noban,mempool@127.0.0.1", "-whitelistforcerelay"],
            ["noban", "mempool", "download"],
            False)

        self.checkpermission(
            # missing mempool permission to be considered legacy whitelisted
            ["-whitelist=noban@127.0.0.1"],
            ["noban", "download"],
            False)

        self.checkpermission(
            # all permission added
            ["-whitelist=all@127.0.0.1"],
            ["forcerelay", "noban", "mempool", "bloomfilter", "relay", "download", "addr"],
            False)

        self.stop_node(1)
        self.nodes[1].assert_start_raises_init_error(["-whitelist=oopsie@127.0.0.1"], "Invalid P2P permission", match=ErrorMatch.PARTIAL_REGEX)
        self.nodes[1].assert_start_raises_init_error(["-whitelist=noban@127.0.0.1:230"], "Invalid netmask specified in", match=ErrorMatch.PARTIAL_REGEX)
        self.nodes[1].assert_start_raises_init_error(["-whitebind=noban@127.0.0.1/10"], "Cannot resolve -whitebind address", match=ErrorMatch.PARTIAL_REGEX)

    def check_tx_relay(self):
        block_op_true = self.nodes[0].getblock(self.nodes[0].generatetoaddress(100, ADDRESS_BCRT1_P2WSH_OP_TRUE)[0])
        self.sync_all()

        self.log.debug("Create a connection from a forcerelay peer that rebroadcasts raw txs")
        # A test framework p2p connection is needed to send the raw transaction directly. If a full node was used, it could only
        # rebroadcast via the inv-getdata mechanism. However, even for forcerelay connections, a full node would
        # currently not request a txid that is already in the mempool.
        self.restart_node(1, extra_args=["-whitelist=forcerelay@127.0.0.1"])
        p2p_rebroadcast_wallet = self.nodes[1].add_p2p_connection(P2PDataStore())

        self.log.debug("Send a tx from the wallet initially")
        tx = FromHex(
            CTransaction(),
            self.nodes[0].createrawtransaction(
                inputs=[{
                    'txid': block_op_true['tx'][0],
                    'vout': 0,
                }], outputs=[{
                    ADDRESS_BCRT1_P2WSH_OP_TRUE: 5,
                }]),
        )
        tx.wit.vtxinwit = [CTxInWitness()]
        tx.wit.vtxinwit[0].scriptWitness.stack = [CScript([OP_TRUE])]
        txid = tx.rehash()

        self.log.debug("Wait until tx is in node[1]'s mempool")
        p2p_rebroadcast_wallet.send_txs_and_test([tx], self.nodes[1])

        self.log.debug("Check that node[1] will send the tx to node[0] even though it is already in the mempool")
        self.connect_nodes(1, 0)
        with self.nodes[1].assert_debug_log(["Force relaying tx {} from peer=0".format(txid)]):
            p2p_rebroadcast_wallet.send_txs_and_test([tx], self.nodes[1])
            self.wait_until(lambda: txid in self.nodes[0].getrawmempool())

        self.log.debug("Check that node[1] will not send an invalid tx to node[0]")
        tx.vout[0].nValue += 1
        txid = tx.rehash()
        # Send the transaction twice. The first time, it'll be rejected by ATMP because it conflicts
        # with a mempool transaction. The second time, it'll be in the recentRejects filter.
        p2p_rebroadcast_wallet.send_txs_and_test(
            [tx],
            self.nodes[1],
            success=False,
            reject_reason='{} from peer=0 was not accepted: txn-mempool-conflict'.format(txid)
        )

        p2p_rebroadcast_wallet.send_txs_and_test(
            [tx],
            self.nodes[1],
            success=False,
            reject_reason='Not relaying non-mempool transaction {} from forcerelay peer=0'.format(txid)
        )

    def checkpermission(self, args, expectedPermissions, whitelisted):
        if whitelisted is not None:
            args = [*args, '-deprecatedrpc=whitelisted']
        self.restart_node(1, args)
        self.connect_nodes(0, 1)
        peerinfo = self.nodes[1].getpeerinfo()[0]
        if whitelisted is None:
            assert 'whitelisted' not in peerinfo
        else:
            assert_equal(peerinfo['whitelisted'], whitelisted)
        assert_equal(len(expectedPermissions), len(peerinfo['permissions']))
        for p in expectedPermissions:
            if not p in peerinfo['permissions']:
                raise AssertionError("Expected permissions %r is not granted." % p)

    def replaceinconfig(self, nodeid, old, new):
        with open(self.nodes[nodeid].widecoinconf, encoding="utf8") as f:
            newText = f.read().replace(old, new)
        with open(self.nodes[nodeid].widecoinconf, 'w', encoding="utf8") as f:
            f.write(newText)


if __name__ == '__main__':
    P2PPermissionsTests().main()
