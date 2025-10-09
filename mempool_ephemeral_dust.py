def test_reorgs(self):
    self.log.info("Test that reorgs breaking the truc topology doesn't cause issues")

    assert_equal(self.nodes[0].getrawmempool(), [])

    # Many shallow re-orgs confuse block gossiping making test less reliable otherwise
    self.disconnect_nodes(0, 1)

    # Get dusty tx mined, then check that it makes it back into mempool on reorg
    # due to bypass_limits allowing 0-fee individually
    dusty_tx, _ = self.create_ephemeral_dust_package(tx_version=3)
    assert_raises_rpc_error(-26, "min relay fee not met", self.nodes[0].sendrawtransaction, dusty_tx["hex"])

    # === First invalidateblock instance replaced ===
    block_with_dust = self.generateblock(self.nodes[0], self.wallet.get_address(), [dusty_tx["hex"]], sync_fun=self.no_op)
    self.nodes[0].invalidateblock(block_with_dust["hash"])
    self.nodes[0].generatetoaddress(2, self.wallet.get_address())
    self.nodes[0].reconsiderblock(block_with_dust["hash"])
    assert_mempool_contents(self, self.nodes[0], expected=[dusty_tx["tx"]], sync=False)

    # Create a sweep that has dust of its own and leaves dusty_tx's dust unspent
    sweep_tx = self.wallet.create_self_transfer_multi(fee_per_output=0, utxos_to_spend=[dusty_tx["new_utxos"][0]], version=3)
    self.add_output_to_create_multi_result(sweep_tx)
    assert_raises_rpc_error(-26, "min relay fee not met", self.nodes[0].sendrawtransaction, sweep_tx["hex"])

    # === Second invalidateblock instance replaced ===
    block_with_sweep = self.generateblock(self.nodes[0], self.wallet.get_address(), [dusty_tx["hex"], sweep_tx["hex"]], sync_fun=self.no_op)
    self.nodes[0].invalidateblock(block_with_sweep["hash"])
    self.nodes[0].generatetoaddress(2, self.wallet.get_address())
    self.nodes[0].reconsiderblock(block_with_sweep["hash"])
    assert_mempool_contents(self, self.nodes[0], expected=[dusty_tx["tx"]], sync=False)

    # Should re-enter if dust is swept
    sweep_tx_2 = self.wallet.create_self_transfer_multi(fee_per_output=0, utxos_to_spend=dusty_tx["new_utxos"], version=3)
    self.add_output_to_create_multi_result(sweep_tx_2)
    assert_raises_rpc_error(-26, "min relay fee not met", self.nodes[0].sendrawtransaction, sweep_tx_2["hex"])

    # === Third invalidateblock instance replaced ===
    block_with_sweep2 = self.generateblock(self.nodes[0], self.wallet.get_address(), [dusty_tx["hex"], sweep_tx_2["hex"]], sync_fun=self.no_op)
    self.nodes[0].invalidateblock(block_with_sweep2["hash"])
    self.nodes[0].generatetoaddress(2, self.wallet.get_address())
    self.nodes[0].reconsiderblock(block_with_sweep2["hash"])
    assert_mempool_contents(self, self.nodes[0], expected=[dusty_tx["tx"], sweep_tx_2["tx"]], sync=False)

    # TRUC transactions restriction for ephemeral dust disallows further spends of ancestor chains
    child_tx = self.wallet.create_self_transfer_multi(utxos_to_spend=sweep_tx_2["new_utxos"], version=3)
    assert_raises_rpc_error(-26, "TRUC-violation", self.nodes[0].sendrawtransaction, child_tx["hex"])

    # === Fourth invalidateblock instance replaced ===
    self.nodes[0].reconsiderblock(block_with_sweep2["hash"])
    assert_equal(self.nodes[0].getrawmempool(), [])

    self.log.info("Test that ephemeral dust tx with fees or multi dust don't enter mempool via reorg")
    multi_dusty_tx, _ = self.create_ephemeral_dust_package(tx_version=3, num_dust_outputs=2)
    block_res = self.generateblock(self.nodes[0], self.wallet.get_address(), [multi_dusty_tx["hex"]], sync_fun=self.no_op)
    self.nodes[0].invalidateblock(block_res["hash"])
    self.nodes[0].generatetoaddress(2, self.wallet.get_address())
    self.nodes[0].reconsiderblock(block_res["hash"])
    assert_equal(self.nodes[0].getrawmempool(), [])

    # With fee and one dust
    dusty_fee_tx, _ = self.create_ephemeral_dust_package(tx_version=3, dust_tx_fee=1)
    block_res = self.generateblock(self.nodes[0], self.wallet.get_address(), [dusty_fee_tx["hex"]], sync_fun=self.no_op)
    self.nodes[0].invalidateblock(block_res["hash"])
    self.nodes[0].generatetoaddress(2, self.wallet.get_address())
    self.nodes[0].reconsiderblock(block_res["hash"])
    assert_equal(self.nodes[0].getrawmempool(), [])

    # Re-connect and make sure we have same state still
    self.connect_nodes(0, 1)
    self.sync_all()
