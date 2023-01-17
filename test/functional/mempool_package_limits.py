#!/usr/bin/env python3
# Copyright (c) 2021-2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test logic for limiting mempool and package ancestors/descendants."""

from decimal import Decimal

from test_framework.blocktools import COINBASE_MATURITY
from test_framework.test_framework import SyscoinTestFramework
from test_framework.messages import (
    COIN,
    WITNESS_SCALE_FACTOR,
)
from test_framework.util import (
    assert_equal,
)
from test_framework.wallet import MiniWallet

class MempoolPackageLimitsTest(SyscoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True

    def run_test(self):
        self.wallet = MiniWallet(self.nodes[0])
        # Add enough mature utxos to the wallet so that all txs spend confirmed coins.
        self.generate(self.wallet, 35)
        self.generate(self.nodes[0], COINBASE_MATURITY)

        self.test_chain_limits()
        self.test_desc_count_limits()
        self.test_desc_count_limits_2()
        self.test_anc_count_limits()
        self.test_anc_count_limits_2()
        self.test_anc_count_limits_bushy()

        # The node will accept (nonstandard) extra large OP_RETURN outputs
        self.restart_node(0, extra_args=["-datacarriersize=100000"])
        self.test_anc_size_limits()
        self.test_desc_size_limits()

    def test_chain_limits_helper(self, mempool_count, package_count):
        node = self.nodes[0]
        assert_equal(0, node.getmempoolinfo()["size"])
        chain_hex = []

        chaintip_utxo = self.wallet.send_self_transfer_chain(from_node=node, chain_length=mempool_count)[-1]["new_utxo"]
        # in-package transactions
        for _ in range(package_count):
            tx = self.wallet.create_self_transfer(utxo_to_spend=chaintip_utxo)
            chaintip_utxo = tx["new_utxo"]
            chain_hex.append(tx["hex"])
        testres_too_long = node.testmempoolaccept(rawtxs=chain_hex)
        for txres in testres_too_long:
            assert_equal(txres["package-error"], "package-mempool-limits")

        # Clear mempool and check that the package passes now
        self.generate(node, 1)
        assert all([res["allowed"] for res in node.testmempoolaccept(rawtxs=chain_hex)])

    def test_chain_limits(self):
        """Create chains from mempool and package transactions that are longer than 25,
        but only if both in-mempool and in-package transactions are considered together.
        This checks that both mempool and in-package transactions are taken into account when
        calculating ancestors/descendant limits.
        """
        self.log.info("Check that in-package ancestors count for mempool ancestor limits")

        # 24 transactions in the mempool and 2 in the package. The parent in the package has
        # 24 in-mempool ancestors and 1 in-package descendant. The child has 0 direct parents
        # in the mempool, but 25 in-mempool and in-package ancestors in total.
        self.test_chain_limits_helper(24, 2)
        # 2 transactions in the mempool and 24 in the package.
        self.test_chain_limits_helper(2, 24)
        # 13 transactions in the mempool and 13 in the package.
        self.test_chain_limits_helper(13, 13)

    def test_desc_count_limits(self):
        """Create an 'A' shaped package with 24 transactions in the mempool and 2 in the package:
                    M1
                   ^  ^
                 M2a  M2b
                .       .
               .         .
              .           .
             M12a          ^
            ^              M13b
           ^                 ^
          Pa                  Pb
        The top ancestor in the package exceeds descendant limits but only if the in-mempool and in-package
        descendants are all considered together (24 including in-mempool descendants and 26 including both
        package transactions).
        """
        node = self.nodes[0]
        assert_equal(0, node.getmempoolinfo()["size"])
        self.log.info("Check that in-mempool and in-package descendants are calculated properly in packages")
        # Top parent in mempool, M1
        m1_utxos = self.wallet.send_self_transfer_multi(from_node=node, num_outputs=2)['new_utxos']

        package_hex = []
        # Chain A (M2a... M12a)
        chain_a_tip_utxo = self.wallet.send_self_transfer_chain(from_node=node, chain_length=11, utxo_to_spend=m1_utxos[0])[-1]["new_utxo"]
        # Pa
        pa_hex = self.wallet.create_self_transfer(utxo_to_spend=chain_a_tip_utxo)["hex"]
        package_hex.append(pa_hex)

        # Chain B (M2b... M13b)
        chain_b_tip_utxo = self.wallet.send_self_transfer_chain(from_node=node, chain_length=12, utxo_to_spend=m1_utxos[1])[-1]["new_utxo"]
        # Pb
        pb_hex = self.wallet.create_self_transfer(utxo_to_spend=chain_b_tip_utxo)["hex"]
        package_hex.append(pb_hex)

        assert_equal(24, node.getmempoolinfo()["size"])
        assert_equal(2, len(package_hex))
        testres_too_long = node.testmempoolaccept(rawtxs=package_hex)
        for txres in testres_too_long:
            assert_equal(txres["package-error"], "package-mempool-limits")

        # Clear mempool and check that the package passes now
        self.generate(node, 1)
        assert all([res["allowed"] for res in node.testmempoolaccept(rawtxs=package_hex)])

    def test_desc_count_limits_2(self):
        """Create a Package with 24 transaction in mempool and 2 transaction in package:
                      M1
                     ^  ^
                   M2    ^
                   .      ^
                  .        ^
                 .          ^
                M24          ^
                              ^
                              P1
                              ^
                              P2
        P1 has M1 as a mempool ancestor, P2 has no in-mempool ancestors, but when
        combined P2 has M1 as an ancestor and M1 exceeds descendant_limits(23 in-mempool
        descendants + 2 in-package descendants, a total of 26 including itself).
        """

        node = self.nodes[0]
        package_hex = []
        # M1
        m1_utxos = self.wallet.send_self_transfer_multi(from_node=node, num_outputs=2)['new_utxos']

        # Chain M2...M24
        self.wallet.send_self_transfer_chain(from_node=node, chain_length=23, utxo_to_spend=m1_utxos[0])[-1]["new_utxo"]

        # P1
        p1_tx = self.wallet.create_self_transfer(utxo_to_spend=m1_utxos[1])
        package_hex.append(p1_tx["hex"])

        # P2
        p2_tx = self.wallet.create_self_transfer(utxo_to_spend=p1_tx["new_utxo"])
        package_hex.append(p2_tx["hex"])

        assert_equal(24, node.getmempoolinfo()["size"])
        assert_equal(2, len(package_hex))
        testres = node.testmempoolaccept(rawtxs=package_hex)
        assert_equal(len(testres), len(package_hex))
        for txres in testres:
            assert_equal(txres["package-error"], "package-mempool-limits")

        # Clear mempool and check that the package passes now
        self.generate(node, 1)
        assert all([res["allowed"] for res in node.testmempoolaccept(rawtxs=package_hex)])

    def test_anc_count_limits(self):
        """Create a 'V' shaped chain with 24 transactions in the mempool and 3 in the package:
        M1a                    M1b
         ^                     ^
          M2a                M2b
           .                 .
            .               .
             .             .
             M12a        M12b
               ^         ^
                Pa     Pb
                 ^    ^
                   Pc
        The lowest descendant, Pc, exceeds ancestor limits, but only if the in-mempool
        and in-package ancestors are all considered together.
        """
        node = self.nodes[0]
        assert_equal(0, node.getmempoolinfo()["size"])
        package_hex = []
        pc_parent_utxos = []

        self.log.info("Check that in-mempool and in-package ancestors are calculated properly in packages")

        # Two chains of 13 transactions each
        for _ in range(2):
            chain_tip_utxo = self.wallet.send_self_transfer_chain(from_node=node, chain_length=12)[-1]["new_utxo"]
            # Save the 13th transaction for the package
            tx = self.wallet.create_self_transfer(utxo_to_spend=chain_tip_utxo)
            package_hex.append(tx["hex"])
            pc_parent_utxos.append(tx["new_utxo"])

        # Child Pc
        pc_hex = self.wallet.create_self_transfer_multi(utxos_to_spend=pc_parent_utxos)["hex"]
        package_hex.append(pc_hex)

        assert_equal(24, node.getmempoolinfo()["size"])
        assert_equal(3, len(package_hex))
        testres_too_long = node.testmempoolaccept(rawtxs=package_hex)
        for txres in testres_too_long:
            assert_equal(txres["package-error"], "package-mempool-limits")

        # Clear mempool and check that the package passes now
        self.generate(node, 1)
        assert all([res["allowed"] for res in node.testmempoolaccept(rawtxs=package_hex)])

    def test_anc_count_limits_2(self):
        """Create a 'Y' shaped chain with 24 transactions in the mempool and 2 in the package:
        M1a                M1b
         ^                ^
          M2a            M2b
           .            .
            .          .
             .        .
            M12a    M12b
               ^    ^
                 Pc
                 ^
                 Pd
        The lowest descendant, Pd, exceeds ancestor limits, but only if the in-mempool
        and in-package ancestors are all considered together.
        """
        node = self.nodes[0]
        assert_equal(0, node.getmempoolinfo()["size"])
        pc_parent_utxos = []

        self.log.info("Check that in-mempool and in-package ancestors are calculated properly in packages")
        # Two chains of 12 transactions each
        for _ in range(2):
            chaintip_utxo = self.wallet.send_self_transfer_chain(from_node=node, chain_length=12)[-1]["new_utxo"]
            # last 2 transactions will be the parents of Pc
            pc_parent_utxos.append(chaintip_utxo)

        # Child Pc
        pc_tx = self.wallet.create_self_transfer_multi(utxos_to_spend=pc_parent_utxos)

        # Child Pd
        pd_tx = self.wallet.create_self_transfer(utxo_to_spend=pc_tx["new_utxos"][0])

        assert_equal(24, node.getmempoolinfo()["size"])
        testres_too_long = node.testmempoolaccept(rawtxs=[pc_tx["hex"], pd_tx["hex"]])
        for txres in testres_too_long:
            assert_equal(txres["package-error"], "package-mempool-limits")

        # Clear mempool and check that the package passes now
        self.generate(node, 1)
        assert all([res["allowed"] for res in node.testmempoolaccept(rawtxs=[pc_tx["hex"], pd_tx["hex"]])])

    def test_anc_count_limits_bushy(self):
        """Create a tree with 20 transactions in the mempool and 6 in the package:
        M1...M4 M5...M8 M9...M12 M13...M16 M17...M20
            ^      ^       ^        ^         ^             (each with 4 parents)
            P0     P1      P2      P3        P4
             ^     ^       ^       ^         ^              (5 parents)
                           PC
        Where M(4i+1)...M+(4i+4) are the parents of Pi and P0, P1, P2, P3, and P4 are the parents of PC.
        P0... P4 individually only have 4 parents each, and PC has no in-mempool parents. But
        combined, PC has 25 in-mempool and in-package parents.
        """
        node = self.nodes[0]
        assert_equal(0, node.getmempoolinfo()["size"])
        package_hex = []
        pc_parent_utxos = []
        for _ in range(5): # Make package transactions P0 ... P4
            pc_grandparent_utxos = []
            for _ in range(4): # Make mempool transactions M(4i+1)...M(4i+4)
                pc_grandparent_utxos.append(self.wallet.send_self_transfer(from_node=node)["new_utxo"])
            # Package transaction Pi
            pi_tx = self.wallet.create_self_transfer_multi(utxos_to_spend=pc_grandparent_utxos)
            package_hex.append(pi_tx["hex"])
            pc_parent_utxos.append(pi_tx["new_utxos"][0])
        # Package transaction PC
        pc_hex = self.wallet.create_self_transfer_multi(utxos_to_spend=pc_parent_utxos)["hex"]
        package_hex.append(pc_hex)

        assert_equal(20, node.getmempoolinfo()["size"])
        assert_equal(6, len(package_hex))
        testres = node.testmempoolaccept(rawtxs=package_hex)
        for txres in testres:
            assert_equal(txres["package-error"], "package-mempool-limits")

        # Clear mempool and check that the package passes now
        self.generate(node, 1)
        assert all([res["allowed"] for res in node.testmempoolaccept(rawtxs=package_hex)])

    def test_anc_size_limits(self):
        """Test Case with 2 independent transactions in the mempool and a parent + child in the
        package, where the package parent is the child of both mempool transactions (30KvB each):
              A     B
               ^   ^
                 C
                 ^
                 D
        The lowest descendant, D, exceeds ancestor size limits, but only if the in-mempool
        and in-package ancestors are all considered together.
        """
        node = self.nodes[0]
        assert_equal(0, node.getmempoolinfo()["size"])
        parent_utxos = []
        target_weight = WITNESS_SCALE_FACTOR * 1000 * 30 # 30KvB
        high_fee = Decimal("0.003") # 10 sats/vB
        self.log.info("Check that in-mempool and in-package ancestor size limits are calculated properly in packages")
        # Mempool transactions A and B
        for _ in range(2):
            bulked_tx = self.wallet.create_self_transfer(target_weight=target_weight)
            self.wallet.sendrawtransaction(from_node=node, tx_hex=bulked_tx["hex"])
            parent_utxos.append(bulked_tx["new_utxo"])

        # Package transaction C
        pc_tx = self.wallet.create_self_transfer_multi(utxos_to_spend=parent_utxos, fee_per_output=int(high_fee * COIN), target_weight=target_weight)

        # Package transaction D
        pd_tx = self.wallet.create_self_transfer(utxo_to_spend=pc_tx["new_utxos"][0], target_weight=target_weight)

        assert_equal(2, node.getmempoolinfo()["size"])
        testres_too_heavy = node.testmempoolaccept(rawtxs=[pc_tx["hex"], pd_tx["hex"]])
        for txres in testres_too_heavy:
            assert_equal(txres["package-error"], "package-mempool-limits")

        # Clear mempool and check that the package passes now
        self.generate(node, 1)
        assert all([res["allowed"] for res in node.testmempoolaccept(rawtxs=[pc_tx["hex"], pd_tx["hex"]])])

    def test_desc_size_limits(self):
        """Create 3 mempool transactions and 2 package transactions (25KvB each):
              Ma
             ^ ^
            Mb  Mc
           ^     ^
          Pd      Pe
        The top ancestor in the package exceeds descendant size limits but only if the in-mempool
        and in-package descendants are all considered together.
        """
        node = self.nodes[0]
        assert_equal(0, node.getmempoolinfo()["size"])
        target_weight = 21 * 1000 * WITNESS_SCALE_FACTOR
        high_fee = Decimal("0.0021") # 10 sats/vB
        self.log.info("Check that in-mempool and in-package descendant sizes are calculated properly in packages")
        # Top parent in mempool, Ma
        ma_tx = self.wallet.create_self_transfer_multi(num_outputs=2, fee_per_output=int(high_fee / 2 * COIN), target_weight=target_weight)
        self.wallet.sendrawtransaction(from_node=node, tx_hex=ma_tx["hex"])

        package_hex = []
        for j in range(2): # Two legs (left and right)
            # Mempool transaction (Mb and Mc)
            mempool_tx = self.wallet.create_self_transfer(utxo_to_spend=ma_tx["new_utxos"][j], target_weight=target_weight)
            self.wallet.sendrawtransaction(from_node=node, tx_hex=mempool_tx["hex"])

            # Package transaction (Pd and Pe)
            package_tx = self.wallet.create_self_transfer(utxo_to_spend=mempool_tx["new_utxo"], target_weight=target_weight)
            package_hex.append(package_tx["hex"])

        assert_equal(3, node.getmempoolinfo()["size"])
        assert_equal(2, len(package_hex))
        testres_too_heavy = node.testmempoolaccept(rawtxs=package_hex)
        for txres in testres_too_heavy:
            assert_equal(txres["package-error"], "package-mempool-limits")

        # Clear mempool and check that the package passes now
        self.generate(node, 1)
        assert all([res["allowed"] for res in node.testmempoolaccept(rawtxs=package_hex)])

if __name__ == "__main__":
    MempoolPackageLimitsTest().main()
