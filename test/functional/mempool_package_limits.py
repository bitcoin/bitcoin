#!/usr/bin/env python3
# Copyright (c) 2021-2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test logic for limiting mempool and package ancestors/descendants."""
from test_framework.blocktools import COINBASE_MATURITY
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
)
from test_framework.wallet import MiniWallet

# Decorator to
# 1) check that mempool is empty at the start of a subtest
# 2) run the subtest, which may submit some transaction(s) to the mempool and
#    create a list of hex transactions
# 3) testmempoolaccept the package hex and check that it fails with the error
#    "too-large-cluster" for each tx
# 4) after mining a block, clearing the pre-submitted transactions from mempool,
#    check that submitting the created package succeeds
def check_package_limits(func):
    def func_wrapper(self, *args, **kwargs):
        node = self.nodes[0]
        assert_equal(0, node.getmempoolinfo()["size"])
        package_hex = func(self, *args, **kwargs)
        testres_error_expected = node.testmempoolaccept(rawtxs=package_hex)
        assert_equal(len(testres_error_expected), len(package_hex))
        for txres in testres_error_expected:
            assert "too-large-cluster" in txres["package-error"]

        # Clear mempool and check that the package passes now
        self.generate(node, 1)
        assert all([res["allowed"] for res in node.testmempoolaccept(rawtxs=package_hex)])

    return func_wrapper


class MempoolPackageLimitsTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True
        self.extra_args = [["-limitclustercount=25"]]

    def run_test(self):
        self.wallet = MiniWallet(self.nodes[0])
        # Add enough mature utxos to the wallet so that all txs spend confirmed coins.
        self.generate(self.wallet, COINBASE_MATURITY + 35)

        self.test_chain_limits()
        self.test_desc_count_limits()
        self.test_desc_count_limits_2()
        self.test_desc_size_limits()

    @check_package_limits
    def test_chain_limits_helper(self, mempool_count, package_count):
        node = self.nodes[0]
        chain_hex = []

        chaintip_utxo = self.wallet.send_self_transfer_chain(from_node=node, chain_length=mempool_count)[-1]["new_utxo"]
        # in-package transactions
        for _ in range(package_count):
            tx = self.wallet.create_self_transfer(utxo_to_spend=chaintip_utxo)
            chaintip_utxo = tx["new_utxo"]
            chain_hex.append(tx["hex"])
        return chain_hex

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

    @check_package_limits
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
        return package_hex

    @check_package_limits
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
        return package_hex

    @check_package_limits
    def test_desc_size_limits(self):
        """Create 3 mempool transactions and 2 package transactions (21KvB each):
              Ma
             ^ ^
            Mb  Mc
           ^     ^
          Pd      Pe
        The top ancestor in the package exceeds descendant size limits but only if the in-mempool
        and in-package descendants are all considered together.
        """
        node = self.nodes[0]
        target_vsize = 21_000
        high_fee = 10 * target_vsize  # 10 sats/vB
        self.log.info("Check that in-mempool and in-package descendant sizes are calculated properly in packages")
        # Top parent in mempool, Ma
        ma_tx = self.wallet.create_self_transfer_multi(num_outputs=2, fee_per_output=high_fee // 2, target_vsize=target_vsize)
        self.wallet.sendrawtransaction(from_node=node, tx_hex=ma_tx["hex"])

        package_hex = []
        for j in range(2): # Two legs (left and right)
            # Mempool transaction (Mb and Mc)
            mempool_tx = self.wallet.create_self_transfer(utxo_to_spend=ma_tx["new_utxos"][j], target_vsize=target_vsize)
            self.wallet.sendrawtransaction(from_node=node, tx_hex=mempool_tx["hex"])

            # Package transaction (Pd and Pe)
            package_tx = self.wallet.create_self_transfer(utxo_to_spend=mempool_tx["new_utxo"], target_vsize=target_vsize)
            package_hex.append(package_tx["hex"])

        assert_equal(3, node.getmempoolinfo()["size"])
        assert_equal(2, len(package_hex))
        return package_hex


if __name__ == "__main__":
    MempoolPackageLimitsTest(__file__).main()
