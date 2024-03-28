#!/usr/bin/env python3
# Copyright (c) 2024-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""
Test that 1p1c package submission allows a 1p1c package to propagate in a "network" of nodes. Send
various packages from different nodes on a network in which some nodes have already received some of
the transactions (and submitted them to mempool, kept them as orphans or rejected them as
too-low-feerate transactions). The packages should be received and accepted by all nodes.
"""

from decimal import Decimal
from test_framework.messages import (
    CInv,
    MSG_WTX,
    msg_inv,
    msg_tx,
)
from test_framework.p2p import (
    P2PInterface,
)
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_greater_than,
    fill_mempool,
)
from test_framework.wallet import (
    MiniWallet,
    MiniWalletMode,
)

# 1sat/vB feerate denominated in BTC/KvB
FEERATE_1SAT_VB = Decimal("0.00001000")

class PackageRelayTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 4
        # hugely speeds up the test, as it involves multiple hops of tx relay.
        self.noban_tx_relay = True
        self.extra_args = [[
            "-datacarriersize=100000",
            "-maxmempool=5",
        ]] * self.num_nodes
        self.supports_cli = False

    def raise_network_minfee(self):
        filler_wallet = MiniWallet(self.nodes[0])
        fill_mempool(self, self.nodes[0], filler_wallet)

        self.log.debug("Wait for the network to sync mempools")
        self.sync_mempools()

        self.log.debug("Check that all nodes' mempool minimum feerates are above min relay feerate")
        for node in self.nodes:
            assert_equal(node.getmempoolinfo()['minrelaytxfee'], FEERATE_1SAT_VB)
            assert_greater_than(node.getmempoolinfo()['mempoolminfee'], FEERATE_1SAT_VB)

    def create_packages(self):
        # Basic 1-parent-1-child package, parent 1sat/vB, child 999sat/vB
        low_fee_parent = self.wallet.create_self_transfer(fee_rate=FEERATE_1SAT_VB, confirmed_only=True)
        high_fee_child = self.wallet.create_self_transfer(utxo_to_spend=low_fee_parent["new_utxo"], fee_rate=999*FEERATE_1SAT_VB)
        package_hex_basic = [low_fee_parent["hex"], high_fee_child["hex"]]

        # Basic 1-parent-1-child package, parent 1sat/vB, child 999sat/vB
        # Parent's txid is the same as its wtxid.
        low_fee_parent_nonsegwit = self.wallet_nonsegwit.create_self_transfer(fee_rate=FEERATE_1SAT_VB, confirmed_only=True)
        assert_equal(low_fee_parent_nonsegwit["txid"], low_fee_parent_nonsegwit["wtxid"])
        high_fee_child_nonsegwit = self.wallet_nonsegwit.create_self_transfer(utxo_to_spend=low_fee_parent_nonsegwit["new_utxo"], fee_rate=999*FEERATE_1SAT_VB)
        assert_equal(high_fee_child_nonsegwit["txid"], high_fee_child_nonsegwit["wtxid"])
        package_hex_basic_nonsegwit = [low_fee_parent_nonsegwit["hex"], high_fee_child_nonsegwit["hex"]]

        packages_to_submit = []
        transactions_to_presend = [[]] * self.num_nodes

        # node0: sender
        # node1: pre-received the child (orphan)
        # node2: pre-received nothing
        # node3: pre-received the parent (too low fee)
        packages_to_submit.append(package_hex_basic)
        packages_to_submit.append(package_hex_basic_nonsegwit)
        transactions_to_presend[1] = [high_fee_child["tx"], high_fee_child_nonsegwit["tx"]]
        transactions_to_presend[3] = [low_fee_parent["tx"], low_fee_parent_nonsegwit["tx"]]

        return packages_to_submit, transactions_to_presend

    def run_test(self):
        self.wallet = MiniWallet(self.nodes[1])
        self.wallet_nonsegwit = MiniWallet(self.nodes[2], mode=MiniWalletMode.RAW_P2PK)
        self.generate(self.wallet_nonsegwit, 10)
        self.generate(self.wallet, 120)

        self.log.info("Fill mempools with large transactions to raise mempool minimum feerates")
        self.raise_network_minfee()

        # Create the transactions.
        self.wallet.rescan_utxos(include_mempool=True)
        packages_to_submit, transactions_to_presend = self.create_packages()

        self.peers = []
        for i in range(self.num_nodes):
            self.peers.append(self.nodes[i].add_outbound_p2p_connection(P2PInterface(), p2p_idx=i, connection_type="outbound-full-relay"))

        self.log.info("Pre-send some transactions to nodes")
        for i in range(self.num_nodes):
            peer = self.peers[i]
            for tx in transactions_to_presend[i]:
                inv = CInv(t=MSG_WTX, h=int(tx.getwtxid(), 16))
                peer.send_and_ping(msg_inv([inv]))
                peer.wait_for_getdata([int(tx.getwtxid(), 16)])
                peer.send_and_ping(msg_tx(tx))
            # This disconnect removes any sent orphans from the orphanage (EraseForPeer) and times
            # out the in-flight requests.  It is currently required for the test to pass right now,
            # because the node will not reconsider an orphan tx and will not (re)try requesting
            # orphan parents from multiple peers if the first one didn't respond.
            # TODO: remove this in the future if the node tries orphan resolution with multiple peers.
            peer.peer_disconnect()

        self.log.info("Submit full packages to node0")
        for package_hex in packages_to_submit:
            self.nodes[0].submitpackage(package_hex)

        self.log.info("Wait for mempools to sync")
        self.sync_mempools(timeout=20)


if __name__ == '__main__':
    PackageRelayTest().main()
