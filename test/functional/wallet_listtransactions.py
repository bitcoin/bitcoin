#!/usr/bin/env python3
# Copyright (c) 2014-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the listtransactions API."""

from decimal import Decimal
import time
import os
import shutil

from test_framework.blocktools import MAX_FUTURE_BLOCK_TIME
from test_framework.descriptors import descsum_create
from test_framework.extendedkey import ExtendedPrivateKey
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_not_equal,
    assert_array_result,
    assert_equal,
    assert_greater_than,
    assert_raises_rpc_error,
    find_vout_for_address,
)
from test_framework.wallet_util import get_generate_key


class ListTransactionsTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 3
        # whitelist peers to speed up tx relay / mempool sync
        self.noban_tx_relay = True

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        self.log.info("Test simple send from node0 to node1")
        txid = self.nodes[0].sendtoaddress(self.nodes[1].getnewaddress(), 0.1)
        self.sync_all()
        assert_array_result(self.nodes[0].listtransactions(),
                            {"txid": txid},
                            {"category": "send", "amount": Decimal("-0.1"), "confirmations": 0, "trusted": True})
        assert_array_result(self.nodes[1].listtransactions(),
                            {"txid": txid},
                            {"category": "receive", "amount": Decimal("0.1"), "confirmations": 0, "trusted": False})
        self.log.info("Test confirmations change after mining a block")
        blockhash = self.generate(self.nodes[0], 1)[0]
        blockheight = self.nodes[0].getblockheader(blockhash)['height']
        assert_array_result(self.nodes[0].listtransactions(),
                            {"txid": txid},
                            {"category": "send", "amount": Decimal("-0.1"), "confirmations": 1, "blockhash": blockhash, "blockheight": blockheight})
        assert_array_result(self.nodes[1].listtransactions(),
                            {"txid": txid},
                            {"category": "receive", "amount": Decimal("0.1"), "confirmations": 1, "blockhash": blockhash, "blockheight": blockheight})

        self.log.info("Test send-to-self on node0")
        txid = self.nodes[0].sendtoaddress(self.nodes[0].getnewaddress(), 0.2)
        assert_array_result(self.nodes[0].listtransactions(),
                            {"txid": txid, "category": "send"},
                            {"amount": Decimal("-0.2")})
        assert_array_result(self.nodes[0].listtransactions(),
                            {"txid": txid, "category": "receive"},
                            {"amount": Decimal("0.2")})

        self.log.info("Test sendmany from node1: twice to self, twice to node0")
        send_to = {self.nodes[0].getnewaddress(): 0.11,
                   self.nodes[1].getnewaddress(): 0.22,
                   self.nodes[0].getnewaddress(): 0.33,
                   self.nodes[1].getnewaddress(): 0.44}
        txid = self.nodes[1].sendmany("", send_to)
        self.sync_all()
        assert_array_result(self.nodes[1].listtransactions(),
                            {"category": "send", "amount": Decimal("-0.11")},
                            {"txid": txid})
        assert_array_result(self.nodes[0].listtransactions(),
                            {"category": "receive", "amount": Decimal("0.11")},
                            {"txid": txid})
        assert_array_result(self.nodes[1].listtransactions(),
                            {"category": "send", "amount": Decimal("-0.22")},
                            {"txid": txid})
        assert_array_result(self.nodes[1].listtransactions(),
                            {"category": "receive", "amount": Decimal("0.22")},
                            {"txid": txid})
        assert_array_result(self.nodes[1].listtransactions(),
                            {"category": "send", "amount": Decimal("-0.33")},
                            {"txid": txid})
        assert_array_result(self.nodes[0].listtransactions(),
                            {"category": "receive", "amount": Decimal("0.33")},
                            {"txid": txid})
        assert_array_result(self.nodes[1].listtransactions(),
                            {"category": "send", "amount": Decimal("-0.44")},
                            {"txid": txid})
        assert_array_result(self.nodes[1].listtransactions(),
                            {"category": "receive", "amount": Decimal("0.44")},
                            {"txid": txid})

        self.run_externally_generated_address_test()
        self.run_coinjoin_test()
        self.run_invalid_parameters_test()
        self.test_op_return()
        self.test_alternate_witness_tx()
        self.test_from_me_status_change()

    def run_externally_generated_address_test(self):
        """Test behavior when receiving address is not in the address book."""

        self.log.info("Setup the same wallet on two nodes")
        # refill keypool otherwise the second node wouldn't recognize addresses generated on the first nodes
        self.nodes[0].keypoolrefill(1000)
        self.stop_nodes()
        wallet0 = os.path.join(self.nodes[0].chain_path, self.default_wallet_name, "wallet.dat")
        wallet2 = os.path.join(self.nodes[2].chain_path, self.default_wallet_name, "wallet.dat")
        shutil.copyfile(wallet0, wallet2)
        self.start_nodes()
        # reconnect nodes
        self.connect_nodes(0, 1)
        self.connect_nodes(1, 2)
        self.connect_nodes(2, 0)

        addr1 = self.nodes[0].getnewaddress("pizza1", 'legacy')
        addr2 = self.nodes[0].getnewaddress("pizza2", 'p2sh-segwit')
        addr3 = self.nodes[0].getnewaddress("pizza3", 'bech32')

        self.log.info("Send to externally generated addresses")
        # send to an address beyond the next to be generated to test the keypool gap
        self.nodes[1].sendtoaddress(addr3, "0.001")
        self.generate(self.nodes[1], 1)

        # send to an address that is already marked as used due to the keypool gap mechanics
        self.nodes[1].sendtoaddress(addr2, "0.001")
        self.generate(self.nodes[1], 1)

        # send to self transaction
        self.nodes[0].sendtoaddress(addr1, "0.001")
        self.generate(self.nodes[0], 1)

        self.log.info("Verify listtransactions is the same regardless of where the address was generated")
        transactions0 = self.nodes[0].listtransactions()
        transactions2 = self.nodes[2].listtransactions()

        # normalize results: remove fields that normally could differ and sort
        def normalize_list(txs):
            for tx in txs:
                tx.pop('label', None)
                tx.pop('time', None)
                tx.pop('timereceived', None)
            txs.sort(key=lambda x: x['txid'])

        normalize_list(transactions0)
        normalize_list(transactions2)
        assert_equal(transactions0, transactions2)

        self.log.info("Verify labels are persistent on the node that generated the addresses")
        assert_equal(['pizza1'], self.nodes[0].getaddressinfo(addr1)['labels'])
        assert_equal(['pizza2'], self.nodes[0].getaddressinfo(addr2)['labels'])
        assert_equal(['pizza3'], self.nodes[0].getaddressinfo(addr3)['labels'])

    def run_coinjoin_test(self):
        self.log.info('Check "coin-join" transaction')
        input_0 = next(i for i in self.nodes[0].listunspent(query_options={"minimumAmount": 0.2}, include_unsafe=False))
        input_1 = next(i for i in self.nodes[1].listunspent(query_options={"minimumAmount": 0.2}, include_unsafe=False))
        raw_hex = self.nodes[0].createrawtransaction(
            inputs=[
                {
                    "txid": input_0["txid"],
                    "vout": input_0["vout"],
                },
                {
                    "txid": input_1["txid"],
                    "vout": input_1["vout"],
                },
            ],
            outputs={
                self.nodes[0].getnewaddress(): 0.123,
                self.nodes[1].getnewaddress(): 0.123,
            },
        )
        raw_hex = self.nodes[0].signrawtransactionwithwallet(raw_hex)["hex"]
        raw_hex = self.nodes[1].signrawtransactionwithwallet(raw_hex)["hex"]
        txid_join = self.nodes[0].sendrawtransaction(hexstring=raw_hex, maxfeerate=0)
        fee_join = self.nodes[0].getmempoolentry(txid_join)["fees"]["base"]
        # Fee should be correct: assert_equal(fee_join, self.nodes[0].gettransaction(txid_join)['fee'])
        # But it is not, see for example https://github.com/bitcoin/bitcoin/issues/14136:
        assert_not_equal(fee_join, self.nodes[0].gettransaction(txid_join)["fee"])

    def run_invalid_parameters_test(self):
        self.log.info("Test listtransactions RPC parameter validity")
        assert_raises_rpc_error(-8, 'Label argument must be a valid label name or "*".', self.nodes[0].listtransactions, label="")
        self.nodes[0].listtransactions(label="*")
        assert_raises_rpc_error(-8, "Negative count", self.nodes[0].listtransactions, count=-1)
        assert_raises_rpc_error(-8, "Negative from", self.nodes[0].listtransactions, skip=-1)

    def test_op_return(self):
        """Test if OP_RETURN outputs will be displayed correctly."""
        raw_tx = self.nodes[0].createrawtransaction([], [{'data': 'aa'}])
        funded_tx = self.nodes[0].fundrawtransaction(raw_tx)
        signed_tx = self.nodes[0].signrawtransactionwithwallet(funded_tx['hex'])
        tx_id = self.nodes[0].sendrawtransaction(signed_tx['hex'])

        op_ret_tx = [tx for tx in self.nodes[0].listtransactions() if tx['txid'] == tx_id][0]

        assert 'address' not in op_ret_tx

    def test_from_me_status_change(self):
        self.log.info("Test gettransaction after changing a transaction's 'from me' status")
        self.nodes[0].createwallet("fromme")
        default_wallet = self.nodes[0].get_wallet_rpc(self.default_wallet_name)
        wallet = self.nodes[0].get_wallet_rpc("fromme")

        # The 'fee' field of gettransaction is only added when the transaction is 'from me'
        # Run twice, once for a transaction in the mempool, again when it confirms
        for confirm in [False, True]:
            key = get_generate_key()
            descriptor = descsum_create(f"wpkh({key.privkey})")
            default_wallet.importdescriptors([{"desc": descriptor, "timestamp": "now"}])

            send_res = default_wallet.send(outputs=[{key.p2wpkh_addr: 1}, {wallet.getnewaddress(): 1}])
            assert_equal(send_res["complete"], True)
            vout = find_vout_for_address(self.nodes[0], send_res["txid"], key.p2wpkh_addr)
            utxos = [{"txid": send_res["txid"], "vout": vout}]
            self.generate(self.nodes[0], 1, sync_fun=self.no_op)

            # Send to the test wallet, ensuring that one input is for the descriptor we will import,
            # and that there are other inputs belonging to only the sending wallet
            send_res = default_wallet.send(outputs=[{wallet.getnewaddress(): 1.5}], inputs=utxos, add_inputs=True)
            assert_equal(send_res["complete"], True)
            txid = send_res["txid"]
            self.nodes[0].syncwithvalidationinterfacequeue()
            tx_info = wallet.gettransaction(txid)
            assert "fee" not in tx_info
            assert_equal(any(detail["category"] == "send" for detail in tx_info["details"]), False)

            if confirm:
                self.generate(self.nodes[0], 1, sync_fun=self.no_op)
                # Mock time forward and generate blocks so that the import does not rescan the transaction
                self.nodes[0].setmocktime(int(time.time()) + MAX_FUTURE_BLOCK_TIME + 1)
                self.generate(self.nodes[0], 10, sync_fun=self.no_op)

            import_res = wallet.importdescriptors([{"desc": descriptor, "timestamp": "now"}])
            assert_equal(import_res[0]["success"], True)
            # TODO: We should check that the fee matches, but since the transaction spends inputs
            # not known to the wallet, it is incorrectly calculating the fee.
            # assert_equal(wallet.gettransaction(txid)["fee"], fee)
            tx_info = wallet.gettransaction(txid)
            assert "fee" in tx_info
            assert_equal(any(detail["category"] == "send" for detail in tx_info["details"]), True)

    def check_tx_variants(self, wallet, txid, canonical_tx_hex, canonical_wtxid, alternate_wtxids):
        """Assert gettransaction and listtransactions report tx variants properly"""
        tx_info = wallet.gettransaction(txid)
        assert_equal(tx_info["hex"], canonical_tx_hex)
        assert_equal(tx_info["wtxid"], canonical_wtxid)
        # alternate_wtxids lists the other variants, never the canonical one
        assert canonical_wtxid not in tx_info["alternate_wtxids"]
        assert_equal(set(tx_info["alternate_wtxids"]), set(alternate_wtxids))

        # listtransactions exposes the same alternate_wtxids field as gettransaction
        list_entry = next(entry for entry in wallet.listtransactions() if entry["txid"] == txid)
        assert_equal(list_entry["alternate_wtxids"], tx_info["alternate_wtxids"])

    # Returns the finalized psbt transaction and its wtxid
    def finalize_tx_variant(self, wallet, psbt, spend_path):
        # First check the expected spend path is being used
        sig_field = {"script": "taproot_script_path_sigs", "key": "taproot_key_path_sig"}
        present, absent = sig_field[spend_path], sig_field["key" if spend_path == "script" else "script"]
        psbt_input = self.nodes[0].decodepsbt(psbt)["inputs"][0]
        assert present in psbt_input and absent not in psbt_input

        # Then finalize and decode
        tx = wallet.finalizepsbt(psbt)["hex"]
        return tx, wallet.decoderawtransaction(tx)["hash"]

    def test_alternate_witness_tx(self):
        self.log.info("Test gettransaction and listtransactions report alternate witnesses and canonical variant")
        self.nodes[0].createwallet("altwit")
        default_wallet = self.nodes[0].get_wallet_rpc(self.default_wallet_name)
        wallet = self.nodes[0].get_wallet_rpc("altwit")

        xprvs = [ExtendedPrivateKey.generate() for _ in range(0, 2)]
        xpubs = [xprv.pubkey() for xprv in xprvs]

        # Import a taproot descriptor with script paths
        desc = descsum_create(f"tr({xpubs[0].to_string()}/*,pk({xprvs[1].to_string()}/*))")
        assert_equal(wallet.importdescriptors([{"desc": desc, "active": True, "timestamp": "now"}])[0]["success"], True)
        default_wallet.sendtoaddress(wallet.getnewaddress(address_type="bech32m"), 1)
        self.generate(self.nodes[0], 1, sync_fun=self.no_op)
        # Isolate node0 for later reorg coverage
        self.disconnect_nodes(0, 1)
        self.disconnect_nodes(0, 2)

        # Create output psbt
        psbt = wallet.walletcreatefundedpsbt(outputs=[{default_wallet.getnewaddress(): 0.5}])["psbt"]

        # Create a script path spend and relay it. With only one variant known it
        # is trivially canonical and has no alternates
        self.log.info("Test the only known variant is canonical with no alternates")
        script_path_psbt = wallet.walletprocesspsbt(psbt=psbt, finalize=False)["psbt"]
        script_path_tx, script_path_wtxid = self.finalize_tx_variant(wallet, script_path_psbt, spend_path="script")
        txid = self.nodes[0].sendrawtransaction(script_path_tx)
        self.check_tx_variants(wallet, txid, script_path_tx, script_path_wtxid, alternate_wtxids=[])

        # Make a key path spend separate from the wallet
        key_path_desc = descsum_create(f"tr({xprvs[0].to_string()}/*,pk({xpubs[1].to_string()}/*))")
        key_path_psbt = self.nodes[0].descriptorprocesspsbt(psbt=psbt, descriptors=[{"desc": key_path_desc}], finalize=False)["psbt"]
        key_path_tx, key_path_wtxid = self.finalize_tx_variant(wallet, key_path_psbt, spend_path="key")

        # Ensure variants share the same txid but differ in wtxid, and the key path is the lighter of the two
        assert_equal(txid, self.nodes[0].decoderawtransaction(key_path_tx)["txid"])
        assert_not_equal(script_path_wtxid, key_path_wtxid)
        assert_greater_than(
            self.nodes[0].decoderawtransaction(script_path_tx)["weight"],
            self.nodes[0].decoderawtransaction(key_path_tx)["weight"],
        )

        # The wallet only learns the key path witness from a block (the mempool
        # holds one transaction per txid). Mine the key path: a confirmed variant
        # is canonical, with the script path now listed as its alternate.
        block = self.generateblock(self.nodes[0], default_wallet.getnewaddress(), [key_path_tx], sync_fun=self.no_op)["hash"]
        self.check_tx_variants(wallet, txid, key_path_tx, key_path_wtxid, alternate_wtxids=[script_path_wtxid])

        # Reorg that block out so both variants are known and unconfirmed. With no
        # confirmation to force the choice, the lighter key path is canonical.
        self.log.info("Test the lighter variant is canonical when both are known and unconfirmed")
        self.nodes[0].invalidateblock(block)
        self.nodes[0].syncwithvalidationinterfacequeue()
        assert_equal(wallet.gettransaction(txid)["confirmations"], 0)
        self.check_tx_variants(wallet, txid, key_path_tx, key_path_wtxid, alternate_wtxids=[script_path_wtxid])

        # The canonical choice and alternates survive a wallet reload
        wallet.unloadwallet()
        self.nodes[0].loadwallet("altwit")
        self.check_tx_variants(wallet, txid, key_path_tx, key_path_wtxid, alternate_wtxids=[script_path_wtxid])

        # Now confirm the heavier script path instead, on a longer competing chain
        # from node1, and reconnect so node0 reorgs onto it. The confirmed variant
        # is canonical even though it is the heavier one.
        self.log.info("Test a confirmed variant is canonical even when it is the heavier one")
        self.generate(self.nodes[1], 3, sync_fun=self.no_op)
        block = self.generateblock(self.nodes[1], default_wallet.getnewaddress(), [script_path_tx], sync_fun=self.no_op)["hash"]
        self.connect_nodes(0, 1)
        self.connect_nodes(0, 2)
        self.sync_all()
        self.check_tx_variants(wallet, txid, script_path_tx, script_path_wtxid, alternate_wtxids=[key_path_wtxid])

        # The confirmed-canonical choice survive a reload
        wallet.unloadwallet()
        self.nodes[0].loadwallet("altwit")
        self.check_tx_variants(wallet, txid, script_path_tx, script_path_wtxid, alternate_wtxids=[key_path_wtxid])

        self.log.info("Test canonical reverts to the lighter variant when the confirmed one is reorged out")
        # Both variants are unconfirmed again, so the lighter key path is canonical once more
        self.disconnect_nodes(0, 1)
        self.disconnect_nodes(0, 2)
        self.nodes[0].invalidateblock(block)
        self.nodes[0].syncwithvalidationinterfacequeue()
        assert_equal(wallet.gettransaction(txid)["confirmations"], 0)
        self.check_tx_variants(wallet, txid, key_path_tx, key_path_wtxid, alternate_wtxids=[script_path_wtxid])


if __name__ == '__main__':
    ListTransactionsTest(__file__).main()
