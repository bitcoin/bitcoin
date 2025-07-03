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
from math import ceil

from test_framework.mempool_util import (
    fill_mempool,
)
from test_framework.messages import (
    msg_tx,
)
from test_framework.p2p import (
    P2PInterface,
)
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_greater_than,
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
            "-maxmempool=5",
        ]] * self.num_nodes

    def raise_network_minfee(self):
        fill_mempool(self, self.nodes[0])

        self.log.debug("Check that all nodes' mempool minimum feerates are above min relay feerate")
        for node in self.nodes:
            assert_equal(node.getmempoolinfo()['minrelaytxfee'], FEERATE_1SAT_VB)
            assert_greater_than(node.getmempoolinfo()['mempoolminfee'], FEERATE_1SAT_VB)

    def create_basic_1p1c(self, wallet):
        low_fee_parent = wallet.create_self_transfer(fee_rate=FEERATE_1SAT_VB, confirmed_only=True)
        high_fee_child = wallet.create_self_transfer(utxo_to_spend=low_fee_parent["new_utxo"], fee_rate=999*FEERATE_1SAT_VB)
        package_hex_basic = [low_fee_parent["hex"], high_fee_child["hex"]]
        return package_hex_basic, low_fee_parent["tx"], high_fee_child["tx"]

    def create_package_2outs(self, wallet):
        # First create a tester tx to see the vsize, and then adjust the fees
        utxo_for_2outs = wallet.get_utxo(confirmed_only=True)

        low_fee_parent_2outs_tester = wallet.create_self_transfer_multi(
            utxos_to_spend=[utxo_for_2outs],
            num_outputs=2,
        )

        # Target 1sat/vB so the number of satoshis is equal to the vsize.
        # Round up. The goal is to be between min relay feerate and mempool min feerate.
        fee_2outs = ceil(low_fee_parent_2outs_tester["tx"].get_vsize() / 2)

        low_fee_parent_2outs = wallet.create_self_transfer_multi(
            utxos_to_spend=[utxo_for_2outs],
            num_outputs=2,
            fee_per_output=fee_2outs,
        )

        # Now create the child
        high_fee_child_2outs = wallet.create_self_transfer_multi(
            utxos_to_spend=low_fee_parent_2outs["new_utxos"][::-1],
            fee_per_output=fee_2outs*100,
        )
        return [low_fee_parent_2outs["hex"], high_fee_child_2outs["hex"]], low_fee_parent_2outs["tx"], high_fee_child_2outs["tx"]

    def create_package_2p1c(self, wallet):
        parent1 = wallet.create_self_transfer(fee_rate=FEERATE_1SAT_VB*10, confirmed_only=True)
        parent2 = wallet.create_self_transfer(fee_rate=FEERATE_1SAT_VB*20, confirmed_only=True)
        child = wallet.create_self_transfer_multi(
            utxos_to_spend=[parent1["new_utxo"], parent2["new_utxo"]],
            fee_per_output=999*parent1["tx"].get_vsize(),
        )
        return [parent1["hex"], parent2["hex"], child["hex"]], parent1["tx"], parent2["tx"], child["tx"]

    def create_packages(self):
        # 1: Basic 1-parent-1-child package, parent 1sat/vB, child 999sat/vB
        package_hex_1, parent_1, child_1 = self.create_basic_1p1c(self.wallet)

        # 2: same as 1, parent's txid is the same as its wtxid.
        package_hex_2, parent_2, child_2 = self.create_basic_1p1c(self.wallet_nonsegwit)

        # 3: 2-parent-1-child package. Both parents are above mempool min feerate. No package submission happens.
        # We require packages to be child-with-unconfirmed-parents and only allow 1-parent-1-child packages.
        package_hex_3, parent_31, _parent_32, child_3 = self.create_package_2p1c(self.wallet)

        # 4: parent + child package where the child spends 2 different outputs from the parent.
        package_hex_4, parent_4, child_4 = self.create_package_2outs(self.wallet)

        # Assemble return results
        packages_to_submit = [package_hex_1, package_hex_2, package_hex_3, package_hex_4]
        # node0: sender
        # node1: pre-received the children (orphan)
        # node3: pre-received the parents (too low fee)
        # All nodes receive parent_31 ahead of time.
        txns_to_send = [
            [],
            [child_1, child_2, parent_31, child_3, child_4],
            [parent_31],
            [parent_1, parent_2, parent_31, parent_4]
        ]

        return packages_to_submit, txns_to_send

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

        self.peers = [self.nodes[i].add_p2p_connection(P2PInterface()) for i in range(self.num_nodes)]

        self.log.info("Pre-send some transactions to nodes")
        for (i, peer) in enumerate(self.peers):
            for tx in transactions_to_presend[i]:
                peer.send_and_ping(msg_tx(tx))

        # Disconnect python peers to clear outstanding orphan requests with them, avoiding timeouts.
        # We are only interested in the syncing behavior between real nodes.
        for i in range(self.num_nodes):
            self.nodes[i].disconnect_p2ps()

        self.log.info("Submit full packages to node0")
        for package_hex in packages_to_submit:
            submitpackage_result = self.nodes[0].submitpackage(package_hex)
            assert_equal(submitpackage_result["package_msg"], "success")

        self.log.info("Wait for mempools to sync")
        self.sync_mempools()


if __name__ == '__main__':
    PackageRelayTest(__file__).main()
