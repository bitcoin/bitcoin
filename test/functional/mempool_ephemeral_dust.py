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

class EphemeralDustTest(BitcoinTestFramework):
    def set_test_params(self):
        # Mempools should match via 1P1C p2p relay
        self.num_nodes = 2

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

    def add_output_to_create_multi_result(self, result, output_value=0):
        """ Add output without changing absolute tx fee
        """
        assert len(result["tx"].vout) > 0
        assert result["tx"].vout[0].nValue >= output_value
        result["tx"].vout.append(CTxOut(output_value, result["tx"].vout[0].scriptPubKey))
        # Take value from first output
        result["tx"].vout[0].nValue -= output_value
        result["new_utxos"][0]["value"] = Decimal(result["tx"].vout[0].nValue) / COIN
        new_txid = result["tx"].rehash()
        result["txid"]  = new_txid
        result["wtxid"] = result["tx"].getwtxid()
        result["hex"] = result["tx"].serialize().hex()
        for new_utxo in result["new_utxos"]:
            new_utxo["txid"] = new_txid
            new_utxo["wtxid"] = result["tx"].getwtxid()

        result["new_utxos"].append({"txid": new_txid, "vout": len(result["tx"].vout) - 1, "value": Decimal(output_value) / COIN, "height": 0, "coinbase": False, "confirmations": 0})

    def run_test(self):

        node = self.nodes[0]
        self.wallet = MiniWallet(node)

        self.test_normal_dust()
        self.test_sponsor_cycle()
        self.test_node_restart()
        self.test_fee_having_parent()
        self.test_multidust()
        self.test_nonzero_dust()
        self.test_non_truc()
        self.test_unspent_ephemeral()
        self.test_reorgs()
        self.test_acceptnonstd()
        self.test_free_relay()

    def test_normal_dust(self):
        self.log.info("Create 0-value dusty output, show that it works inside truc when spend in package")

        assert_equal(self.nodes[0].getrawmempool(), [])

        dusty_tx = self.wallet.create_self_transfer_multi(fee_per_output=0, version=3)
        self.add_output_to_create_multi_result(dusty_tx)

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
        self.add_output_to_create_multi_result(dusty_tx)

        sweep_tx = self.wallet.create_self_transfer_multi(utxos_to_spend=dusty_tx["new_utxos"], version=3)

        res = self.nodes[0].submitpackage([dusty_tx["hex"], sweep_tx["hex"]])
        assert_equal(res["package_msg"], "success")
        assert_equal(len(self.nodes[0].getrawmempool()), 2)
        self.assert_mempool_contents(expected=[dusty_tx["tx"], sweep_tx["tx"]])

        # Node restart; doesn't allow allow ephemeral transaction back in due to individual submission
        # resulting in 0-fee
        self.restart_node(0)
        self.connect_nodes(0, 1)
        assert_equal(self.nodes[0].getrawmempool(), [])

    def test_fee_having_parent(self):
        self.log.info("Test that a transaction with ephemeral dust may not have base fee, and ignores modified")

        assert_equal(self.nodes[0].getrawmempool(), [])

        dusty_tx = self.wallet.create_self_transfer_multi(fee_per_output=1, version=3)
        self.add_output_to_create_multi_result(dusty_tx)
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
        self.add_output_to_create_multi_result(dusty_tx)

        sweep_tx = self.wallet.create_self_transfer_multi(utxos_to_spend=dusty_tx["new_utxos"], version=3)

        self.nodes[0].prioritisetransaction(txid=dusty_tx["txid"], dummy=0, fee_delta=1)

        res = self.nodes[0].submitpackage([dusty_tx["hex"], sweep_tx["hex"]])
        assert_equal(res["package_msg"], "success")
        assert_equal(len(self.nodes[0].getrawmempool()), 2)
        self.assert_mempool_contents(expected=[dusty_tx["tx"], sweep_tx["tx"]])

        self.generate(self.nodes[0], 1)
        assert_equal(self.nodes[0].getrawmempool(), [])

    def test_multidust(self):
        self.log.info("Test that a transaction with multiple ephemeral dusts is not allowed")

        assert_equal(self.nodes[0].getrawmempool(), [])

        dusty_tx = self.wallet.create_self_transfer_multi(fee_per_output=0, version=3)
        self.add_output_to_create_multi_result(dusty_tx)
        self.add_output_to_create_multi_result(dusty_tx)

        sweep_tx = self.wallet.create_self_transfer_multi(utxos_to_spend=dusty_tx["new_utxos"], version=3)

        res = self.nodes[0].submitpackage([dusty_tx["hex"], sweep_tx["hex"]])
        assert_equal(res["package_msg"], "transaction failed")
        assert_equal(res["tx-results"][dusty_tx["wtxid"]]["error"], "dust")
        assert_equal(self.nodes[0].getrawmempool(), [])

    def test_nonzero_dust(self):
        self.log.info("Test that a transaction with any value is standard as long as it's spent")

        # 330 is dust threshold for taproot outputs
        for value in [1, 329, 330]:
            assert_equal(self.nodes[0].getrawmempool(), [])

            dusty_tx = self.wallet.create_self_transfer_multi(fee_per_output=0, version=3)
            self.add_output_to_create_multi_result(dusty_tx, value)

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

    def test_non_truc(self):
        self.log.info("Test that v2 dust-having transaction is rejected even if spent, because of min relay requirement")

        assert_equal(self.nodes[0].getrawmempool(), [])

        dusty_tx = self.wallet.create_self_transfer_multi(fee_per_output=0, version=2)
        self.add_output_to_create_multi_result(dusty_tx)

        sweep_tx = self.wallet.create_self_transfer_multi(utxos_to_spend=dusty_tx["new_utxos"], version=2)

        res = self.nodes[0].submitpackage([dusty_tx["hex"], sweep_tx["hex"]])
        assert_equal(res["package_msg"], "transaction failed")
        assert_equal(res["tx-results"][dusty_tx["wtxid"]]["error"], "min relay fee not met, 0 < 147")

        assert_equal(self.nodes[0].getrawmempool(), [])

    def test_unspent_ephemeral(self):
        self.log.info("Test that ephemeral outputs of any value are disallowed if not spent in a package")

        assert_equal(self.nodes[0].getrawmempool(), [])

        dusty_tx = self.wallet.create_self_transfer_multi(fee_per_output=0, version=3)
        self.add_output_to_create_multi_result(dusty_tx, 329)

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

        # Re-set and spend the non-dust instead (1st output)
        self.generate(self.nodes[0], 1)
        assert_equal(self.nodes[0].getrawmempool(), [])

        dusty_tx = self.wallet.create_self_transfer_multi(fee_per_output=0, version=3)
        self.add_output_to_create_multi_result(dusty_tx, 329)

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

        self.add_output_to_create_multi_result(dusty_tx)

        coins = self.wallet.get_utxos(mark_as_spent=False)
        sponsor_coin = coins[0]
        del coins[0]

        # Bring "fee" input that can be double-spend seperately
        sweep_tx = self.wallet.create_self_transfer_multi(utxos_to_spend=dusty_tx["new_utxos"] + [sponsor_coin], version=3)

        res = self.nodes[0].submitpackage([dusty_tx["hex"], sweep_tx["hex"]])
        assert_equal(res["package_msg"], "success")
        assert_equal(len(self.nodes[0].getrawmempool()), 2)
        self.assert_mempool_contents(expected=[dusty_tx["tx"], sweep_tx["tx"]])
        # sync to make sure unsponsor_tx hits second node's mempool after initial package
        self.sync_all()

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
        assert_raises_rpc_error(-26, "ephemeral-dust-unspent, tx does not spend parent ephemeral dust", self.nodes[0].sendrawtransaction, sweep_tx["hex"])

        # Create sweep that doesn't spend conflicting sponsor
        sweep_tx = self.wallet.create_self_transfer_multi(utxos_to_spend=dusty_tx["new_utxos"], version=3)

        # Can rebroadcast the sweep again and get it mined
        self.nodes[0].sendrawtransaction(sweep_tx["hex"])

        self.generate(self.nodes[0], 1)
        assert_equal(self.nodes[0].getrawmempool(), [])

    def test_reorgs(self):
        self.log.info("Test that reorgs breaking the truc topology doesn't cause issues")

        assert_equal(self.nodes[0].getrawmempool(), [])

        # Many shallow re-orgs confuse block gossiping making test less reliable otherwise
        self.disconnect_nodes(0, 1)

        # Get dusty tx mined, then check that it makes it back into mempool on reorg
        # due to bypass_limits
        dusty_tx = self.wallet.create_self_transfer_multi(fee_per_output=0, version=3)
        self.add_output_to_create_multi_result(dusty_tx)
        assert_raises_rpc_error(-26, "dust", self.nodes[0].sendrawtransaction, dusty_tx["hex"])

        block_res = self.nodes[0].rpc.generateblock(self.wallet.get_address(), [dusty_tx["hex"]])
        self.nodes[0].invalidateblock(block_res["hash"])
        self.assert_mempool_contents(expected=[dusty_tx["tx"]])

        # Create a sweep that has dust of its own and leaves dusty_tx's dust unspent
        sweep_tx = self.wallet.create_self_transfer_multi(fee_per_output=0, utxos_to_spend=[dusty_tx["new_utxos"][0]], version=3)
        self.add_output_to_create_multi_result(sweep_tx)
        assert_raises_rpc_error(-26, "dust", self.nodes[0].sendrawtransaction, sweep_tx["hex"])

        # Mine the sweep then re-org, the sweep will make it even with unspent dusty_tx dust
        block_res = self.nodes[0].rpc.generateblock(self.wallet.get_address(), [dusty_tx["hex"], sweep_tx["hex"]])
        self.nodes[0].invalidateblock(block_res["hash"])
        self.assert_mempool_contents(expected=[dusty_tx["tx"], sweep_tx["tx"]])

        # Also should happen if dust is swept
        sweep_tx_2 = self.wallet.create_self_transfer_multi(fee_per_output=0, utxos_to_spend=dusty_tx["new_utxos"], version=3)
        self.add_output_to_create_multi_result(sweep_tx)
        assert_raises_rpc_error(-26, "dust", self.nodes[0].sendrawtransaction, sweep_tx["hex"])

        reconsider_block_res = self.nodes[0].rpc.generateblock(self.wallet.get_address(), [dusty_tx["hex"], sweep_tx_2["hex"]])
        self.nodes[0].invalidateblock(reconsider_block_res["hash"])
        self.assert_mempool_contents(expected=[dusty_tx["tx"], sweep_tx_2["tx"]])

        # TRUC transactions restriction for ephemeral dust disallows further spends of ancestor chains
        child_tx = self.wallet.create_self_transfer_multi(utxos_to_spend=sweep_tx_2["new_utxos"], version=3)
        assert_raises_rpc_error(-26, "TRUC-violation", self.nodes[0].sendrawtransaction, child_tx["hex"])

        self.nodes[0].reconsiderblock(reconsider_block_res["hash"])
        assert_equal(self.nodes[0].getrawmempool(), [])

        self.log.info("Test that ephemeral dust tx with fees or multi dust don't enter mempool via reorg")
        multi_dusty_tx = self.wallet.create_self_transfer_multi(fee_per_output=0, version=3)
        self.add_output_to_create_multi_result(multi_dusty_tx)
        self.add_output_to_create_multi_result(multi_dusty_tx)

        block_res = self.nodes[0].rpc.generateblock(self.wallet.get_address(), [multi_dusty_tx["hex"]])
        self.nodes[0].invalidateblock(block_res["hash"])
        assert_equal(self.nodes[0].getrawmempool(), [])

        # With fee and one dust
        dusty_fee_tx = self.wallet.create_self_transfer_multi(fee_per_output=1, version=3)
        self.add_output_to_create_multi_result(dusty_fee_tx)

        block_res = self.nodes[0].rpc.generateblock(self.wallet.get_address(), [dusty_fee_tx["hex"]])
        self.nodes[0].invalidateblock(block_res["hash"])
        assert_equal(self.nodes[0].getrawmempool(), [])

        # Re-connect and make sure we have same state still
        self.connect_nodes(0, 1)
        self.sync_all()

    def test_acceptnonstd(self):
        self.log.info("Test that dust is still allowed when acceptnonstd=1")
        self.restart_node(0, extra_args=["-acceptnonstdtxn=1"])
        self.restart_node(1, extra_args=["-acceptnonstdtxn=1"])
        self.connect_nodes(0, 1)

        assert_equal(self.nodes[0].getrawmempool(), [])

        # Double dust, both unspent, with fees. Would have failed individual and package checks.
        dusty_tx = self.wallet.create_self_transfer_multi(fee_per_output=1000, version=3)
        self.add_output_to_create_multi_result(dusty_tx)
        self.add_output_to_create_multi_result(dusty_tx)
        self.nodes[0].rpc.sendrawtransaction(dusty_tx["hex"])

        self.assert_mempool_contents(expected=[dusty_tx["tx"]])

        self.restart_node(0, extra_args=[])
        self.restart_node(1, extra_args=[])
        self.connect_nodes(0, 1)

        assert_equal(self.nodes[0].getrawmempool(), [])

    # N.B. this extra_args can be removed post cluster mempool
    def test_free_relay(self):
        self.log.info("Test that ephemeral dust works in non-TRUC contexts when there's no minrelay requirement")

        # Note: since minrelay is 0, it is not testing 1P1C relay
        self.restart_node(0, extra_args=["-minrelaytxfee=0"])
        self.restart_node(1, extra_args=["-minrelaytxfee=0"])
        self.connect_nodes(0, 1)

        assert_equal(self.nodes[0].getrawmempool(), [])

        dusty_tx = self.wallet.create_self_transfer_multi(fee_per_output=0, version=2)
        self.add_output_to_create_multi_result(dusty_tx)

        sweep_tx = self.wallet.create_self_transfer_multi(utxos_to_spend=dusty_tx["new_utxos"], version=2)

        self.nodes[0].submitpackage([dusty_tx["hex"], sweep_tx["hex"]])

        self.assert_mempool_contents(expected=[dusty_tx["tx"], sweep_tx["tx"]])

        # generate coins for next tests
        self.generate(self.nodes[0], 1)
        self.wallet.rescan_utxos()
        assert_equal(self.nodes[0].getrawmempool(), [])

        self.log.info("Test batched ephemeral dust sweep")
        dusty_txs = []
        for _ in range(24):
            dusty_txs.append(self.wallet.create_self_transfer_multi(fee_per_output=0, version=2))
            self.add_output_to_create_multi_result(dusty_txs[-1])

        all_parent_utxos = [utxo for tx in dusty_txs for utxo in tx["new_utxos"]]

        # Missing one dust spend spend from a single parent, nothing in mempool
        insufficient_sweep_tx = self.wallet.create_self_transfer_multi(fee_per_output=25000, utxos_to_spend=all_parent_utxos[:-1], version=2)

        res = self.nodes[0].submitpackage([dusty_tx["hex"] for dusty_tx in dusty_txs] + [insufficient_sweep_tx["hex"]])
        assert_equal(res['package_msg'], "unspent-dust")
        assert_equal(self.nodes[0].getrawmempool(), [])

        # Next put some parents in mempool, but not others, and test unspent-dust again with all parents spent
        B_coin = self.wallet.get_utxo() # coin to cycle out CPFP
        sweep_all_but_one_tx = self.wallet.create_self_transfer_multi(fee_per_output=20000, utxos_to_spend=all_parent_utxos[:-2] + [B_coin], version=2)
        res = self.nodes[0].submitpackage([dusty_tx["hex"] for dusty_tx in dusty_txs[:-1]] + [sweep_all_but_one_tx["hex"]])
        assert_equal(res['package_msg'], "success")
        self.assert_mempool_contents(expected=[dusty_tx["tx"] for dusty_tx in dusty_txs[:-1]] + [sweep_all_but_one_tx["tx"]])

        res = self.nodes[0].submitpackage([dusty_tx["hex"] for dusty_tx in dusty_txs] + [insufficient_sweep_tx["hex"]])
        assert_equal(res['package_msg'], "unspent-dust")
        self.assert_mempool_contents(expected=[dusty_tx["tx"] for dusty_tx in dusty_txs[:-1]] + [sweep_all_but_one_tx["tx"]])

        # Cycle out the partial sweep to avoid triggering package RBF behavior which limits package to no in-mempool ancestors
        cancel_sweep = self.wallet.create_self_transfer_multi(fee_per_output=21000, utxos_to_spend=[B_coin], version=2)
        self.nodes[0].sendrawtransaction(cancel_sweep["hex"])
        self.assert_mempool_contents(expected=[dusty_tx["tx"] for dusty_tx in dusty_txs[:-1]] + [cancel_sweep["tx"]])

        # Sweeps all dust, where most are already in-mempool
        sweep_tx = self.wallet.create_self_transfer_multi(fee_per_output=25000, utxos_to_spend=all_parent_utxos, version=2)

        res = self.nodes[0].submitpackage([dusty_tx["hex"] for dusty_tx in dusty_txs] + [sweep_tx["hex"]])
        assert_equal(res['package_msg'], "success")
        self.assert_mempool_contents(expected=[dusty_tx["tx"] for dusty_tx in dusty_txs] + [sweep_tx["tx"], cancel_sweep["tx"]])

        self.generate(self.nodes[0], 25)
        self.wallet.rescan_utxos()
        assert_equal(self.nodes[0].getrawmempool(), [])

        # Other topology tests require relaxation of submitpackage topology

        self.restart_node(0, extra_args=[])
        self.restart_node(1, extra_args=[])
        self.connect_nodes(0, 1)

        assert_equal(self.nodes[0].getrawmempool(), [])

if __name__ == "__main__":
    EphemeralDustTest(__file__).main()
