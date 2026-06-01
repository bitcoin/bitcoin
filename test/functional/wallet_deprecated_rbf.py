#!/usr/bin/env python3
# Copyright (c) 2026-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test deprecation of wallet RBF RPCs and startup options."""
from test_framework.messages import MAX_BIP125_RBF_SEQUENCE
from test_framework.util import assert_equal, assert_greater_than, assert_raises_rpc_error
from test_framework.test_framework import BitcoinTestFramework

'''
This test exercises the deprecatedrpc=bip125 flag and deprecated -walletrbf options
and should be removed when the options are removed post deprecation.
'''
class WalletDeprecatedRBFTest(BitcoinTestFramework):
    def set_test_params(self):
      self.num_nodes = 1
      self.setup_clean_chain = True
      self.extra_args = [[]]

    def skip_test_if_missing_module(self):
      self.skip_if_no_wallet()

    def run_test(self):
      self.log.info("Test -deprecatedrpc=bip125 and -walletrbf startup option")

      self.restart_node(0, extra_args=["-walletrbf=0", "-deprecatedrpc=bip125"])
      node = self.nodes[0]
      node.createwallet("deprecated_optinrbf")
      wallet = node.get_wallet_rpc("deprecated_optinrbf")
      self.generatetoaddress(node, nblocks=101, address=wallet.getnewaddress(), sync_fun=self.no_op)

      def get_largest_unspent():
        return max(wallet.listunspent(), key=lambda unspent: unspent["amount"])

      def assert_rbf_in_tx_hex(tx_hex):
        for input in wallet.decoderawtransaction(tx_hex)["vin"]:
          assert_equal(input["sequence"], MAX_BIP125_RBF_SEQUENCE)

      def assert_rbf_in_psbt(encoded_psbt):
        decoded_psbt = wallet.decodepsbt(encoded_psbt)
        for input in decoded_psbt["inputs"]:
          assert_equal(input["sequence"], MAX_BIP125_RBF_SEQUENCE)

      def assert_rbf_in_wallet_tx(tx_id):
        wallet_mempool_tx = wallet.gettransaction(tx_id)
        assert_equal("bip125-replaceable" in wallet_mempool_tx, True)
        assert_equal(wallet_mempool_tx["bip125-replaceable"], "yes")

      # raw transaction
      unspent = get_largest_unspent()
      # Test `createrawtransaction` mismatch between sequence number(s) and `replaceable` option
      assert_raises_rpc_error(-8, "Invalid parameter combination: Sequence number(s) contradict replaceable option",
                              self.nodes[0].createrawtransaction, [{'txid': unspent["txid"], 'vout': unspent["vout"], 'sequence': MAX_BIP125_RBF_SEQUENCE+1}], {}, 0, True)

      raw_tx_hex = wallet.createrawtransaction(inputs=[unspent], outputs=[{wallet.getnewaddress(): 1}], replaceable=True)
      assert_rbf_in_tx_hex(raw_tx_hex)
      funded_tx_hex = wallet.fundrawtransaction(raw_tx_hex)["hex"]
      assert_rbf_in_tx_hex(raw_tx_hex)
      signed_tx_hex = wallet.signrawtransactionwithwallet(funded_tx_hex)["hex"]
      sent_tx_id = wallet.sendrawtransaction(signed_tx_hex)
      assert_rbf_in_wallet_tx(sent_tx_id)
      bumped_tx_details = wallet.bumpfee(sent_tx_id, replaceable=True)
      assert_greater_than(bumped_tx_details["fee"], bumped_tx_details["origfee"])
      assert_rbf_in_wallet_tx(bumped_tx_details["txid"])
      wallet.sendrawtransaction(wallet.gettransaction(bumped_tx_details["txid"])["hex"])
      self.generate(node, 1, sync_fun=self.no_op)

      # psbt
      unspent = get_largest_unspent()
      inputs = [{"txid": unspent["txid"], "vout": unspent["vout"]}]
      outputs = [{wallet.getnewaddress(): 1}]
      psbt_encoded = wallet.createpsbt(inputs=inputs, outputs=outputs, replaceable=True)
      assert_rbf_in_psbt(psbt_encoded)
      funded_psbt_encoded = wallet.walletcreatefundedpsbt(inputs=inputs, outputs=outputs, replaceable=True)["psbt"]
      assert_rbf_in_psbt(funded_psbt_encoded)
      signed_tx_hex = wallet.walletprocesspsbt(funded_psbt_encoded)["hex"]
      sent_tx_id = wallet.sendrawtransaction(signed_tx_hex)
      bumped_tx_details = wallet.psbtbumpfee(sent_tx_id, replaceable=True)
      assert_greater_than(bumped_tx_details["fee"], bumped_tx_details["origfee"])
      signed_bumped_tx_hex = wallet.walletprocesspsbt(bumped_tx_details["psbt"])["hex"]
      sent_tx_id = wallet.sendrawtransaction(signed_bumped_tx_hex)
      assert_rbf_in_wallet_tx(sent_tx_id)
      self.generate(node, 1, sync_fun=self.no_op)

      rpcs = [
        lambda: wallet.send(outputs=[{wallet.getnewaddress(): 1}], options={"replaceable": True})["txid"],
        lambda: wallet.sendtoaddress(address=wallet.getnewaddress(), amount=1, replaceable=True),
        lambda: wallet.sendmany(amounts={wallet.getnewaddress():1, wallet.getnewaddress():1}, replaceable=True),
        lambda: wallet.sendall(recipients=[wallet.getnewaddress()], replaceable=True)["txid"],
      ]

      for rpc in rpcs:
        sent_tx_id = rpc()
        assert_rbf_in_wallet_tx(sent_tx_id)
        self.generate(node, 1, sync_fun=self.no_op)

      self.stop_node(0, expected_stderr="Warning: -walletrbf is deprecated and will be fully removed in the next release.")


if __name__ == '__main__':
    WalletDeprecatedRBFTest(__file__).main()
