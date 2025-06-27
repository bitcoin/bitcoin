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
from test_framework.mempool_util import assert_mempool_contents
from test_framework.util import (
    assert_equal,
    assert_greater_than,
    assert_raises_rpc_error,
    assert_not_equal,
)
from test_framework.wallet import (
    MiniWallet,
)

class EphemeralDustTest(BitcoinTestFramework):
    def set_test_params(self):
        # Mempools should match via 1P1C p2p relay
        self.num_nodes = 2

        # Don't test trickling logic
        self.noban_tx_relay = True

    def add_output_to_create_multi_result(self, result, output_value=0):
        """ Add output without changing absolute tx fee
        """
        assert len(result["tx"].vout) > 0
        assert result["tx"].vout[0].nValue >= output_value
        result["tx"].vout.append(CTxOut(output_value, result["tx"].vout[0].scriptPubKey))
        # Take value from first output
        result["tx"].vout[0].nValue -= output_value
        result["new_utxos"][0]["value"] = Decimal(result["tx"].vout[0].nValue) / COIN
        new_txid = result["tx"].txid_hex
        result["txid"]  = new_txid
        result["wtxid"] = result["tx"].wtxid_hex
        result["hex"] = result["tx"].serialize().hex()
        for new_utxo in result["new_utxos"]:
            new_utxo["txid"] = new_txid
            new_utxo["wtxid"] = result["tx"].wtxid_hex

        result["new_utxos"].append({"txid": new_txid, "vout": len(result["tx"].vout) - 1, "value": Decimal(output_value) / COIN, "height": 0, "coinbase": False, "confirmations": 0})

    def create_ephemeral_dust_package(self, *, tx_version, dust_tx_fee=0, dust_value=0, num_dust_outputs=1, extra_sponsors=None):
        """Creates a 1P1C package containing ephemeral dust. By default, the parent transaction
           is zero-fee and creates a single zero-value dust output, and all of its outputs are
           spent by the child."""
        dusty_tx = self.wallet.create_self_transfer_multi(fee_per_output=dust_tx_fee, version=tx_version)
        for _ in range(num_dust_outputs):
            self.add_output_to_create_multi_result(dusty_tx, dust_value)

        extra_sponsors = extra_sponsors or []
        sweep_tx = self.wallet.create_self_transfer_multi(
            utxos_to_spend=dusty_tx["new_utxos"] + extra_sponsors,
            version=tx_version,
        )

        return dusty_tx, sweep_tx

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
        self.test_no_minrelay_fee()

    def test_normal_dust(self):
        self.log.info("Create 0-value dusty output, show that it works inside truc when spent in package")

        assert_equal(self.nodes[0].getrawmempool(), [])
        dusty_tx, sweep_tx = self.create_ephemeral_dust_package(tx_version=3)

        # Test doesn't work because lack of package feerates
        test_res = self.nodes[0].testmempoolaccept([dusty_tx["hex"], sweep_tx["hex"]])
        assert not test_res[0]["allowed"]
        assert_equal(test_res[0]["reject-reason"], "min relay fee not met")

        # And doesn't work on its own
        assert_raises_rpc_error(-26, "min relay fee not met", self.nodes[0].sendrawtransaction, dusty_tx["hex"])

        # If we add modified fees, it is still not allowed due to dust check
        self.nodes[0].prioritisetransaction(txid=dusty_tx["txid"], dummy=0, fee_delta=COIN)
        test_res = self.nodes[0].testmempoolaccept([dusty_tx["hex"]])
        assert not test_res[0]["allowed"]
        assert_equal(test_res[0]["reject-reason"], "dust")
        # Reset priority
        self.nodes[0].prioritisetransaction(txid=dusty_tx["txid"], dummy=0, fee_delta=-COIN)
        assert_equal(self.nodes[0].getprioritisedtransactions(), {})

        # Package evaluation succeeds
        res = self.nodes[0].submitpackage([dusty_tx["hex"], sweep_tx["hex"]])
        assert_equal(res["package_msg"], "success")
        assert_mempool_contents(self, self.nodes[0], expected=[dusty_tx["tx"], sweep_tx["tx"]])

        # Entry is denied when non-0-fee, either base or unmodified.
        # If in-mempool, we're not allowed to prioritise due to detected dust output
        assert_raises_rpc_error(-8, "Priority is not supported for transactions with dust outputs.", self.nodes[0].prioritisetransaction, dusty_tx["txid"], 0, 1)
        assert_equal(self.nodes[0].getprioritisedtransactions(), {})

        self.generate(self.nodes[0], 1)
        assert_equal(self.nodes[0].getrawmempool(), [])

    def test_node_restart(self):
        self.log.info("Test that an ephemeral package is rejected on restart due to individual evaluation")

        assert_equal(self.nodes[0].getrawmempool(), [])
        dusty_tx, sweep_tx = self.create_ephemeral_dust_package(tx_version=3)

        res = self.nodes[0].submitpackage([dusty_tx["hex"], sweep_tx["hex"]])
        assert_equal(res["package_msg"], "success")
        assert_equal(len(self.nodes[0].getrawmempool()), 2)
        assert_mempool_contents(self, self.nodes[0], expected=[dusty_tx["tx"], sweep_tx["tx"]])

        # Node restart; doesn't allow ephemeral transaction back in due to individual submission
        # resulting in 0-fee. Supporting re-submission of CPFP packages on restart is desired but not
        # yet implemented.
        self.restart_node(0)
        self.restart_node(1)
        self.connect_nodes(0, 1)
        assert_mempool_contents(self, self.nodes[0], expected=[])

    def test_fee_having_parent(self):
        self.log.info("Test that a transaction with ephemeral dust may not have non-0 base fee")

        assert_equal(self.nodes[0].getrawmempool(), [])

        sats_fee = 1
        dusty_tx, sweep_tx = self.create_ephemeral_dust_package(tx_version=3, dust_tx_fee=sats_fee)
        assert_equal(int(COIN * dusty_tx["fee"]), sats_fee) # has fees
        assert_greater_than(dusty_tx["tx"].vout[0].nValue, 330) # main output is not dust
        assert_equal(dusty_tx["tx"].vout[1].nValue, 0) # added one is dust

        # When base fee is non-0, we report dust like usual
        res = self.nodes[0].submitpackage([dusty_tx["hex"], sweep_tx["hex"]])
        assert_equal(res["package_msg"], "transaction failed")
        assert_equal(res["tx-results"][dusty_tx["wtxid"]]["error"], "dust, tx with dust output must be 0-fee")

        # Priority is ignored: rejected even if modified fee is 0
        self.nodes[0].prioritisetransaction(txid=dusty_tx["txid"], dummy=0, fee_delta=-sats_fee)
        self.nodes[1].prioritisetransaction(txid=dusty_tx["txid"], dummy=0, fee_delta=-sats_fee)
        res = self.nodes[0].submitpackage([dusty_tx["hex"], sweep_tx["hex"]])
        assert_equal(res["package_msg"], "transaction failed")
        assert_equal(res["tx-results"][dusty_tx["wtxid"]]["error"], "dust, tx with dust output must be 0-fee")

        # Will not be accepted if base fee is 0 with modified fee of non-0
        dusty_tx, sweep_tx = self.create_ephemeral_dust_package(tx_version=3)

        self.nodes[0].prioritisetransaction(txid=dusty_tx["txid"], dummy=0, fee_delta=1000)
        self.nodes[1].prioritisetransaction(txid=dusty_tx["txid"], dummy=0, fee_delta=1000)

        # It's rejected submitted alone
        test_res = self.nodes[0].testmempoolaccept([dusty_tx["hex"]])
        assert not test_res[0]["allowed"]
        assert_equal(test_res[0]["reject-reason"], "dust")

        # Or as a package
        res = self.nodes[0].submitpackage([dusty_tx["hex"], sweep_tx["hex"]])
        assert_equal(res["package_msg"], "transaction failed")
        assert_equal(res["tx-results"][dusty_tx["wtxid"]]["error"], "dust, tx with dust output must be 0-fee")

        assert_mempool_contents(self, self.nodes[0], expected=[])

    def test_multidust(self):
        self.log.info("Test that a transaction with multiple ephemeral dusts is not allowed")

        assert_mempool_contents(self, self.nodes[0], expected=[])
        dusty_tx, sweep_tx = self.create_ephemeral_dust_package(tx_version=3, num_dust_outputs=2)

        res = self.nodes[0].submitpackage([dusty_tx["hex"], sweep_tx["hex"]])
        assert_equal(res["package_msg"], "transaction failed")
        assert_equal(res["tx-results"][dusty_tx["wtxid"]]["error"], "dust")
        assert_equal(self.nodes[0].getrawmempool(), [])

    def test_nonzero_dust(self):
        self.log.info("Test that a single output of any satoshi amount is allowed, not checking spending")

        # We aren't checking spending, allow it in with no fee
        self.restart_node(0, extra_args=["-minrelaytxfee=0"])
        self.restart_node(1, extra_args=["-minrelaytxfee=0"])
        self.connect_nodes(0, 1)

        # 330 is dust threshold for taproot outputs
        for value in [1, 329, 330]:
            assert_equal(self.nodes[0].getrawmempool(), [])
            dusty_tx, _ = self.create_ephemeral_dust_package(tx_version=3, dust_value=value)
            test_res = self.nodes[0].testmempoolaccept([dusty_tx["hex"]])
            assert test_res[0]["allowed"]

        self.restart_node(0, extra_args=[])
        self.restart_node(1, extra_args=[])
        self.connect_nodes(0, 1)
        assert_mempool_contents(self, self.nodes[0], expected=[])

    # N.B. If individual minrelay requirement is dropped, this test can be dropped
    def test_non_truc(self):
        self.log.info("Test that v2 dust-having transaction is rejected even if spent, because of min relay requirement")

        assert_equal(self.nodes[0].getrawmempool(), [])
        dusty_tx, sweep_tx = self.create_ephemeral_dust_package(tx_version=2)

        res = self.nodes[0].submitpackage([dusty_tx["hex"], sweep_tx["hex"]])
        assert_equal(res["package_msg"], "transaction failed")
        assert_equal(res["tx-results"][dusty_tx["wtxid"]]["error"], "min relay fee not met, 0 < 147")

        assert_equal(self.nodes[0].getrawmempool(), [])

    def test_unspent_ephemeral(self):
        self.log.info("Test that spending from a tx with ephemeral outputs is only allowed if dust is spent as well")

        assert_equal(self.nodes[0].getrawmempool(), [])
        dusty_tx, sweep_tx = self.create_ephemeral_dust_package(tx_version=3, dust_value=329)

        # Valid sweep we will RBF incorrectly by not spending dust as well
        self.nodes[0].submitpackage([dusty_tx["hex"], sweep_tx["hex"]])
        assert_mempool_contents(self, self.nodes[0], expected=[dusty_tx["tx"], sweep_tx["tx"]])

        # Doesn't spend in-mempool dust output from parent
        unspent_sweep_tx = self.wallet.create_self_transfer_multi(fee_per_output=2000, utxos_to_spend=[dusty_tx["new_utxos"][0]], version=3)
        assert_greater_than(unspent_sweep_tx["fee"], sweep_tx["fee"])
        res = self.nodes[0].submitpackage([dusty_tx["hex"], unspent_sweep_tx["hex"]])
        assert_equal(res["tx-results"][unspent_sweep_tx["wtxid"]]["error"], f"missing-ephemeral-spends, tx {unspent_sweep_tx['txid']} (wtxid={unspent_sweep_tx['wtxid']}) did not spend parent's ephemeral dust")
        assert_raises_rpc_error(-26, f"missing-ephemeral-spends, tx {unspent_sweep_tx['txid']} (wtxid={unspent_sweep_tx['wtxid']}) did not spend parent's ephemeral dust", self.nodes[0].sendrawtransaction, unspent_sweep_tx["hex"])
        assert_mempool_contents(self, self.nodes[0], expected=[dusty_tx["tx"], sweep_tx["tx"]])

        # Spend works with dust spent
        sweep_tx_2 = self.wallet.create_self_transfer_multi(fee_per_output=2000, utxos_to_spend=dusty_tx["new_utxos"], version=3)
        assert_not_equal(sweep_tx["hex"], sweep_tx_2["hex"])
        res = self.nodes[0].submitpackage([dusty_tx["hex"], sweep_tx_2["hex"]])
        assert_equal(res["package_msg"], "success")

        # Re-set and test again with nothing from package in mempool this time
        self.generate(self.nodes[0], 1)
        assert_equal(self.nodes[0].getrawmempool(), [])

        dusty_tx, _ = self.create_ephemeral_dust_package(tx_version=3, dust_value=329)

        # Spend non-dust only
        unspent_sweep_tx = self.wallet.create_self_transfer_multi(utxos_to_spend=[dusty_tx["new_utxos"][0]], version=3)

        res = self.nodes[0].submitpackage([dusty_tx["hex"], unspent_sweep_tx["hex"]])
        assert_equal(res["package_msg"], "unspent-dust")

        assert_equal(self.nodes[0].getrawmempool(), [])

        # Now spend dust only which should work
        second_coin = self.wallet.get_utxo() # another fee-bringing coin
        sweep_tx = self.wallet.create_self_transfer_multi(utxos_to_spend=[dusty_tx["new_utxos"][1], second_coin], version=3)

        res = self.nodes[0].submitpackage([dusty_tx["hex"], sweep_tx["hex"]])
        assert_equal(res["package_msg"], "success")
        assert_mempool_contents(self, self.nodes[0], expected=[dusty_tx["tx"], sweep_tx["tx"]])

        self.generate(self.nodes[0], 1)
        assert_mempool_contents(self, self.nodes[0], expected=[])

    def test_sponsor_cycle(self):
        self.log.info("Test that dust txn is not evicted when it becomes childless, but won't be mined")

        assert_equal(self.nodes[0].getrawmempool(), [])
        sponsor_coin = self.wallet.get_utxo()
        # Bring "fee" input that can be double-spend separately
        dusty_tx, sweep_tx = self.create_ephemeral_dust_package(tx_version=3, extra_sponsors=[sponsor_coin])

        res = self.nodes[0].submitpackage([dusty_tx["hex"], sweep_tx["hex"]])
        assert_equal(res["package_msg"], "success")
        assert_equal(len(self.nodes[0].getrawmempool()), 2)
        # sync to make sure unsponsor_tx hits second node's mempool after initial package
        assert_mempool_contents(self, self.nodes[0], expected=[dusty_tx["tx"], sweep_tx["tx"]])

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

        assert_mempool_contents(self, self.nodes[0], expected=[dusty_tx["tx"], unsponsor_tx["tx"]])

        # Dust tx is not mined
        self.generate(self.nodes[0], 1)
        assert_mempool_contents(self, self.nodes[0], expected=[dusty_tx["tx"]])

        # Create sweep that doesn't spend conflicting sponsor coin
        sweep_tx = self.wallet.create_self_transfer_multi(utxos_to_spend=dusty_tx["new_utxos"], version=3)

        # Can resweep
        self.nodes[0].sendrawtransaction(sweep_tx["hex"])
        assert_mempool_contents(self, self.nodes[0], expected=[dusty_tx["tx"], sweep_tx["tx"]])

        self.generate(self.nodes[0], 1)
        assert_mempool_contents(self, self.nodes[0], expected=[])

    def test_reorgs(self):
        self.log.info("Test that reorgs breaking the truc topology doesn't cause issues")

        assert_equal(self.nodes[0].getrawmempool(), [])

        # Many shallow re-orgs confuse block gossiping making test less reliable otherwise
        self.disconnect_nodes(0, 1)

        # Get dusty tx mined, then check that it makes it back into mempool on reorg
        # due to bypass_limits allowing 0-fee individually
        dusty_tx, _ = self.create_ephemeral_dust_package(tx_version=3)
        assert_raises_rpc_error(-26, "min relay fee not met", self.nodes[0].sendrawtransaction, dusty_tx["hex"])

        block_res = self.generateblock(self.nodes[0], self.wallet.get_address(), [dusty_tx["hex"]], sync_fun=self.no_op)
        self.nodes[0].invalidateblock(block_res["hash"])
        assert_mempool_contents(self, self.nodes[0], expected=[dusty_tx["tx"]], sync=False)

        # Create a sweep that has dust of its own and leaves dusty_tx's dust unspent
        sweep_tx = self.wallet.create_self_transfer_multi(fee_per_output=0, utxos_to_spend=[dusty_tx["new_utxos"][0]], version=3)
        self.add_output_to_create_multi_result(sweep_tx)
        assert_raises_rpc_error(-26, "min relay fee not met", self.nodes[0].sendrawtransaction, sweep_tx["hex"])

        # Mine the sweep then re-org, the sweep will not make it back in due to spend checks
        block_res = self.generateblock(self.nodes[0], self.wallet.get_address(), [dusty_tx["hex"], sweep_tx["hex"]], sync_fun=self.no_op)
        self.nodes[0].invalidateblock(block_res["hash"])
        assert_mempool_contents(self, self.nodes[0], expected=[dusty_tx["tx"]], sync=False)

        # Should re-enter if dust is swept
        sweep_tx_2 = self.wallet.create_self_transfer_multi(fee_per_output=0, utxos_to_spend=dusty_tx["new_utxos"], version=3)
        self.add_output_to_create_multi_result(sweep_tx_2)
        assert_raises_rpc_error(-26, "min relay fee not met", self.nodes[0].sendrawtransaction, sweep_tx_2["hex"])

        reconsider_block_res = self.generateblock(self.nodes[0], self.wallet.get_address(), [dusty_tx["hex"], sweep_tx_2["hex"]], sync_fun=self.no_op)
        self.nodes[0].invalidateblock(reconsider_block_res["hash"])
        assert_mempool_contents(self, self.nodes[0], expected=[dusty_tx["tx"], sweep_tx_2["tx"]], sync=False)

        # TRUC transactions restriction for ephemeral dust disallows further spends of ancestor chains
        child_tx = self.wallet.create_self_transfer_multi(utxos_to_spend=sweep_tx_2["new_utxos"], version=3)
        assert_raises_rpc_error(-26, "TRUC-violation", self.nodes[0].sendrawtransaction, child_tx["hex"])

        self.nodes[0].reconsiderblock(reconsider_block_res["hash"])
        assert_equal(self.nodes[0].getrawmempool(), [])

        self.log.info("Test that ephemeral dust tx with fees or multi dust don't enter mempool via reorg")
        multi_dusty_tx, _ = self.create_ephemeral_dust_package(tx_version=3, num_dust_outputs=2)
        block_res = self.generateblock(self.nodes[0], self.wallet.get_address(), [multi_dusty_tx["hex"]], sync_fun=self.no_op)
        self.nodes[0].invalidateblock(block_res["hash"])
        assert_equal(self.nodes[0].getrawmempool(), [])

        # With fee and one dust
        dusty_fee_tx, _ = self.create_ephemeral_dust_package(tx_version=3, dust_tx_fee=1)
        block_res = self.generateblock(self.nodes[0], self.wallet.get_address(), [dusty_fee_tx["hex"]], sync_fun=self.no_op)
        self.nodes[0].invalidateblock(block_res["hash"])
        assert_equal(self.nodes[0].getrawmempool(), [])

        # Re-connect and make sure we have same state still
        self.connect_nodes(0, 1)
        self.sync_all()

    # N.B. this extra_args can be removed post cluster mempool
    def test_no_minrelay_fee(self):
        self.log.info("Test that ephemeral dust works in non-TRUC contexts when there's no minrelay requirement")

        # Note: since minrelay is 0, it is not testing 1P1C relay
        self.restart_node(0, extra_args=["-minrelaytxfee=0"])
        self.restart_node(1, extra_args=["-minrelaytxfee=0"])
        self.connect_nodes(0, 1)

        assert_equal(self.nodes[0].getrawmempool(), [])
        dusty_tx, sweep_tx = self.create_ephemeral_dust_package(tx_version=2)

        self.nodes[0].submitpackage([dusty_tx["hex"], sweep_tx["hex"]])

        assert_mempool_contents(self, self.nodes[0], expected=[dusty_tx["tx"], sweep_tx["tx"]])

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

        # Missing one dust spend from a single parent, child rejected
        insufficient_sweep_tx = self.wallet.create_self_transfer_multi(fee_per_output=25000, utxos_to_spend=all_parent_utxos[:-1], version=2)

        res = self.nodes[0].submitpackage([dusty_tx["hex"] for dusty_tx in dusty_txs] + [insufficient_sweep_tx["hex"]])
        assert_equal(res['package_msg'], "transaction failed")
        assert_equal(res['tx-results'][insufficient_sweep_tx['wtxid']]['error'], f"missing-ephemeral-spends, tx {insufficient_sweep_tx['txid']} (wtxid={insufficient_sweep_tx['wtxid']}) did not spend parent's ephemeral dust")
        # Everything got in except for insufficient spend
        assert_mempool_contents(self, self.nodes[0], expected=[dusty_tx["tx"] for dusty_tx in dusty_txs])

        # Next put some parents in mempool, but not others, and test unspent dust again with all parents spent
        B_coin = self.wallet.get_utxo() # coin to cycle out CPFP
        sweep_all_but_one_tx = self.wallet.create_self_transfer_multi(fee_per_output=20000, utxos_to_spend=all_parent_utxos[:-2] + [B_coin], version=2)
        res = self.nodes[0].submitpackage([dusty_tx["hex"] for dusty_tx in dusty_txs[:-1]] + [sweep_all_but_one_tx["hex"]])
        assert_equal(res['package_msg'], "success")
        assert_mempool_contents(self, self.nodes[0], expected=[dusty_tx["tx"] for dusty_tx in dusty_txs] + [sweep_all_but_one_tx["tx"]])

        res = self.nodes[0].submitpackage([dusty_tx["hex"] for dusty_tx in dusty_txs] + [insufficient_sweep_tx["hex"]])
        assert_equal(res['package_msg'], "transaction failed")
        assert_equal(res['tx-results'][insufficient_sweep_tx["wtxid"]]["error"], f"missing-ephemeral-spends, tx {insufficient_sweep_tx['txid']} (wtxid={insufficient_sweep_tx['wtxid']}) did not spend parent's ephemeral dust")
        assert_mempool_contents(self, self.nodes[0], expected=[dusty_tx["tx"] for dusty_tx in dusty_txs] + [sweep_all_but_one_tx["tx"]])

        # Cycle out the partial sweep to avoid triggering package RBF behavior which limits package to no in-mempool ancestors
        cancel_sweep = self.wallet.create_self_transfer_multi(fee_per_output=21000, utxos_to_spend=[B_coin], version=2)
        self.nodes[0].sendrawtransaction(cancel_sweep["hex"])
        assert_mempool_contents(self, self.nodes[0], expected=[dusty_tx["tx"] for dusty_tx in dusty_txs] + [cancel_sweep["tx"]])

        # Sweeps all dust, where all dusty txs are already in-mempool
        sweep_tx = self.wallet.create_self_transfer_multi(fee_per_output=25000, utxos_to_spend=all_parent_utxos, version=2)

        # N.B. Since we have multiple parents these are not propagating via 1P1C relay.
        # minrelay being zero allows them to propagate on their own.
        res = self.nodes[0].submitpackage([dusty_tx["hex"] for dusty_tx in dusty_txs] + [sweep_tx["hex"]])
        assert_equal(res['package_msg'], "success")
        assert_mempool_contents(self, self.nodes[0], expected=[dusty_tx["tx"] for dusty_tx in dusty_txs] + [sweep_tx["tx"], cancel_sweep["tx"]])

        self.generate(self.nodes[0], 1)
        self.wallet.rescan_utxos()
        assert_equal(self.nodes[0].getrawmempool(), [])

        # Other topology tests (e.g., grandparents and parents both with dust) require relaxation of submitpackage topology

        self.restart_node(0, extra_args=[])
        self.restart_node(1, extra_args=[])
        self.connect_nodes(0, 1)

        assert_equal(self.nodes[0].getrawmempool(), [])

if __name__ == "__main__":
    EphemeralDustTest(__file__).main()
