#!/usr/bin/env python3
# Copyright (c) 2026-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test deprecation of wallet RBF RPCs and startup options."""
from test_framework.messages import MAX_BIP125_RBF_SEQUENCE
from test_framework.util import assert_equal, assert_not_equal, assert_greater_than, assert_raises_rpc_error, JSONRPCException
from test_framework.test_framework import BitcoinTestFramework

DEPRECATED_RPC_MESSAGE = "Deprecated \"replaceable\" argument passed. Run with -deprecatedrpc=bip125 startup option to use it."

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
      self.log.info("Test without and with -deprecatedrpc=bip125 and -walletrbf startup options")
      node = self.nodes[0]
      node.createwallet("deprecated_optinrbf")
      wallet = node.get_wallet_rpc("deprecated_optinrbf")
      self.generatetoaddress(node, nblocks=101, address=wallet.getnewaddress(), sync_fun=self.no_op)

      # helper functions
      def get_largest_unspent():
        return max(wallet.listunspent(), key=lambda unspent: unspent["amount"])

      def check_rbf_signalling(inputs, rbf_signalling=True):
        for input in inputs:
          if rbf_signalling:
            assert_equal(input["sequence"], MAX_BIP125_RBF_SEQUENCE)
          else:
            assert_not_equal(input["sequence"], MAX_BIP125_RBF_SEQUENCE)

      def assert_rbf_signalling_in_wallet_tx(tx_id):
        wallet_mempool_tx = wallet.gettransaction(tx_id)
        assert_equal("bip125-replaceable" in wallet_mempool_tx, True)
        assert_equal(wallet_mempool_tx["bip125-replaceable"], "yes")

      send_rpcs = [
        lambda: wallet.send(outputs=[{wallet.getnewaddress(): 1}], options={"replaceable": True})["txid"],
        lambda: wallet.sendtoaddress(address=wallet.getnewaddress(), amount=1, replaceable=True),
        lambda: wallet.sendmany(amounts={wallet.getnewaddress():1, wallet.getnewaddress():1}, replaceable=True),
        lambda: wallet.sendall(recipients=[wallet.getnewaddress()], replaceable=True)["txid"],
      ]

      # check for errors regarding usage of deprecated arguments without -deprecatedrpc startup option
      unspent = get_largest_unspent()
      assert_raises_rpc_error(-32, DEPRECATED_RPC_MESSAGE, wallet.createrawtransaction, inputs=[unspent], outputs=[{wallet.getnewaddress(): 1}], replaceable=True)
      assert_raises_rpc_error(-32, DEPRECATED_RPC_MESSAGE, wallet.createpsbt, inputs=[unspent], outputs=[{wallet.getnewaddress(): 1}], replaceable=True)
      for rpc in send_rpcs:
        try:
          rpc()
        except JSONRPCException as e:
          assert_equal(e.error["code"], -32)
          assert_equal(e.error["message"], DEPRECATED_RPC_MESSAGE)
        except Exception as e:
          raise AssertionError("Unexpected exception raised: " + type(e).__name__)

      # restart the node with deprecated startup options
      self.restart_node(0, extra_args=["-walletrbf=0", "-deprecatedrpc=bip125"])
      node.loadwallet("deprecated_optinrbf")
      wallet = node.get_wallet_rpc("deprecated_optinrbf")

      # Test `createrawtransaction` mismatch between sequence number(s) and `replaceable` option
      assert_raises_rpc_error(-8, "Invalid parameter combination: Sequence number(s) contradict replaceable option",
                              self.nodes[0].createrawtransaction, [{'txid': unspent["txid"], 'vout': unspent["vout"], 'sequence': MAX_BIP125_RBF_SEQUENCE+1}], {}, 0, True)

      # test raw transaction RPCs
      raw_tx_hex = wallet.createrawtransaction(inputs=[unspent], outputs=[{wallet.getnewaddress(): 1}], replaceable=False)
      check_rbf_signalling(wallet.decoderawtransaction(raw_tx_hex)["vin"], rbf_signalling=False)
      raw_tx_hex = wallet.createrawtransaction(inputs=[unspent], outputs=[{wallet.getnewaddress(): 1}], replaceable=True)
      check_rbf_signalling(wallet.decoderawtransaction(raw_tx_hex)["vin"])
      funded_tx_hex = wallet.fundrawtransaction(raw_tx_hex)["hex"]
      check_rbf_signalling(wallet.decoderawtransaction(funded_tx_hex)["vin"])
      signed_tx_hex = wallet.signrawtransactionwithwallet(funded_tx_hex)["hex"]
      sent_tx_id = wallet.sendrawtransaction(signed_tx_hex)
      assert_rbf_signalling_in_wallet_tx(sent_tx_id)
      bumped_tx_details = wallet.bumpfee(sent_tx_id, replaceable=True)
      assert_greater_than(bumped_tx_details["fee"], bumped_tx_details["origfee"])
      assert_rbf_signalling_in_wallet_tx(bumped_tx_details["txid"])
      wallet.sendrawtransaction(wallet.gettransaction(bumped_tx_details["txid"])["hex"])
      self.generate(node, 1, sync_fun=self.no_op)

      # test psbt RPCs (along with testing bumping non-replaceable passes)
      unspent = get_largest_unspent()
      inputs = [{"txid": unspent["txid"], "vout": unspent["vout"]}]
      outputs = [{wallet.getnewaddress(): 1}]
      psbt_encoded = wallet.createpsbt(inputs=inputs, outputs=outputs, replaceable=False)
      check_rbf_signalling(wallet.decodepsbt(psbt_encoded)["inputs"], rbf_signalling=False)
      funded_psbt_encoded = wallet.walletcreatefundedpsbt(inputs=inputs, outputs=outputs, replaceable=False)["psbt"]
      check_rbf_signalling(wallet.decodepsbt(funded_psbt_encoded)["inputs"], rbf_signalling=False)
      signed_tx_hex = wallet.walletprocesspsbt(funded_psbt_encoded)["hex"]
      sent_tx_id = wallet.sendrawtransaction(signed_tx_hex)
      bumped_tx_details = wallet.psbtbumpfee(sent_tx_id, replaceable=True)
      assert_greater_than(bumped_tx_details["fee"], bumped_tx_details["origfee"])
      signed_bumped_tx_hex = wallet.walletprocesspsbt(bumped_tx_details["psbt"])["hex"]
      sent_tx_id = wallet.sendrawtransaction(signed_bumped_tx_hex)
      assert_rbf_signalling_in_wallet_tx(sent_tx_id)
      self.generate(node, 1, sync_fun=self.no_op)

      for rpc in send_rpcs:
        sent_tx_id = rpc()
        assert_rbf_signalling_in_wallet_tx(sent_tx_id)
        self.generate(node, 1, sync_fun=self.no_op)

      self.stop_node(0, expected_stderr="Warning: -walletrbf is deprecated and will be fully removed in the next release.")


if __name__ == '__main__':
    WalletDeprecatedRBFTest(__file__).main()
