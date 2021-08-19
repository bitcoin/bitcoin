#!/usr/bin/env python3
# Copyright (c) 2021 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test logic for limiting mempool and package ancestors/descendants."""

from decimal import Decimal

from test_framework.address import ADDRESS_BCRT1_P2WSH_OP_TRUE
from test_framework.test_framework import BitcoinTestFramework
from test_framework.messages import (
    COIN,
    CTransaction,
    CTxInWitness,
    tx_from_hex,
    WITNESS_SCALE_FACTOR,
)
from test_framework.script import (
    CScript,
    OP_TRUE,
)
from test_framework.util import (
    assert_equal,
)
from test_framework.wallet import (
    bulk_transaction,
    create_child_with_parents,
    make_chain,
)

class MempoolPackageLimitsTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True

    def run_test(self):
        self.log.info("Generate blocks to create UTXOs")
        node = self.nodes[0]
        self.privkeys = [node.get_deterministic_priv_key().key]
        self.address = node.get_deterministic_priv_key().address
        self.coins = []
        # The last 100 coinbase transactions are premature
        for b in self.generatetoaddress(node, 200, self.address)[:100]:
            coinbase = node.getblock(blockhash=b, verbosity=2)["tx"][0]
            self.coins.append({
                "txid": coinbase["txid"],
                "amount": coinbase["vout"][0]["value"],
                "scriptPubKey": coinbase["vout"][0]["scriptPubKey"],
            })

        self.test_chain_limits()
        self.test_desc_count_limits()
        self.test_anc_count_limits()
        self.test_anc_count_limits_2()
        self.test_anc_count_limits_bushy()

        # The node will accept our (nonstandard) extra large OP_RETURN outputs
        self.restart_node(0, extra_args=["-acceptnonstdtxn=1"])
        self.test_anc_size_limits()
        self.test_desc_size_limits()

    def test_chain_limits_helper(self, mempool_count, package_count):
        node = self.nodes[0]
        assert_equal(0, node.getmempoolinfo()["size"])
        first_coin = self.coins.pop()
        spk = None
        txid = first_coin["txid"]
        chain_hex = []
        chain_txns = []
        value = first_coin["amount"]

        for i in range(mempool_count + package_count):
            (tx, txhex, value, spk) = make_chain(node, self.address, self.privkeys, txid, value, 0, spk)
            txid = tx.rehash()
            if i < mempool_count:
                node.sendrawtransaction(txhex)
                assert_equal(node.getmempoolentry(txid)["ancestorcount"], i + 1)
            else:
                chain_hex.append(txhex)
                chain_txns.append(tx)
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
        first_coin = self.coins.pop()
        parent_value = (first_coin["amount"] - Decimal("0.0002")) / 2 # Deduct reasonable fee and make 2 outputs
        inputs = [{"txid": first_coin["txid"], "vout": 0}]
        outputs = [{self.address : parent_value}, {ADDRESS_BCRT1_P2WSH_OP_TRUE : parent_value}]
        rawtx = node.createrawtransaction(inputs, outputs)

        parent_signed = node.signrawtransactionwithkey(hexstring=rawtx, privkeys=self.privkeys)
        assert parent_signed["complete"]
        parent_tx = tx_from_hex(parent_signed["hex"])
        parent_txid = parent_tx.rehash()
        node.sendrawtransaction(parent_signed["hex"])

        package_hex = []

        # Chain A
        spk = parent_tx.vout[0].scriptPubKey.hex()
        value = parent_value
        txid = parent_txid
        for i in range(12):
            (tx, txhex, value, spk) = make_chain(node, self.address, self.privkeys, txid, value, 0, spk)
            txid = tx.rehash()
            if i < 11: # M2a... M12a
                node.sendrawtransaction(txhex)
            else: # Pa
                package_hex.append(txhex)

        # Chain B
        value = parent_value - Decimal("0.0001")
        rawtx_b = node.createrawtransaction([{"txid": parent_txid, "vout": 1}], {self.address : value})
        tx_child_b = tx_from_hex(rawtx_b) # M2b
        tx_child_b.wit.vtxinwit = [CTxInWitness()]
        tx_child_b.wit.vtxinwit[0].scriptWitness.stack = [CScript([OP_TRUE])]
        tx_child_b_hex = tx_child_b.serialize().hex()
        node.sendrawtransaction(tx_child_b_hex)
        spk = tx_child_b.vout[0].scriptPubKey.hex()
        txid = tx_child_b.rehash()
        for i in range(12):
            (tx, txhex, value, spk) = make_chain(node, self.address, self.privkeys, txid, value, 0, spk)
            txid = tx.rehash()
            if i < 11: # M3b... M13b
                node.sendrawtransaction(txhex)
            else: # Pb
                package_hex.append(txhex)

        assert_equal(24, node.getmempoolinfo()["size"])
        assert_equal(2, len(package_hex))
        testres_too_long = node.testmempoolaccept(rawtxs=package_hex)
        for txres in testres_too_long:
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
        parents_tx = []
        values = []
        scripts = []

        self.log.info("Check that in-mempool and in-package ancestors are calculated properly in packages")

        # Two chains of 13 transactions each
        for _ in range(2):
            spk = None
            top_coin = self.coins.pop()
            txid = top_coin["txid"]
            value = top_coin["amount"]
            for i in range(13):
                (tx, txhex, value, spk) = make_chain(node, self.address, self.privkeys, txid, value, 0, spk)
                txid = tx.rehash()
                if i < 12:
                    node.sendrawtransaction(txhex)
                else: # Save the 13th transaction for the package
                    package_hex.append(txhex)
                    parents_tx.append(tx)
                    scripts.append(spk)
                    values.append(value)

        # Child Pc
        child_hex = create_child_with_parents(node, self.address, self.privkeys, parents_tx, values, scripts)
        package_hex.append(child_hex)

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
        parents_tx = []
        values = []
        scripts = []

        self.log.info("Check that in-mempool and in-package ancestors are calculated properly in packages")
        # Two chains of 12 transactions each
        for _ in range(2):
            spk = None
            top_coin = self.coins.pop()
            txid = top_coin["txid"]
            value = top_coin["amount"]
            for i in range(12):
                (tx, txhex, value, spk) = make_chain(node, self.address, self.privkeys, txid, value, 0, spk)
                txid = tx.rehash()
                value -= Decimal("0.0001")
                node.sendrawtransaction(txhex)
                if i == 11:
                    # last 2 transactions will be the parents of Pc
                    parents_tx.append(tx)
                    values.append(value)
                    scripts.append(spk)

        # Child Pc
        pc_hex = create_child_with_parents(node, self.address, self.privkeys, parents_tx, values, scripts)
        pc_tx = tx_from_hex(pc_hex)
        pc_value = sum(values) - Decimal("0.0002")
        pc_spk = pc_tx.vout[0].scriptPubKey.hex()

        # Child Pd
        (_, pd_hex, _, _) = make_chain(node, self.address, self.privkeys, pc_tx.rehash(), pc_value, 0, pc_spk)

        assert_equal(24, node.getmempoolinfo()["size"])
        testres_too_long = node.testmempoolaccept(rawtxs=[pc_hex, pd_hex])
        for txres in testres_too_long:
            assert_equal(txres["package-error"], "package-mempool-limits")

        # Clear mempool and check that the package passes now
        self.generate(node, 1)
        assert all([res["allowed"] for res in node.testmempoolaccept(rawtxs=[pc_hex, pd_hex])])

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
        parent_txns = []
        parent_values = []
        scripts = []
        for _ in range(5): # Make package transactions P0 ... P4
            gp_tx = []
            gp_values = []
            gp_scripts = []
            for _ in range(4): # Make mempool transactions M(4i+1)...M(4i+4)
                parent_coin = self.coins.pop()
                value = parent_coin["amount"]
                txid = parent_coin["txid"]
                (tx, txhex, value, spk) = make_chain(node, self.address, self.privkeys, txid, value)
                gp_tx.append(tx)
                gp_values.append(value)
                gp_scripts.append(spk)
                node.sendrawtransaction(txhex)
            # Package transaction Pi
            pi_hex = create_child_with_parents(node, self.address, self.privkeys, gp_tx, gp_values, gp_scripts)
            package_hex.append(pi_hex)
            pi_tx = tx_from_hex(pi_hex)
            parent_txns.append(pi_tx)
            parent_values.append(Decimal(pi_tx.vout[0].nValue) / COIN)
            scripts.append(pi_tx.vout[0].scriptPubKey.hex())
        # Package transaction PC
        package_hex.append(create_child_with_parents(node, self.address, self.privkeys, parent_txns, parent_values, scripts))

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
        parents_tx = []
        values = []
        scripts = []
        target_weight = WITNESS_SCALE_FACTOR * 1000 * 30 # 30KvB
        high_fee = Decimal("0.003") # 10 sats/vB
        self.log.info("Check that in-mempool and in-package ancestor size limits are calculated properly in packages")
        # Mempool transactions A and B
        for _ in range(2):
            spk = None
            top_coin = self.coins.pop()
            txid = top_coin["txid"]
            value = top_coin["amount"]
            (tx, _, _, _) = make_chain(node, self.address, self.privkeys, txid, value, 0, spk, high_fee)
            bulked_tx = bulk_transaction(tx, node, target_weight, self.privkeys)
            node.sendrawtransaction(bulked_tx.serialize().hex())
            parents_tx.append(bulked_tx)
            values.append(Decimal(bulked_tx.vout[0].nValue) / COIN)
            scripts.append(bulked_tx.vout[0].scriptPubKey.hex())

        # Package transaction C
        small_pc_hex = create_child_with_parents(node, self.address, self.privkeys, parents_tx, values, scripts, high_fee)
        pc_tx = bulk_transaction(tx_from_hex(small_pc_hex), node, target_weight, self.privkeys)
        pc_value = Decimal(pc_tx.vout[0].nValue) / COIN
        pc_spk = pc_tx.vout[0].scriptPubKey.hex()
        pc_hex = pc_tx.serialize().hex()

        # Package transaction D
        (small_pd, _, val, spk) = make_chain(node, self.address, self.privkeys, pc_tx.rehash(), pc_value, 0, pc_spk, high_fee)
        prevtxs = [{
            "txid": pc_tx.rehash(),
            "vout": 0,
            "scriptPubKey": spk,
            "amount": val,
        }]
        pd_tx = bulk_transaction(small_pd, node, target_weight, self.privkeys, prevtxs)
        pd_hex = pd_tx.serialize().hex()

        assert_equal(2, node.getmempoolinfo()["size"])
        testres_too_heavy = node.testmempoolaccept(rawtxs=[pc_hex, pd_hex])
        for txres in testres_too_heavy:
            assert_equal(txres["package-error"], "package-mempool-limits")

        # Clear mempool and check that the package passes now
        self.generate(node, 1)
        assert all([res["allowed"] for res in node.testmempoolaccept(rawtxs=[pc_hex, pd_hex])])

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
        first_coin = self.coins.pop()
        parent_value = (first_coin["amount"] - high_fee) / 2 # Deduct fee and make 2 outputs
        inputs = [{"txid": first_coin["txid"], "vout": 0}]
        outputs = [{self.address : parent_value}, {ADDRESS_BCRT1_P2WSH_OP_TRUE:  parent_value}]
        rawtx = node.createrawtransaction(inputs, outputs)
        parent_tx = bulk_transaction(tx_from_hex(rawtx), node, target_weight, self.privkeys)
        node.sendrawtransaction(parent_tx.serialize().hex())

        package_hex = []
        for j in range(2): # Two legs (left and right)
            # Mempool transaction (Mb and Mc)
            mempool_tx = CTransaction()
            spk = parent_tx.vout[j].scriptPubKey.hex()
            value = Decimal(parent_tx.vout[j].nValue) / COIN
            txid = parent_tx.rehash()
            prevtxs = [{
                "txid": txid,
                "vout": j,
                "scriptPubKey": spk,
                "amount": value,
            }]
            if j == 0: # normal key
                (tx_small, _, _, _) = make_chain(node, self.address, self.privkeys, txid, value, j, spk, high_fee)
                mempool_tx = bulk_transaction(tx_small, node, target_weight, self.privkeys, prevtxs)
            else: # OP_TRUE
                inputs = [{"txid": txid, "vout": 1}]
                outputs = {self.address: value - high_fee}
                small_tx = tx_from_hex(node.createrawtransaction(inputs, outputs))
                mempool_tx = bulk_transaction(small_tx, node, target_weight, None, prevtxs)
            node.sendrawtransaction(mempool_tx.serialize().hex())

            # Package transaction (Pd and Pe)
            spk = mempool_tx.vout[0].scriptPubKey.hex()
            value = Decimal(mempool_tx.vout[0].nValue) / COIN
            txid = mempool_tx.rehash()
            (tx_small, _, _, _) = make_chain(node, self.address, self.privkeys, txid, value, 0, spk, high_fee)
            prevtxs = [{
                "txid": txid,
                "vout": 0,
                "scriptPubKey": spk,
                "amount": value,
            }]
            package_tx = bulk_transaction(tx_small, node, target_weight, self.privkeys, prevtxs)
            package_hex.append(package_tx.serialize().hex())

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
