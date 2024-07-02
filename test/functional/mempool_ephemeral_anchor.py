#!/usr/bin/env python3
# Copyright (c) 2024-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from decimal import Decimal

from test_framework.messages import (
    COIN,
    CTxOut,
)
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_raises_rpc_error,
)
from test_framework.wallet import (
    MiniWallet,
)

def cleanup(extra_args=None):
    def decorator(func):
        def wrapper(self):
            try:
                if extra_args is not None:
                    self.restart_node(0, extra_args=extra_args)
                func(self)
            finally:
                # Clear mempool again after test
                self.generate(self.nodes[0], 1)
                if extra_args is not None:
                    self.restart_node(0)
        return wrapper
    return decorator

class EphemeralAnchorTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1

    def assert_mempool_contents(self, expected=None):
        """Assert that all transactions in expected are in the mempool,
        and no additional ones exist.
        """
        if not expected:
            expected = []
        mempool = self.nodes[0].getrawmempool(verbose=False)
        assert_equal(len(mempool), len(expected))
        for tx in expected:
            assert tx.rehash() in mempool

    def add_output_to_create_multi_result(self, result, output):

        # Add output without changing absolute tx fee
        assert len(result["tx"].vout) > 0
        assert result["tx"].vout[0].nValue >= output.nValue
        result["tx"].vout.append(output)
        # Take value from first output
        result["tx"].vout[0].nValue -= output.nValue
        result["new_utxos"][0]["value"] = Decimal(result["tx"].vout[0].nValue) / COIN
        result["txid"]  = result["tx"].rehash()
        result["wtxid"] = result["tx"].getwtxid()
        result["hex"] = result["tx"].serialize().hex()
        for new_utxo in result["new_utxos"]:
            new_utxo["txid"] = result["tx"].rehash()
            new_utxo["wtxid"] = result["tx"].getwtxid()

        result["new_utxos"].append({"txid": result["tx"].rehash(), "vout": len(result["tx"].vout) - 1, "value": Decimal(output.nValue) / COIN, "height": 0, "coinbase": False, "confirmations": 0})

    def run_test(self):

        node = self.nodes[0]
        self.wallet = MiniWallet(node)

        self.test_normal_dust()
        self.test_sponsor_cycle()
        self.test_node_restart()
        self.test_fee_having_parent()
        self.test_multianchor()
        self.test_nonzero_anchor()
        self.test_non_v3()
        self.test_unspent_ephemeral()
        self.test_reorgs()
        self.test_acceptnonstd()

    def test_normal_dust(self):
        self.log.info("Create 0-value dusty output, show that it works inside v3 when spend in package")

        assert_equal(self.nodes[0].getrawmempool(), [])

        dusty_tx = self.wallet.create_self_transfer_multi(fee_per_output=0, version=3)
        self.add_output_to_create_multi_result(dusty_tx, CTxOut(0, dusty_tx["tx"].vout[0].scriptPubKey))

        sweep_tx = self.wallet.create_self_transfer_multi(utxos_to_spend=dusty_tx["new_utxos"], version=3)

        # Test doesn't work because lack of package feerates
        test_res = self.nodes[0].testmempoolaccept([dusty_tx["hex"], sweep_tx["hex"]])
        assert not test_res[0]["allowed"]
        assert_equal(test_res[0]["reject-reason"], "dust")

        # And doesn't work on its own
        assert_raises_rpc_error(-26, "dust", self.nodes[0].sendrawtransaction, dusty_tx["hex"])

        # But package evalution succeeds
        res = self.nodes[0].submitpackage([dusty_tx["hex"], sweep_tx["hex"]])
        assert_equal(res["package_msg"], "success")
        assert_equal(len(self.nodes[0].getrawmempool()), 2)
        self.assert_mempool_contents(expected=[dusty_tx["tx"], sweep_tx["tx"]])

        self.generate(self.nodes[0], 1)
        assert_equal(self.nodes[0].getrawmempool(), [])

    def test_node_restart(self):
        self.log.info("Test that an ephemeral package is rejected on restart due to individual evaluation")

        assert_equal(self.nodes[0].getrawmempool(), [])

        dusty_tx = self.wallet.create_self_transfer_multi(fee_per_output=0, version=3)
        self.add_output_to_create_multi_result(dusty_tx, CTxOut(0, dusty_tx["tx"].vout[0].scriptPubKey))

        sweep_tx = self.wallet.create_self_transfer_multi(utxos_to_spend=dusty_tx["new_utxos"], version=3)

        res = self.nodes[0].submitpackage([dusty_tx["hex"], sweep_tx["hex"]])
        assert_equal(res["package_msg"], "success")
        assert_equal(len(self.nodes[0].getrawmempool()), 2)
        self.assert_mempool_contents(expected=[dusty_tx["tx"], sweep_tx["tx"]])

        # Node restart; doesn't allow allow ephemeral transaction back in due to individual submission
        # resulting in 0-fee
        self.restart_node(0)
        assert_equal(self.nodes[0].getrawmempool(), [])

    def test_fee_having_parent(self):
        self.log.info("Test that a transaction with ephemeral anchor may not have base fee, and ignores modified")

        assert_equal(self.nodes[0].getrawmempool(), [])

        dusty_tx = self.wallet.create_self_transfer_multi(fee_per_output=1, version=3)
        self.add_output_to_create_multi_result(dusty_tx, CTxOut(0, dusty_tx["tx"].vout[0].scriptPubKey))
        assert_equal(dusty_tx["tx"].vout[0].nValue, 5000000000 - 1) # main output is not dust
        assert_equal(dusty_tx["tx"].vout[1].nValue, 0) # added one is

        sweep_tx = self.wallet.create_self_transfer_multi(utxos_to_spend=dusty_tx["new_utxos"], version=3)

        # When base fee is non-0, we report dust like usual
        res = self.nodes[0].submitpackage([dusty_tx["hex"], sweep_tx["hex"]])
        assert_equal(res["package_msg"], "transaction failed")
        assert_equal(res["tx-results"][dusty_tx["wtxid"]]["error"], "dust")

        # Priority is ignored: rejected even if modified fee is 0
        self.nodes[0].prioritisetransaction(txid=dusty_tx["txid"], dummy=0, fee_delta=-1)
        res = self.nodes[0].submitpackage([dusty_tx["hex"], sweep_tx["hex"]])
        assert_equal(res["package_msg"], "transaction failed")
        assert_equal(res["tx-results"][dusty_tx["wtxid"]]["error"], "dust")

        # Will be accepted if base fee is 0 even if modified fee is non-0
        dusty_tx = self.wallet.create_self_transfer_multi(fee_per_output=0, version=3)
        self.add_output_to_create_multi_result(dusty_tx, CTxOut(0, dusty_tx["tx"].vout[0].scriptPubKey))

        sweep_tx = self.wallet.create_self_transfer_multi(utxos_to_spend=dusty_tx["new_utxos"], version=3)

        self.nodes[0].prioritisetransaction(txid=dusty_tx["txid"], dummy=0, fee_delta=1)

        res = self.nodes[0].submitpackage([dusty_tx["hex"], sweep_tx["hex"]])
        assert_equal(res["package_msg"], "success")
        assert_equal(len(self.nodes[0].getrawmempool()), 2)
        self.assert_mempool_contents(expected=[dusty_tx["tx"], sweep_tx["tx"]])

        self.generate(self.nodes[0], 1)
        assert_equal(self.nodes[0].getrawmempool(), [])

    def test_multianchor(self):
        self.log.info("Test that a transaction with multiple ephemeral anchors is not allowed")

        assert_equal(self.nodes[0].getrawmempool(), [])

        dusty_tx = self.wallet.create_self_transfer_multi(fee_per_output=0, version=3)
        self.add_output_to_create_multi_result(dusty_tx, CTxOut(0, dusty_tx["tx"].vout[0].scriptPubKey))
        self.add_output_to_create_multi_result(dusty_tx, CTxOut(0, dusty_tx["tx"].vout[0].scriptPubKey))

        sweep_tx = self.wallet.create_self_transfer_multi(utxos_to_spend=dusty_tx["new_utxos"], version=3)

        res = self.nodes[0].submitpackage([dusty_tx["hex"], sweep_tx["hex"]])
        assert_equal(res["package_msg"], "transaction failed")
        assert_equal(res["tx-results"][dusty_tx["wtxid"]]["error"], "dust")
        assert_equal(self.nodes[0].getrawmempool(), [])

    def test_nonzero_anchor(self):
        self.log.info("Test that a transaction with any value is standard as long as it's spent")

        # 330 is dust threshold for taproot outputs
        for value in [1, 329, 330]:
            assert_equal(self.nodes[0].getrawmempool(), [])

            dusty_tx = self.wallet.create_self_transfer_multi(fee_per_output=0, version=3)
            self.add_output_to_create_multi_result(dusty_tx, CTxOut(value, dusty_tx["tx"].vout[0].scriptPubKey))

            sweep_tx = self.wallet.create_self_transfer_multi(utxos_to_spend=dusty_tx["new_utxos"], version=3)

            test_res = self.nodes[0].testmempoolaccept([dusty_tx["hex"], sweep_tx["hex"]])
            if value >= 330:
                assert_equal(test_res[0]["reject-reason"], "min relay fee not met")
            else:
                assert_equal(test_res[0]["reject-reason"], "dust")

            res = self.nodes[0].submitpackage([dusty_tx["hex"], sweep_tx["hex"]])
            assert_equal(res["package_msg"], "success")
            assert_equal(len(self.nodes[0].getrawmempool()), 2)
            self.assert_mempool_contents(expected=[dusty_tx["tx"], sweep_tx["tx"]])

            self.generate(self.nodes[0], 1)
            assert_equal(self.nodes[0].getrawmempool(), [])


    def test_non_v3(self):
        self.log.info("Test that v2 dust-having transaction is rejected even if spent")

        assert_equal(self.nodes[0].getrawmempool(), [])

        dusty_tx = self.wallet.create_self_transfer_multi(fee_per_output=0, version=2)
        self.add_output_to_create_multi_result(dusty_tx, CTxOut(0, dusty_tx["tx"].vout[0].scriptPubKey))

        sweep_tx = self.wallet.create_self_transfer_multi(utxos_to_spend=dusty_tx["new_utxos"], version=2)

        res = self.nodes[0].submitpackage([dusty_tx["hex"], sweep_tx["hex"]])
        assert_equal(res["package_msg"], "transaction failed")
        assert_equal(res["tx-results"][dusty_tx["wtxid"]]["error"], "dust")

        assert_equal(self.nodes[0].getrawmempool(), [])

    def test_unspent_ephemeral(self):
        self.log.info("Test that ephemeral outputs of any value are disallowed if not spent in a package")

        assert_equal(self.nodes[0].getrawmempool(), [])

        dusty_tx = self.wallet.create_self_transfer_multi(fee_per_output=0, version=3)
        self.add_output_to_create_multi_result(dusty_tx, CTxOut(329, dusty_tx["tx"].vout[0].scriptPubKey))

        # We are sweeping dust at 0-fee to create dust in child, which should be rejected still
        sweep_tx = self.wallet.create_self_transfer_multi(fee_per_output=0, utxos_to_spend=[dusty_tx["new_utxos"][1]], version=3)

        # Prioritise sweep to keep calculations simple
        self.nodes[0].prioritisetransaction(sweep_tx["txid"], 0, 1000)

        res = self.nodes[0].submitpackage([dusty_tx["hex"], sweep_tx["hex"]])
        assert_equal(res["package_msg"], "unspent-dust")
        assert_equal(self.nodes[0].getrawmempool(), [])

        # Set fee to 0 for simplicity, but prioritise a bit to have package be acceptable
        sweep_tx = self.wallet.create_self_transfer_multi(fee_per_output=0, utxos_to_spend=dusty_tx["new_utxos"], version=3)

        self.nodes[0].prioritisetransaction(sweep_tx["txid"], 0, 1000)

        # Spend works
        res = self.nodes[0].submitpackage([dusty_tx["hex"], sweep_tx["hex"]])
        assert_equal(res["package_msg"], "success")

        # Re-set and spend the non-anchor instead (1st output)
        self.generate(self.nodes[0], 1)
        assert_equal(self.nodes[0].getrawmempool(), [])

        dusty_tx = self.wallet.create_self_transfer_multi(fee_per_output=0, version=3)
        self.add_output_to_create_multi_result(dusty_tx, CTxOut(329, dusty_tx["tx"].vout[0].scriptPubKey))

        # Set fee to 0 for simplicity, but prioritise it to have package be acceptable
        sweep_tx = self.wallet.create_self_transfer_multi(fee_per_output=0, utxos_to_spend=[dusty_tx["new_utxos"][0]], version=3)
        self.nodes[0].prioritisetransaction(sweep_tx["txid"], 0, 1000)

        res = self.nodes[0].submitpackage([dusty_tx["hex"], sweep_tx["hex"]])
        assert_equal(res["package_msg"], "unspent-dust")

        assert_equal(self.nodes[0].getrawmempool(), [])

    def test_sponsor_cycle(self):
        self.log.info("Test that dust txn is not evicted when it becomes childless, but won't be mined")

        assert_equal(self.nodes[0].getrawmempool(), [])

        dusty_tx = self.wallet.create_self_transfer_multi(
            fee_per_output=0,
            version=3
        )

        self.add_output_to_create_multi_result(dusty_tx, CTxOut(0, dusty_tx["tx"].vout[0].scriptPubKey))

        coins = self.wallet.get_utxos(mark_as_spent=False)
        sponsor_coin = coins[0]
        del coins[0]

        # Bring "fee" input that can be double-spend seperately
        sweep_tx = self.wallet.create_self_transfer_multi(utxos_to_spend=dusty_tx["new_utxos"] + [sponsor_coin], version=3)

        res = self.nodes[0].submitpackage([dusty_tx["hex"], sweep_tx["hex"]])
        assert_equal(res["package_msg"], "success")
        assert_equal(len(self.nodes[0].getrawmempool()), 2)
        self.assert_mempool_contents(expected=[dusty_tx["tx"], sweep_tx["tx"]])

        # Now we RBF away the child using the sponsor input only
        unsponsor_tx = self.wallet.create_self_transfer_multi(
            utxos_to_spend=[sponsor_coin],
            num_outputs=1,
            fee_per_output=2000,
            version=3
        )
        self.nodes[0].sendrawtransaction(unsponsor_tx["hex"])

        # Parent is now childless and fee-free, so will not be mined
        entry_info = self.nodes[0].getmempoolentry(dusty_tx["txid"])
        assert_equal(entry_info["descendantcount"], 1)
        assert_equal(entry_info["fees"]["descendant"], Decimal(0))

        self.assert_mempool_contents(expected=[dusty_tx["tx"], unsponsor_tx["tx"]])

        self.generate(self.nodes[0], 1)
        self.assert_mempool_contents(expected=[dusty_tx["tx"]])

        # Create sweep that doesn't spend the dust with parent still in mempool
        sweep_tx = self.wallet.create_self_transfer_multi(utxos_to_spend=[dusty_tx["new_utxos"][0]], version=3)
        assert_raises_rpc_error(-26, "ephemeral-anchor-unspent, tx does not spend all parent ephemeral anchors", self.nodes[0].sendrawtransaction, sweep_tx["hex"])

        # Create sweep that doesn't spend conflicting sponsor
        sweep_tx = self.wallet.create_self_transfer_multi(utxos_to_spend=dusty_tx["new_utxos"], version=3)

        # Can rebroadcast the sweep again and get it mined
        self.nodes[0].sendrawtransaction(sweep_tx["hex"])

        self.generate(self.nodes[0], 1)
        assert_equal(self.nodes[0].getrawmempool(), [])

    def test_reorgs(self):
        self.log.info("Test that reorgs breaking the v3 topology doesn't cause issues")

        assert_equal(self.nodes[0].getrawmempool(), [])

        # Get dusty tx mined, then check that it makes it back into mempool on reorg
        # due to bypass_limits
        dusty_tx = self.wallet.create_self_transfer_multi(fee_per_output=0, version=3)
        self.add_output_to_create_multi_result(dusty_tx, CTxOut(0, dusty_tx["tx"].vout[0].scriptPubKey))
        assert_raises_rpc_error(-26, "dust", self.nodes[0].sendrawtransaction, dusty_tx["hex"])

        block_res = self.nodes[0].rpc.generateblock(self.wallet.get_address(), [dusty_tx["hex"]])
        self.nodes[0].invalidateblock(block_res["hash"])
        self.assert_mempool_contents(expected=[dusty_tx["tx"]])

        # Create a sweep that has dust of its own and leaves dusty_tx's dust unspent
        sweep_tx = self.wallet.create_self_transfer_multi(fee_per_output=0, utxos_to_spend=[dusty_tx["new_utxos"][0]], version=3)
        self.add_output_to_create_multi_result(sweep_tx, CTxOut(0, sweep_tx["tx"].vout[0].scriptPubKey))
        assert_raises_rpc_error(-26, "dust", self.nodes[0].sendrawtransaction, sweep_tx["hex"])

        # Mine the sweep then re-org, the sweep will make it even with unspent dusty_tx dust
        block_res = self.nodes[0].rpc.generateblock(self.wallet.get_address(), [dusty_tx["hex"], sweep_tx["hex"]])
        self.nodes[0].invalidateblock(block_res["hash"])
        self.assert_mempool_contents(expected=[dusty_tx["tx"], sweep_tx["tx"]])

        # Also should happen if dust is swept
        sweep_tx_2 = self.wallet.create_self_transfer_multi(fee_per_output=0, utxos_to_spend=dusty_tx["new_utxos"], version=3)
        self.add_output_to_create_multi_result(sweep_tx, CTxOut(0, sweep_tx["tx"].vout[0].scriptPubKey))
        assert_raises_rpc_error(-26, "dust", self.nodes[0].sendrawtransaction, sweep_tx["hex"])

        block_res = self.nodes[0].rpc.generateblock(self.wallet.get_address(), [dusty_tx["hex"], sweep_tx_2["hex"]])
        self.nodes[0].invalidateblock(block_res["hash"])
        self.assert_mempool_contents(expected=[dusty_tx["tx"], sweep_tx_2["tx"]])

        # TRUC transactions restriction for ephemeral dust disallows further spends of ancestor chains
        child_tx = self.wallet.create_self_transfer_multi(utxos_to_spend=sweep_tx_2["new_utxos"], version=3)
        assert_raises_rpc_error(-26, "v3-rule-violation", self.nodes[0].sendrawtransaction, child_tx["hex"])

        self.nodes[0].reconsiderblock(block_res["hash"])
        assert_equal(self.nodes[0].getrawmempool(), [])

    @cleanup(extra_args=["-acceptnonstdtxn=1"])
    def test_acceptnonstd(self):
        self.log.info("Test that dust is allowed when acceptnonstd=1")

        assert_equal(self.nodes[0].getrawmempool(), [])

        dusty_tx = self.wallet.create_self_transfer_multi(fee_per_output=0, version=3)
        self.add_output_to_create_multi_result(dusty_tx, CTxOut(0, dusty_tx["tx"].vout[0].scriptPubKey))
        # Make it above minfee
        self.nodes[0].prioritisetransaction(txid=dusty_tx["txid"], dummy=0, fee_delta=1000)
        self.nodes[0].rpc.sendrawtransaction(dusty_tx["hex"])

        self.assert_mempool_contents(expected=[dusty_tx["tx"]])

if __name__ == "__main__":
    EphemeralAnchorTest().main()
