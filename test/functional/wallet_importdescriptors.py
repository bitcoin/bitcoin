#!/usr/bin/env python3
# Copyright (c) 2019-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the importdescriptors RPC.

Test importdescriptors by generating keys on node0, importing the corresponding
descriptors on node1 and then testing the address info for the different address
variants.

- `get_generate_key()` is called to generate keys and return the privkeys,
  pubkeys and all variants of scriptPubKey and address.
- `test_importdesc()` is called to send an importdescriptors call to node1, test
  success, and (if unsuccessful) test the error code and error message returned.
- `test_address()` is called to call getaddressinfo for an address on node1
  and test the values returned."""

import concurrent.futures
import threading
import time

from test_framework.address import key_to_p2sh_p2wpkh, key_to_p2wpkh, script_to_p2wsh
from test_framework.blocktools import COINBASE_MATURITY
from test_framework.test_framework import BitcoinTestFramework
from test_framework.descriptors import descsum_create
from test_framework.extendedkey import ExtendedPrivateKey
from test_framework.script import SEQUENCE_LOCKTIME_TYPE_FLAG
from test_framework.script_util import keys_to_multisig_script
from test_framework.util import (
    assert_equal,
    assert_raises_rpc_error,
    JSONRPCException,
)
from test_framework.wallet_util import (
    get_generate_key,
    test_address,
)

BLOCK_TIME = 60 * 10
MISSING_KEYS_WARNING = "Not all private keys provided. Some wallet functionality may return unexpected errors"

class ImportDescriptorsTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 2
        # whitelist peers to speed up tx relay / mempool sync
        self.noban_tx_relay = True
        self.extra_args = [["-addresstype=legacy"],
                           ["-addresstype=bech32", "-keypool=5"]
                          ]
        self.setup_clean_chain = True
        self.wallet_names = []

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def test_importdesc(self, req, success, global_error=False, error_code=None, error_message=None, warnings=None, wallet=None):
        """Run importdescriptors and assert success"""
        if warnings is None:
            warnings = []
        wrpc = self.nodes[1].get_wallet_rpc('w1')
        if wallet is not None:
            wrpc = wallet

        if global_error and not success:
            try:
                result = wrpc.importdescriptors([req])
            except JSONRPCException as e:
                assert_equal(e.error["code"], error_code)
                assert_equal(e.error["message"], error_message)
                return

        result = wrpc.importdescriptors([req])
        observed_warnings = []
        if 'warnings' in result[0]:
            observed_warnings = result[0]['warnings']
        assert_equal("\n".join(sorted(warnings)), "\n".join(sorted(observed_warnings)))
        self.log.debug(result)
        assert_equal(result[0]['success'], success)
        if error_code is not None:
            assert_equal(result[0]['error']['code'], error_code)
            assert_equal(result[0]['error']['message'], error_message)

    def test_verify_balance(self, node, miner_wallet):
        self.node_time = max(int(time.time()), node.getblockheader(node.getbestblockhash())["time"])
        self.advance_time(node, BLOCK_TIME)
        receive_address = miner_wallet.getnewaddress()
        desc = miner_wallet.getaddressinfo(receive_address)["desc"]

        self.log.info("Send two payments to the address being imported")
        txid_first = miner_wallet.sendtoaddress(receive_address, 2)
        first_block = self.generate(node, 1, sync_fun=self.no_op)[0]
        first_header = node.getblockheader(first_block)
        first_utxo = miner_wallet.listunspent(addresses=[receive_address])[0]
        first_outpoint = {"txid": txid_first, "vout": first_utxo["vout"]}
        miner_wallet.lockunspent(False, [first_outpoint])
        self.advance_time(node, BLOCK_TIME)

        txid_second = miner_wallet.sendtoaddress(receive_address, 2)
        self.generate(node, 1, sync_fun=self.no_op)

        self.log.info("Move both payments outside the two-hour timestamp window")
        for _ in range(20):
            self.advance_time(node, BLOCK_TIME)
            self.generate(node, 1, sync_fun=self.no_op)
        tip = node.getblockheader(node.getbestblockhash())

        node.createwallet(wallet_name="verify_disabled", disable_private_keys=True)
        node.createwallet(wallet_name="verify_enabled", disable_private_keys=True)
        wallet_no_verify = node.get_wallet_rpc("verify_disabled")
        wallet_verify = node.get_wallet_rpc("verify_enabled")

        self.log.info("Import with timestamp=now and verification disabled")
        assert_equal(wallet_no_verify.importdescriptors([{"desc": desc, "timestamp": "now"}]), [{"success": True}])
        assert_equal(wallet_no_verify.getbalance(), 0)
        assert_equal(wallet_no_verify.listtransactions(), [])

        future_timestamp = tip["time"] + 3 * 60 * 60
        assert_equal(wallet_no_verify.importdescriptors([{"desc": desc, "timestamp": future_timestamp}], False), [{"success": True}])
        assert_equal(wallet_no_verify.getbalance(), 0)
        assert_equal(wallet_no_verify.listtransactions(), [])

        self.log.info("Enable verification and recover both payments")
        result = wallet_verify.importdescriptors([{"desc": desc, "timestamp": "now"}], True)[0]
        assert_equal(result["success"], True)
        assert_equal(result["info"], {
            "status": "matched after recovery",
            "utxo_check": True,
            "wallet_utxos": 2,
            "chain_utxos": 2,
            "snapshot_block": tip["hash"],
            "snapshot_height": tip["height"],
            "scan_start_height": first_header["height"],
            "recovery_start_height": first_header["height"],
            "scanned_blocks": tip["height"] - first_header["height"] + 1,
        })
        assert_equal(wallet_verify.getbalance(), 4)
        assert_equal({utxo["txid"] for utxo in wallet_verify.listunspent()}, {txid_first, txid_second})
        assert_equal(len(wallet_verify.listtransactions()), 2)

        self.log.info("Import with the first payment's timestamp, with and without verification")
        node.createwallet(wallet_name="verify_timestamp", disable_private_keys=True)
        node.createwallet(wallet_name="verify_timestamp_enabled", disable_private_keys=True)
        wallet_timestamp = node.get_wallet_rpc("verify_timestamp")
        wallet_timestamp_verify = node.get_wallet_rpc("verify_timestamp_enabled")
        request = [{"desc": desc, "timestamp": first_header["time"]}]
        assert_equal(wallet_timestamp.importdescriptors(request), [{"success": True}])
        result = wallet_timestamp_verify.importdescriptors(request, True)[0]
        assert_equal(result["success"], True)
        assert_equal(result["info"]["utxo_check"], True)
        assert_equal(result["info"]["status"], "matched")
        assert "recovery_start_height" not in result["info"]
        assert result["info"]["scan_start_height"] <= first_header["height"]
        for wallet in [wallet_timestamp, wallet_timestamp_verify]:
            assert_equal(wallet.getbalance(), 4)
            assert_equal({utxo["txid"] for utxo in wallet.listunspent()}, {txid_first, txid_second})
            assert_equal(len(wallet.listtransactions()), 2)
            wallet.unloadwallet()

        wallet_no_verify.unloadwallet()
        wallet_verify.unloadwallet()
        miner_wallet.lockunspent(True, [first_outpoint])

    def test_verify_balance_unmatched(self, node, miner_wallet):
        self.log.info("Create an old payment, spend it, then receive another payment")
        receive_address = miner_wallet.getnewaddress()
        desc = miner_wallet.getaddressinfo(receive_address)["desc"]
        self.advance_time(node, BLOCK_TIME)
        txid_old = miner_wallet.sendtoaddress(receive_address, 2)
        old_block = self.generate(node, 1, sync_fun=self.no_op)[0]
        old_header = node.getblockheader(old_block)
        old_tx = miner_wallet.gettransaction(txid_old)["hex"]
        old_proof = node.gettxoutproof([txid_old], old_block)
        old_utxo = miner_wallet.listunspent(addresses=[receive_address])[0]

        txid_spend = miner_wallet.sendall(
            recipients=[miner_wallet.getnewaddress()],
            inputs=[{"txid": txid_old, "vout": old_utxo["vout"]}],
        )["txid"]
        self.advance_time(node, BLOCK_TIME)
        self.generate(node, 1, sync_fun=self.no_op)
        txid_unspent = miner_wallet.sendtoaddress(receive_address, 2)
        self.advance_time(node, BLOCK_TIME)
        unspent_block = self.generate(node, 1, sync_fun=self.no_op)[0]
        unspent_height = node.getblockheader(unspent_block)["height"]
        for _ in range(20):
            self.advance_time(node, BLOCK_TIME)
            self.generate(node, 1, sync_fun=self.no_op)

        self.log.info("Recover the unspent payment without recovering the older spent history")
        node.createwallet(wallet_name="verify_history", disable_private_keys=True)
        wallet_history = node.get_wallet_rpc("verify_history")
        result = wallet_history.importdescriptors([{"desc": desc, "timestamp": "now"}], True)[0]
        assert_equal(result["success"], True)
        assert_equal(result["info"]["status"], "matched after recovery")
        assert_equal(result["info"]["recovery_start_height"], unspent_height)
        assert_equal(wallet_history.getbalance(), 2)
        assert_equal(len(wallet_history.listtransactions()), 1)
        wallet_history.gettransaction(txid_unspent)
        assert_raises_rpc_error(-5, "Invalid or non-wallet transaction id", wallet_history.gettransaction, txid_old)
        assert_raises_rpc_error(-5, "Invalid or non-wallet transaction id", wallet_history.gettransaction, txid_spend)

        self.log.info("Import the old payment without its spend to create a wallet mismatch")
        node.createwallet(wallet_name="verify_unmatched", disable_private_keys=True)
        wallet_unmatched = node.get_wallet_rpc("verify_unmatched")
        assert_equal(wallet_unmatched.importdescriptors([{"desc": desc, "timestamp": "now"}]), [{"success": True}])
        wallet_unmatched.importprunedfunds(old_tx, old_proof)
        result = wallet_unmatched.importdescriptors([{"desc": desc, "timestamp": "now"}], True)[0]
        assert_equal(result["success"], False)
        assert_equal(result["error"]["code"], -4)
        assert "UTXO verification did not succeed (unmatched)" in result["error"]["message"]
        assert "different UTXO outpoints (wallet: 2, chainstate: 1)" in result["error"]["message"]
        assert "Changes to relevant UTXOs during verification can cause this even if the wallet is up to date." in result["error"]["message"]
        assert_equal(result["info"]["status"], "unmatched")
        assert_equal(result["info"]["utxo_check"], False)
        assert_equal(result["info"]["wallet_utxos"], 2)
        assert_equal(result["info"]["chain_utxos"], 1)
        assert_equal(result["info"]["scan_start_height"], unspent_height)
        assert_equal(result["info"]["recovery_start_height"], unspent_height)
        assert_equal(wallet_unmatched.getbalance(), 4)
        assert_raises_rpc_error(-5, "Invalid or non-wallet transaction id", wallet_unmatched.gettransaction, txid_spend)

        self.log.info("Rescan from the old payment and verify that the mismatch is gone")
        wallet_unmatched.rescanblockchain(old_header["height"])
        result = wallet_unmatched.importdescriptors([{"desc": desc, "timestamp": "now"}], True)[0]
        assert_equal(result["success"], True)
        assert_equal(result["info"]["status"], "matched")
        assert_equal(result["info"]["utxo_check"], True)
        assert "recovery_start_height" not in result["info"]
        assert_equal(wallet_unmatched.getbalance(), 2)

        self.log.info("An earlier import timestamp still recovers history when the UTXOs are already known")
        result = wallet_history.importdescriptors([{"desc": desc, "timestamp": 0}], True)[0]
        assert_equal(result["success"], True)
        assert_equal(result["info"]["status"], "matched")
        assert_equal(result["info"]["utxo_check"], True)
        assert_equal(result["info"]["scan_start_height"], 0)
        assert_equal(result["info"]["scanned_blocks"], node.getblockcount() + 1)
        assert "recovery_start_height" not in result["info"]
        for txid in [txid_old, txid_spend, txid_unspent]:
            wallet_history.gettransaction(txid)
        wallet_history.unloadwallet()
        wallet_unmatched.unloadwallet()

    def test_verify_balance_mempool(self, node, miner_wallet):
        self.log.info("Discover a mempool payment even when the chainstate scan finds no UTXOs")
        receive_address = miner_wallet.getnewaddress()
        desc = miner_wallet.getaddressinfo(receive_address)["desc"]
        txid = miner_wallet.sendtoaddress(receive_address, 2)
        node.createwallet(wallet_name="verify_pending", disable_private_keys=True)
        wallet_pending = node.get_wallet_rpc("verify_pending")
        future_timestamp = node.getblockheader(node.getbestblockhash())["time"] + 3 * 60 * 60
        result = wallet_pending.importdescriptors([{"desc": desc, "timestamp": future_timestamp}], True)[0]
        assert_equal(result["success"], True)
        assert_equal(result["info"]["status"], "matched")
        assert_equal(result["info"]["utxo_check"], True)
        assert_equal(result["info"]["wallet_utxos"], 0)
        assert_equal(result["info"]["chain_utxos"], 0)
        assert_equal(wallet_pending.gettransaction(txid)["confirmations"], 0)
        assert_equal(wallet_pending.getbalances()["mine"]["untrusted_pending"], 2)

        self.log.info("Confirm the payment, then spend it without confirming the spend")
        self.advance_time(node, BLOCK_TIME)
        self.generate(node, 1, sync_fun=self.no_op)
        utxo = miner_wallet.listunspent(addresses=[receive_address])[0]
        txid_spend = miner_wallet.sendall(
            recipients=[miner_wallet.getnewaddress()],
            inputs=[{"txid": txid, "vout": utxo["vout"]}],
        )["txid"]

        self.log.info("The mempool spend should not remove the confirmed UTXO from the comparison")
        node.createwallet(wallet_name="verify_pending_spend", disable_private_keys=True)
        wallet_spend = node.get_wallet_rpc("verify_pending_spend")
        result = wallet_spend.importdescriptors([{"desc": desc, "timestamp": 0}], True)[0]
        assert_equal(result["success"], True)
        assert_equal(result["info"]["utxo_check"], True)
        assert_equal(result["info"]["wallet_utxos"], 1)
        assert_equal(result["info"]["chain_utxos"], 1)
        assert_equal(wallet_spend.gettransaction(txid_spend)["confirmations"], 0)
        wallet_pending.unloadwallet()
        wallet_spend.unloadwallet()

    def test_verify_balance_pruned(self, node, miner_wallet):
        self.log.info("Create a payment whose block will be pruned before import")
        receive_address = miner_wallet.getnewaddress()
        desc = miner_wallet.getaddressinfo(receive_address)["desc"]
        txid = miner_wallet.sendtoaddress(receive_address, 2)
        self.advance_time(node, BLOCK_TIME)
        funding_block = self.generate(node, 1, sync_fun=self.no_op)[0]
        funding_height = node.getblockheader(funding_block)["height"]

        self.log.info("Restart in pruning mode and prune the payment's block")
        self.restart_node(0, extra_args=self.extra_args[0] + ["-prune=1", "-fastprune", f"-mocktime={self.node_time}"])
        # Pruning keeps at least 288 recent blocks.
        self.advance_time(node, 350 * BLOCK_TIME)
        self.generate(node, 350, sync_fun=self.no_op)
        assert node.pruneblockchain(funding_height) >= funding_height
        assert_raises_rpc_error(-1, "Block not available (pruned data)", node.getblock, funding_block)
        tip = node.getblockheader(node.getbestblockhash())

        self.log.info("Verification should report unavailable blocks and keep the descriptor imported")
        node.createwallet(wallet_name="verify_pruned", disable_private_keys=True)
        wallet = node.get_wallet_rpc("verify_pruned")
        with node.assert_debug_log([], unexpected_msgs=["[verify_pruned] Rescan started"]):
            result = wallet.importdescriptors([{"desc": desc, "timestamp": "now"}], True)[0]
        assert_equal(result["success"], False)
        assert_equal(result["error"]["code"], -4)
        assert "UTXO verification did not succeed (blocks unavailable)" in result["error"]["message"]
        assert_equal(result["info"], {
            "status": "blocks unavailable",
            "wallet_utxos": 0,
            "chain_utxos": 1,
            "snapshot_block": tip["hash"],
            "snapshot_height": tip["height"],
            "scan_start_height": funding_height,
            "recovery_start_height": funding_height,
            "scanned_blocks": 0,
        })
        assert_equal(wallet.getaddressinfo(receive_address)["ismine"], True)
        assert_equal(wallet.getbalance(), 0)
        assert_equal(wallet.listtransactions(), [])
        assert_raises_rpc_error(-5, "Invalid or non-wallet transaction id", wallet.gettransaction, txid)
        wallet.unloadwallet()

    def advance_time(self, node, seconds):
        self.node_time += seconds
        node.setmocktime(self.node_time)

    def test_import_unused_key(self):
        self.log.info("Test import of unused(KEY)")
        self.nodes[0].createwallet(wallet_name="import_unused", blank=True)
        wallet = self.nodes[0].get_wallet_rpc("import_unused")

        assert_equal(len(wallet.gethdkeys()), 0)

        extended_key = ExtendedPrivateKey.generate()
        xprv = extended_key.to_string()
        xpub = extended_key.pubkey().to_string()
        self.test_importdesc({"desc":descsum_create(f"unused({xpub})"),
                              "timestamp": "now"},
                              success=False,
                              error_code=-4,
                              error_message='Cannot import descriptor without private keys to a wallet with private keys enabled',
                              wallet=wallet)
        self.test_importdesc({"timestamp": "now", "desc": descsum_create(f"unused({xprv})")},
                             success=True,
                             wallet=wallet)
        hdkeys = wallet.gethdkeys()
        assert_equal(len(hdkeys), 1)
        assert_equal(hdkeys[0]["xpub"], xpub)
        wallet.unloadwallet()

    def test_import_unused_key_existing(self):
        self.log.info("Test import of unused(KEY) with existing KEY")
        self.nodes[0].createwallet(wallet_name="import_existing_unused")
        wallet = self.nodes[0].get_wallet_rpc("import_existing_unused")

        hdkeys = wallet.gethdkeys(private=True)
        assert_equal(len(hdkeys), 1)
        xprv = hdkeys[0]["xprv"]

        self.test_importdesc({"timestamp": "now", "desc": descsum_create(f"unused({xprv})")},
                             success=False,
                             error_code=-4,
                             error_message="Cannot import an unused() descriptor when its private key is already in the wallet",
                             wallet=wallet)
        wallet.unloadwallet()

    def test_import_unused_noprivs(self):
        self.log.info("Test import of unused(KEY) to wallet without privkeys")
        self.nodes[0].createwallet(wallet_name="import_unused_noprivs", disable_private_keys=True)
        wallet = self.nodes[0].get_wallet_rpc("import_unused_noprivs")

        xpub = ExtendedPrivateKey.generate().pubkey().to_string()
        self.test_importdesc({"timestamp": "now", "desc": descsum_create(f"unused({xpub})")},
                             success=False,
                             error_code=-4,
                             error_message="Cannot import unused() to wallet without private keys enabled",
                             wallet=wallet)
        wallet.unloadwallet()

    def test_per_item_errors_are_reported_in_order(self):
        self.log.info("Test that import results are in the same order as the original request")
        self.nodes[0].createwallet(wallet_name="test_order_import", blank=True)
        wallet = self.nodes[0].get_wallet_rpc('test_order_import')
        whitespace_pubkey = f" {get_generate_key().pubkey}"
        cases = [
            ({
                "timestamp": "now"
            }, [False, "Descriptor not found."]),
            ({
                "desc": descsum_create(f"pkh({get_generate_key().privkey})"),
                "timestamp": 1,
                "label": "Valid descriptor 1",
            }, [True]),
            ({
                "desc": descsum_create(f"pkh({get_generate_key().privkey})"),
                "timestamp": "now",
                "internal": True,
            }, [True]),
            ({
                "desc": descsum_create(f"pkh({get_generate_key().pubkey})"),
                "timestamp": "now",
                "label": "Invalid descriptor 2",
                "internal": True,
            }, [False, "Internal addresses should not have a label"]),
            ({
                "desc": descsum_create(f"pkh({whitespace_pubkey})"),
                "timestamp": "now",
                "internal": True,
            }, [False, f"pkh(): Key '{whitespace_pubkey}' is invalid due to whitespace"]),
        ]

        descriptors, expected = map(list, zip(*cases))
        results = wallet.importdescriptors(descriptors)
        for i, result in enumerate(results):
            assert_equal(result["success"], expected[i][0])
            if not result["success"]:
                assert_equal(result["error"]["message"], expected[i][1])

    def test_rescan_fails_import(self):
        xpriv = ExtendedPrivateKey.generate().to_string()

        self.log.info("Test importdescriptors fails when wallet is already rescanning")
        wallet_name = "rescan_wallet"
        self.nodes[0].createwallet(wallet_name=wallet_name, blank=True)
        other_desc = descsum_create("pkh(" + get_generate_key().privkey + ")")

        w_import = self.nodes[0].create_new_rpc_connection(mode="AUTHPROXY") / f"wallet/{wallet_name}"

        with concurrent.futures.ThreadPoolExecutor(max_workers=2) as thread:
            w_rescan = self.nodes[0].create_new_rpc_connection(mode="AUTHPROXY") / f"wallet/{wallet_name}"
            w_conflict = self.nodes[0].create_new_rpc_connection(mode="AUTHPROXY") / f"wallet/{wallet_name}"
            # Use an xprv with timestamp=0 and a large key-range to trigger a slow full rescan that stays in-flight
            slow_desc = [{"desc": descsum_create("pkh(" + xpriv + "/0h/*h)"),
            "timestamp": 0, "range": [0, 10000]}]
            conflicting_desc = [{"desc": descsum_create("pkh(" + xpriv + "/1h/*h)"),
            "timestamp": 0, "range": [0, 10000]}]
            num_relevant_blocks = 1000
            self.generatetoaddress(self.nodes[0], num_relevant_blocks, self.nodes[0].deriveaddresses(slow_desc[0]['desc'], [0, 0])[0])
            self.generatetoaddress(self.nodes[0], num_relevant_blocks, self.nodes[0].deriveaddresses(conflicting_desc[0]['desc'], [0, 0])[0])

            start = threading.Barrier(3)

            def import_after_barrier(wallet, descriptors):
                start.wait(timeout=10)
                return wallet.importdescriptors(descriptors)

            imports = [
                thread.submit(import_after_barrier, w_rescan, slow_desc),
                thread.submit(import_after_barrier, w_conflict, conflicting_desc),
            ]
            start.wait(timeout=10)

            # One importdescriptor call must hold WalletRescanReserver while the other fails immediately.
            num_errors = 0
            num_success = 0
            for future in concurrent.futures.as_completed(imports, timeout=30 * self.options.timeout_factor):
                try:
                    assert_equal(future.result(), [{'success': True}])
                    num_success += 1
                except JSONRPCException as e:
                    assert_equal(e.error["code"], -4)
                    assert_equal(e.error["message"], "Wallet is currently rescanning. Abort existing rescan or wait.")
                    num_errors += 1

            assert_equal(num_success, 1)
            assert_equal(num_errors, 1)

        # After the rescan finishes, any importdescriptors should succeed.
        result = w_import.importdescriptors([{"desc": other_desc, "timestamp": "now"}])
        assert_equal(result[0]['success'], True)

        self.log.info("Aborting an importdescriptors rescan should fail the RPC call")
        wallet_name = "abort_import_wallet"
        self.nodes[0].createwallet(wallet_name, blank=True)

        with concurrent.futures.ThreadPoolExecutor(max_workers=1) as thread:
            w_import = self.nodes[0].create_new_rpc_connection(mode="AUTHPROXY") / f"wallet/{wallet_name}"
            abort_rpc = self.nodes[0].create_new_rpc_connection(mode="AUTHPROXY") / f"wallet/{wallet_name}"
            descriptor = [{"desc": descsum_create("pkh(" + xpriv + "/2h/*h)"),
            "timestamp": 0, "range": [0, 4000]}]

            importing = thread.submit(w_import.importdescriptors, descriptor)

            # Keep trying because an abort before wallet transaction scan starts
            # is reset when the scan loop begins.
            abort_succeeded = False
            abort_deadline = time.time() + 30 * self.options.timeout_factor
            while not importing.done() and time.time() < abort_deadline:
                abort_succeeded = abort_rpc.abortrescan() or abort_succeeded

            assert_equal(abort_succeeded, True)
            try:
                importing.result(timeout=30 * self.options.timeout_factor)
                raise AssertionError("importdescriptors unexpectedly succeeded")
            except JSONRPCException as e:
                assert_equal(e.error["code"], -1)
                assert_equal(e.error["message"], "Rescan aborted by user.")

    def test_musig_private_key_warnings(self, xprv1, acc_xprv2, acc_xpub2, derivation_path):
        self.log.info("Testing importdescriptors MuSig private key warnings")

        self.nodes[1].createwallet(wallet_name="musig_import_warnings", blank=True)
        wallet = self.nodes[1].get_wallet_rpc("musig_import_warnings")
        res = wallet.importdescriptors([
            {
                "desc": descsum_create(f"rawtr(musig({xprv1}/{derivation_path}/0,{acc_xprv2}/1))"),
                "timestamp": "now",
            },
            {
                "desc": descsum_create(f"rawtr(musig({xprv1}/{derivation_path}/2,{acc_xpub2}/3))"),
                "timestamp": "now",
            },
            {
                "desc": descsum_create(f"rawtr(musig({xprv1}/{derivation_path},{acc_xprv2})/0/*)"),
                "timestamp": "now",
                "range": [0, 1],
            },
            {
                "desc": descsum_create(f"rawtr(musig({xprv1}/{derivation_path},{acc_xpub2})/1/*)"),
                "timestamp": "now",
                "range": [0, 1],
            },
        ])

        assert_equal(res[0]["success"], True)
        assert "warnings" not in res[0]
        assert_equal(res[1]["success"], True)
        assert_equal(res[1]["warnings"], [MISSING_KEYS_WARNING])
        assert_equal(res[2]["success"], True)
        assert "warnings" not in res[2]
        assert_equal(res[3]["success"], True)
        assert_equal(res[3]["warnings"], [MISSING_KEYS_WARNING])

    def run_test(self):
        self.log.info('Setting up wallets')
        self.nodes[0].createwallet(wallet_name='w0', disable_private_keys=False)
        w0 = self.nodes[0].get_wallet_rpc('w0')

        self.nodes[1].createwallet(wallet_name='w1', disable_private_keys=True, blank=True)
        w1 = self.nodes[1].get_wallet_rpc('w1')
        assert_equal(w1.getwalletinfo()['keypoolsize'], 0)

        self.nodes[1].createwallet(wallet_name="wpriv", disable_private_keys=False, blank=True)
        wpriv = self.nodes[1].get_wallet_rpc("wpriv")
        assert_equal(wpriv.getwalletinfo()['keypoolsize'], 0)

        self.log.info('Mining coins')
        self.generatetoaddress(self.nodes[0], COINBASE_MATURITY + 1, w0.getnewaddress())

        # RPC importdescriptors -----------------------------------------------

        # # Test import fails if no descriptor present
        self.log.info("Import should fail if a descriptor is not provided")
        self.test_importdesc({"timestamp": "now"},
                             success=False,
                             error_code=-8,
                             error_message='Descriptor not found.')

        # Test import fails if one timestamp is invalid or missing
        self.log.info("Import should fail if timestamp is missing or an invalid timestamp is present in the request")
        key = get_generate_key()
        import_request = {"desc": descsum_create("pkh(" + key.pubkey + ")"), "label": "Descriptor import test"}
        self.test_importdesc(import_request,
            success=False,
            global_error=True,
            error_code=-3,
            error_message="Missing required timestamp field for key")

        import_request = {"desc": descsum_create("pkh(" + key.pubkey + ")"),
            "timestamp": "this_is_not_a_valid_timestamp",
            "label": "Descriptor import test"}
        self.test_importdesc(import_request,
            success=False,
            global_error=True,
            error_code=-3,
            error_message='Expected number or "now" timestamp value for key. got type string')

        # # Test importing of a P2PKH descriptor
        key = get_generate_key()
        self.log.info("Should import a p2pkh descriptor")
        import_request = {"desc": descsum_create("pkh(" + key.pubkey + ")"),
                 "timestamp": "now",
                 "label": "Descriptor import test"}
        self.test_importdesc(import_request, success=True)
        test_address(w1,
                     key.p2pkh_addr,
                     solvable=True,
                     ismine=True,
                     labels=["Descriptor import test"])
        assert_equal(w1.getwalletinfo()['keypoolsize'], 0)

        self.log.info("Test can import same descriptor with public key twice")
        list_descs = w1.listdescriptors()
        self.test_importdesc(import_request, success=True)
        assert_equal(list_descs, w1.listdescriptors())

        self.log.info("Test can update descriptor label")
        self.test_importdesc({**import_request, "label": "Updated label"}, success=True)
        test_address(w1, key.p2pkh_addr, solvable=True, ismine=True, labels=["Updated label"])
        assert_equal(list_descs, w1.listdescriptors())

        self.log.info("Internal addresses cannot have labels")
        self.test_importdesc({**import_request, "internal": True},
                             success=False,
                             error_code=-8,
                             error_message="Internal addresses should not have a label")

        self.log.info("External non-ranged addresses can have labels")
        self.test_importdesc({**import_request, "internal": False}, success=True)

        self.log.info("Internal addresses should be detected as such")
        key = get_generate_key()
        self.test_importdesc({"desc": descsum_create("pkh(" + key.pubkey + ")"),
                              "timestamp": "now",
                              "internal": True},
                             success=True)
        info = w1.getaddressinfo(key.p2pkh_addr)
        assert_equal(info["ismine"], True)
        assert_equal(info["ischange"], True)

        self.log.info("Should not import a descriptor with an invalid public key due to whitespace")
        self.test_importdesc({"desc": descsum_create("pkh( " + key.pubkey + ")"),
                                    "timestamp": "now",
                                    "internal": True},
                                    error_code=-5,
                                    error_message=f"pkh(): Key ' {key.pubkey}' is invalid due to whitespace",
                                    success=False)
        self.test_importdesc({"desc": descsum_create("pkh(" + key.pubkey + " )"),
                                    "timestamp": "now",
                                    "internal": True},
                                    error_code=-5,
                                    error_message=f"pkh(): Key '{key.pubkey} ' is invalid due to whitespace",
                                    success=False)

        # # Test importing of a P2SH-P2WPKH descriptor
        key = get_generate_key()
        self.log.info("Should not import a p2sh-p2wpkh descriptor without checksum")
        self.test_importdesc({"desc": "sh(wpkh(" + key.pubkey + "))",
                              "timestamp": "now"
                              },
                             success=False,
                             error_code=-5,
                             error_message="Missing checksum")

        self.log.info("Should not import a p2sh-p2wpkh descriptor that has range specified")
        self.test_importdesc({"desc": descsum_create("sh(wpkh(" + key.pubkey + "))"),
                               "timestamp": "now",
                               "range": 1,
                              },
                              success=False,
                              error_code=-8,
                              error_message="Range should not be specified for an un-ranged descriptor")

        self.log.info("Should not import a p2sh-p2wpkh descriptor and have it set to active")
        self.test_importdesc({"desc": descsum_create("sh(wpkh(" + key.pubkey + "))"),
                               "timestamp": "now",
                               "active": True,
                              },
                              success=False,
                              error_code=-8,
                              error_message="Active descriptors must be ranged")

        self.log.info("Should import a (non-active) p2sh-p2wpkh descriptor")
        self.test_importdesc({"desc": descsum_create("sh(wpkh(" + key.pubkey + "))"),
                               "timestamp": "now",
                               "active": False,
                              },
                              success=True)
        assert_equal(w1.getwalletinfo()['keypoolsize'], 0)

        test_address(w1,
                     key.p2sh_p2wpkh_addr,
                     ismine=True,
                     solvable=True)

        # Check persistence of data and that loading works correctly
        w1.unloadwallet()
        self.nodes[1].loadwallet('w1')
        test_address(w1,
                     key.p2sh_p2wpkh_addr,
                     ismine=True,
                     solvable=True)

        # # Test importing of a multisig descriptor
        key1 = get_generate_key()
        key2 = get_generate_key()
        self.log.info("Should import a 1-of-2 bare multisig from descriptor")
        self.test_importdesc({"desc": descsum_create("multi(1," + key1.pubkey + "," + key2.pubkey + ")"),
                              "timestamp": "now"},
                             success=True)
        self.log.info("Should not treat individual keys from the imported bare multisig as watchonly")
        test_address(w1,
                     key1.p2pkh_addr,
                     ismine=False)

        # # Test ranged descriptors
        extended_key = ExtendedPrivateKey.generate()
        xpriv = extended_key.to_string()
        xpub = extended_key.pubkey().to_string()
        pubkeys = [extended_key.derive_path(f"m/0'/0'/{i}'").pubkey().pubkey.get_bytes() for i in range(2)]
        addresses = [key_to_p2sh_p2wpkh(pubkey) for pubkey in pubkeys] # hdkeypath=m/0'/0'/0' and 1'
        addresses += [key_to_p2wpkh(pubkey) for pubkey in pubkeys] # wpkh subscripts corresponding to the above addresses
        desc = "sh(wpkh(" + xpub + "/0/0/*" + "))"

        self.log.info("Ranged descriptors cannot have labels")
        self.test_importdesc({"desc":descsum_create(desc),
                              "timestamp": "now",
                              "range": [0, 100],
                              "label": "test"},
                              success=False,
                              error_code=-8,
                              error_message='Ranged descriptors should not have a label')

        self.log.info("Ranged descriptors cannot have labels - even if range not provided by user and only implied by asterisk (*)")
        self.test_importdesc({"desc":descsum_create("wpkh(" + xpub + "/100/0/*)"),
                              "timestamp": "now",
                              "label": "test",
                              "active": True},
                              success=False,
                              warnings=['Range not given, using default keypool range'],
                              error_code=-8,
                              error_message='Ranged descriptors should not have a label')

        self.log.info("Private keys required for private keys enabled wallet")
        self.test_importdesc({"desc":descsum_create(desc),
                              "timestamp": "now",
                              "range": [0, 100]},
                              success=False,
                              error_code=-4,
                              error_message='Cannot import descriptor without private keys to a wallet with private keys enabled',
                              wallet=wpriv)

        self.log.info("Ranged descriptor import should warn without a specified range")
        self.test_importdesc({"desc": descsum_create(desc),
                               "timestamp": "now"},
                              success=True,
                              warnings=['Range not given, using default keypool range'])
        assert_equal(w1.getwalletinfo()['keypoolsize'], 0)

        # # Test importing of a ranged descriptor with xpriv
        self.log.info("Should not import a ranged descriptor that includes xpriv into a watch-only wallet")
        desc = "sh(wpkh(" + xpriv + "/0'/0'/*'" + "))"
        self.test_importdesc({"desc": descsum_create(desc),
                              "timestamp": "now",
                              "range": 1},
                             success=False,
                             error_code=-4,
                             error_message='Cannot import private keys to a wallet with private keys disabled')

        self.log.info("Should not import a descriptor with hardened derivations when private keys are disabled")
        self.test_importdesc({"desc": descsum_create("wpkh(" + xpub + "/1h/*)"),
                              "timestamp": "now",
                              "range": 1},
                             success=False,
                             error_code=-4,
                             error_message='Cannot expand descriptor. Probably because of hardened derivations without private keys provided')

        for address in addresses:
            test_address(w1,
                         address,
                         ismine=False,
                         solvable=False)

        self.test_importdesc({"desc": descsum_create(desc), "timestamp": "now", "range": -1},
                              success=False, error_code=-8, error_message='End of range is too high')

        self.test_importdesc({"desc": descsum_create(desc), "timestamp": "now", "range": [-1, 10]},
                              success=False, error_code=-8, error_message='Range should be greater or equal than 0')

        self.test_importdesc({"desc": descsum_create(desc), "timestamp": "now", "range": [(2 << 31 + 1) - 1000000, (2 << 31 + 1)]},
                              success=False, error_code=-8, error_message='End of range is too high')

        self.test_importdesc({"desc": descsum_create(desc), "timestamp": "now", "range": [2, 1]},
                              success=False, error_code=-8, error_message='Range specified as [begin,end] must not have begin after end')

        self.test_importdesc({"desc": descsum_create(desc), "timestamp": "now", "range": [0, 1000001]},
                              success=False, error_code=-8, error_message='Range is too large')

        self.log.info("Verify we can only extend descriptor's range")
        range_request = {"desc": descsum_create(desc), "timestamp": "now", "range": [5, 10], 'active': True}
        self.test_importdesc(range_request, wallet=wpriv, success=True)
        assert_equal(wpriv.getwalletinfo()['keypoolsize'], 6)
        self.test_importdesc({**range_request, "range": [0, 10]}, wallet=wpriv, success=True)
        assert_equal(wpriv.getwalletinfo()['keypoolsize'], 11)
        self.test_importdesc({**range_request, "range": [0, 20]}, wallet=wpriv, success=True)
        assert_equal(wpriv.getwalletinfo()['keypoolsize'], 21)
        # Can keep range the same
        self.test_importdesc({**range_request, "range": [0, 20]}, wallet=wpriv, success=True)
        assert_equal(wpriv.getwalletinfo()['keypoolsize'], 21)

        self.test_importdesc({**range_request, "range": [5, 10]}, wallet=wpriv, success=False,
                             error_code=-4, error_message=f"Could not add descriptor '{range_request['desc']}': new range must include current range = [0,20]")
        self.test_importdesc({**range_request, "range": [0, 10]}, wallet=wpriv, success=False,
                             error_code=-4, error_message=f"Could not add descriptor '{range_request['desc']}': new range must include current range = [0,20]")
        self.test_importdesc({**range_request, "range": [5, 20]}, wallet=wpriv, success=False,
                             error_code=-4, error_message=f"Could not add descriptor '{range_request['desc']}': new range must include current range = [0,20]")
        assert_equal(wpriv.getwalletinfo()['keypoolsize'], 21)

        self.log.info("Check we can change descriptor internal flag")
        self.test_importdesc({**range_request, "range": [0, 20], "internal": True}, wallet=wpriv, success=True)
        assert_equal(wpriv.getwalletinfo()['keypoolsize'], 0)
        assert_raises_rpc_error(-4, 'This wallet has no available keys', wpriv.getnewaddress, '', 'p2sh-segwit')
        assert_equal(wpriv.getwalletinfo()['keypoolsize_hd_internal'], 21)
        wpriv.getrawchangeaddress('p2sh-segwit')

        self.test_importdesc({**range_request, "range": [0, 20], "internal": False}, wallet=wpriv, success=True)
        assert_equal(wpriv.getwalletinfo()['keypoolsize'], 21)
        wpriv.getnewaddress('', 'p2sh-segwit')
        assert_equal(wpriv.getwalletinfo()['keypoolsize_hd_internal'], 0)
        assert_raises_rpc_error(-4, 'This wallet has no available keys', wpriv.getrawchangeaddress, 'p2sh-segwit')

        # Make sure ranged imports import keys in order
        w1 = self.nodes[1].get_wallet_rpc('w1')
        self.log.info('Key ranges should be imported in order')
        xpub = "tpubDAXcJ7s7ZwicqjprRaEWdPoHKrCS215qxGYxpusRLLmJuT69ZSicuGdSfyvyKpvUNYBW1s2U3NSrT6vrCYB9e6nZUEvrqnwXPF8ArTCRXMY"
        addresses = [
            'bcrt1qtmp74ayg7p24uslctssvjm06q5phz4yrxucgnv', # m/0'/0'/0
            'bcrt1q8vprchan07gzagd5e6v9wd7azyucksq2xc76k8', # m/0'/0'/1
            'bcrt1qtuqdtha7zmqgcrr26n2rqxztv5y8rafjp9lulu', # m/0'/0'/2
            'bcrt1qau64272ymawq26t90md6an0ps99qkrse58m640', # m/0'/0'/3
            'bcrt1qsg97266hrh6cpmutqen8s4s962aryy77jp0fg0', # m/0'/0'/4
        ]

        self.test_importdesc({'desc': descsum_create('wpkh([80002067/0h/0h]' + xpub + '/*)'),
                              'active': True,
                              'range' : [0, 2],
                              'timestamp': 'now'
                             },
                             success=True)
        self.test_importdesc({'desc': descsum_create('sh(wpkh([abcdef12/0h/0h]' + xpub + '/*))'),
                              'active': True,
                              'range' : [0, 2],
                              'timestamp': 'now'
                             },
                             success=True)
        self.test_importdesc({'desc': descsum_create('pkh([12345678/0h/0h]' + xpub + '/*)'),
                              'active': True,
                              'range' : [0, 2],
                              'timestamp': 'now'
                             },
                             success=True)

        assert_equal(w1.getwalletinfo()['keypoolsize'], 5 * 3)
        for i, expected_addr in enumerate(addresses):
            received_addr = w1.getnewaddress('', 'bech32')
            assert_raises_rpc_error(-4, 'This wallet has no available keys', w1.getrawchangeaddress, 'bech32')
            assert_equal(received_addr, expected_addr)
            bech32_addr_info = w1.getaddressinfo(received_addr)
            assert_equal(bech32_addr_info['desc'][:23], 'wpkh([80002067/0h/0h/{}]'.format(i))

            shwpkh_addr = w1.getnewaddress('', 'p2sh-segwit')
            shwpkh_addr_info = w1.getaddressinfo(shwpkh_addr)
            assert_equal(shwpkh_addr_info['desc'][:26], 'sh(wpkh([abcdef12/0h/0h/{}]'.format(i))

            pkh_addr = w1.getnewaddress('', 'legacy')
            pkh_addr_info = w1.getaddressinfo(pkh_addr)
            assert_equal(pkh_addr_info['desc'][:22], 'pkh([12345678/0h/0h/{}]'.format(i))

            assert_equal(w1.getwalletinfo()['keypoolsize'], 4 * 3) # After retrieving a key, we don't refill the keypool again, so it's one less for each address type
        w1.keypoolrefill()
        assert_equal(w1.getwalletinfo()['keypoolsize'], 5 * 3)

        self.log.info("Check we can change next_index")
        # go back and forth with next_index
        for i in [4, 0, 2, 1, 3]:
            # Sometimes use h, sometimes use ' for hardened indicator
            hard = "'" if i & 2 == 0 else "h"
            self.test_importdesc({'desc': descsum_create(f'wpkh([80002067/0{hard}/0{hard}]' + xpub + '/*)'),
                                  'active': True,
                                  'range': [0, 9],
                                  'next_index': i,
                                  'timestamp': 'now'
                                  },
                                 success=True)
            assert_equal(w1.getnewaddress('', 'bech32'), addresses[i])

        self.log.info("Equivalent Miniscript descriptors should not be duplicated")
        self.nodes[1].createwallet(wallet_name="wminiscript", disable_private_keys=True, blank=True)
        wminiscript = self.nodes[1].get_wallet_rpc("wminiscript")
        miniscript_request = {
            'active': True,
            'range': [0, 9],
            'timestamp': 'now',
        }
        self.test_importdesc({
            **miniscript_request,
            'desc': descsum_create(f"wsh(and_v(v:pk([80002067/0h/0h]{xpub}/*),older(1)))"),
        }, success=True, wallet=wminiscript)
        self.test_importdesc({
            **miniscript_request,
            'desc': descsum_create(f"wsh(and_v(v:pk([80002067/0'/0']{xpub}/*),older(1)))"),
        }, success=True, wallet=wminiscript)
        assert_equal(len(wminiscript.listdescriptors()["descriptors"]), 1)

        # Check active=False default
        self.log.info('Check imported descriptors are not active by default')
        self.test_importdesc({'desc': descsum_create('pkh([12345678/1h]' + xpub + '/*)'),
                              'range' : [0, 2],
                              'timestamp': 'now',
                              'internal': True
                             },
                             success=True)
        assert_raises_rpc_error(-4, 'This wallet has no available keys', w1.getrawchangeaddress, 'legacy')

        self.log.info('Check can activate inactive descriptor')
        self.test_importdesc({'desc': descsum_create('pkh([12345678]' + xpub + '/*)'),
                              'range': [0, 5],
                              'active': True,
                              'timestamp': 'now',
                              'internal': True
                              },
                             success=True)
        address = w1.getrawchangeaddress('legacy')
        assert_equal(address, "mpA2Wh9dvZT7yfELq1UnrUmAoc5qCkMetg")

        self.log.info('Check can deactivate active descriptor')
        self.test_importdesc({'desc': descsum_create('pkh([12345678]' + xpub + '/*)'),
                              'range': [0, 5],
                              'active': False,
                              'timestamp': 'now',
                              'internal': True
                              },
                             success=True)
        assert_raises_rpc_error(-4, 'This wallet has no available keys', w1.getrawchangeaddress, 'legacy')

        self.log.info('Verify activation state is persistent')
        w1.unloadwallet()
        self.nodes[1].loadwallet('w1')
        assert_raises_rpc_error(-4, 'This wallet has no available keys', w1.getrawchangeaddress, 'legacy')

        # # Test importing a descriptor containing a WIF private key
        wif_priv = "cTe1f5rdT8A8DFgVWTjyPwACsDPJM9ff4QngFxUixCSvvbg1x6sh"
        address = "2MuhcG52uHPknxDgmGPsV18jSHFBnnRgjPg"
        desc = "sh(wpkh(" + wif_priv + "))"
        self.log.info("Should import a descriptor with a WIF private key as spendable")
        self.test_importdesc({"desc": descsum_create(desc),
                               "timestamp": "now"},
                              success=True,
                              wallet=wpriv)

        self.log.info('Test can import same descriptor with private key twice')
        self.test_importdesc({"desc": descsum_create(desc), "timestamp": "now"}, success=True, wallet=wpriv)

        test_address(wpriv,
                     address,
                     solvable=True,
                     ismine=True)
        txid = w0.sendtoaddress(address, 49.99995540)
        self.generatetoaddress(self.nodes[0], 6, w0.getnewaddress())
        tx = wpriv.createrawtransaction([{"txid": txid, "vout": 0}], {w0.getnewaddress(): 49.999})
        signed_tx = wpriv.signrawtransactionwithwallet(tx)
        w1.sendrawtransaction(signed_tx['hex'])

        # Make sure that we can use import and use multisig as addresses
        self.log.info('Test that multisigs can be imported, signed for, and getnewaddress\'d')
        self.nodes[1].createwallet(wallet_name="wmulti_priv", disable_private_keys=False, blank=True)
        wmulti_priv = self.nodes[1].get_wallet_rpc("wmulti_priv")
        assert_equal(wmulti_priv.getwalletinfo()['keypoolsize'], 0)

        derivation_path = "84h/0h/0h"
        change_derivation_path = "84h/1h/0h"
        extended_key_1 = ExtendedPrivateKey.generate()
        xprv1 = extended_key_1.to_string()
        xprv1_fingerprint = extended_key_1._fingerprint().hex()
        acc_xpub1_key = extended_key_1.derive_path(derivation_path).pubkey()
        acc_xpub1 = acc_xpub1_key.to_string()
        chg_xpub1_key = extended_key_1.derive_path(change_derivation_path).pubkey()
        chg_xpub1 = chg_xpub1_key.to_string()

        extended_key_2 = ExtendedPrivateKey.generate()
        xprv2 = extended_key_2.to_string()
        xprv2_fingerprint = extended_key_2._fingerprint().hex()
        acc_xprv2 = extended_key_2.derive_path(derivation_path).to_string()
        acc_xpub2_key = extended_key_2.derive_path(derivation_path).pubkey()
        acc_xpub2 = acc_xpub2_key.to_string()
        chg_xpub2_key = extended_key_2.derive_path(change_derivation_path).pubkey()
        chg_xpub2 = chg_xpub2_key.to_string()

        extended_key_3 = ExtendedPrivateKey.generate()
        xprv3 = extended_key_3.to_string()
        xprv3_fingerprint = extended_key_3._fingerprint().hex()
        acc_xpub3_key = extended_key_3.derive_path(derivation_path).pubkey()
        acc_xpub3 = acc_xpub3_key.to_string()
        chg_xpub3_key = extended_key_3.derive_path(change_derivation_path).pubkey()
        chg_xpub3 = chg_xpub3_key.to_string()

        self.test_musig_private_key_warnings(xprv1, acc_xprv2, acc_xpub2, derivation_path)

        self.test_importdesc({"desc": descsum_create(f"wsh(multi(2,{xprv1}/{derivation_path}/*,{xprv2}/{derivation_path}/*,{xprv3}/{derivation_path}/*))"),
                            "active": True,
                            "range": 1000,
                            "next_index": 0,
                            "timestamp": "now"},
                            success=True,
                            wallet=wmulti_priv)
        self.test_importdesc({"desc": descsum_create(f"wsh(multi(2,{xprv1}/{change_derivation_path}/*,{xprv2}/{change_derivation_path}/*,{xprv3}/{change_derivation_path}/*))"),
                            "active": True,
                            "internal" : True,
                            "range": 1000,
                            "next_index": 0,
                            "timestamp": "now"},
                            success=True,
                            wallet=wmulti_priv)

        assert_equal(wmulti_priv.getwalletinfo()['keypoolsize'], 1001) # Range end (1000) is inclusive, so 1001 addresses generated

        addr = wmulti_priv.getnewaddress('', 'bech32') # uses receive 0
        expected_addr = script_to_p2wsh(keys_to_multisig_script([k.derive_path("m/0").pubkey.get_bytes() for k in [acc_xpub1_key, acc_xpub2_key, acc_xpub3_key]], k=2))
        assert_equal(addr, expected_addr) # Derived at m/84'/0'/0'/0

        change_addr = wmulti_priv.getrawchangeaddress('bech32') # uses change 0
        expected_change_addr = script_to_p2wsh(keys_to_multisig_script([k.derive_path("m/0").pubkey.get_bytes() for k in [chg_xpub1_key, chg_xpub2_key, chg_xpub3_key]], k=2))
        assert_equal(change_addr, expected_change_addr)  # Derived at m/84'/1'/0'/0
        assert_equal(wmulti_priv.getwalletinfo()['keypoolsize'], 1000)

        txid = w0.sendtoaddress(addr, 10)
        self.generate(self.nodes[0], 6)
        send_txid = wmulti_priv.sendtoaddress(w0.getnewaddress(), 8) # uses change 1
        decoded = wmulti_priv.gettransaction(txid=send_txid, verbose=True)['decoded']
        assert_equal(len(decoded['vin'][0]['txinwitness']), 4)
        self.sync_all()

        self.nodes[1].createwallet(wallet_name="wmulti_pub", disable_private_keys=True, blank=True)
        wmulti_pub = self.nodes[1].get_wallet_rpc("wmulti_pub")
        assert_equal(wmulti_pub.getwalletinfo()['keypoolsize'], 0)

        self.test_importdesc({"desc": descsum_create(f"wsh(multi(2,[{xprv1_fingerprint}/{derivation_path}]{acc_xpub1}/*,[{xprv2_fingerprint}/{derivation_path}]{acc_xpub2}/*,[{xprv3_fingerprint}/{derivation_path}]{acc_xpub3}/*))"),
                            "active": True,
                            "range": 1000,
                            "next_index": 0,
                            "timestamp": "now"},
                            success=True,
                            wallet=wmulti_pub)
        self.test_importdesc({"desc": descsum_create(f"wsh(multi(2,[{xprv1_fingerprint}/{change_derivation_path}]{chg_xpub1}/*,[{xprv2_fingerprint}/{change_derivation_path}]{chg_xpub2}/*,[{xprv3_fingerprint}/{change_derivation_path}]{chg_xpub3}/*))"),
                            "active": True,
                            "internal" : True,
                            "range": 1000,
                            "next_index": 0,
                            "timestamp": "now"},
                            success=True,
                            wallet=wmulti_pub)

        assert_equal(wmulti_pub.getwalletinfo()['keypoolsize'], 1000) # The first one was already consumed by previous import and is detected as used
        addr = wmulti_pub.getnewaddress('', 'bech32') # uses receive 1
        expected_addr = script_to_p2wsh(keys_to_multisig_script([k.derive_path("m/1").pubkey.get_bytes() for k in [acc_xpub1_key, acc_xpub2_key, acc_xpub3_key]], k=2))
        assert_equal(addr, expected_addr) # Derived at m/84'/0'/0'/1
        assert_equal(wmulti_pub.getaddressinfo(addr)["desc"].count(f"{derivation_path}/1"), 3) # Derived at m/84h/0h/0h/1 for all three keys
        change_addr = wmulti_pub.getrawchangeaddress('bech32') # uses change 2
        expected_change_addr = script_to_p2wsh(keys_to_multisig_script([k.derive_path("m/2").pubkey.get_bytes() for k in [chg_xpub1_key, chg_xpub2_key, chg_xpub3_key]], k=2))
        assert_equal(change_addr, expected_change_addr)  # Derived at m/84'/1'/0'/2
        assert_equal(wmulti_pub.getaddressinfo(change_addr)["desc"].count(f"{change_derivation_path}/2"), 3) # Derived at m/84h/1h/0h/1 for all three keys
        assert send_txid in self.nodes[0].getrawmempool(True)
        assert send_txid in (x['txid'] for x in wmulti_pub.listunspent(0))
        assert_equal(wmulti_pub.getwalletinfo()['keypoolsize'], 999)

        # generate some utxos for next tests
        utxo = self.create_outpoints(w0, outputs=[{addr: 10}])[0]

        addr2 = wmulti_pub.getnewaddress('', 'bech32')
        utxo2 = self.create_outpoints(w0, outputs=[{addr2: 10}])[0]

        self.generate(self.nodes[0], 6)
        assert_equal(wmulti_pub.getbalance(), wmulti_priv.getbalance())

        # Make sure that descriptor wallets containing multiple xpubs in a single descriptor load correctly
        wmulti_pub.unloadwallet()
        self.nodes[1].loadwallet('wmulti_pub')

        self.log.info("Multisig with distributed keys")
        self.nodes[1].createwallet(wallet_name="wmulti_priv1")
        wmulti_priv1 = self.nodes[1].get_wallet_rpc("wmulti_priv1")
        res = wmulti_priv1.importdescriptors([
        {
            "desc": descsum_create(f"wsh(multi(2,{xprv1}/{derivation_path}/*,[{xprv2_fingerprint}/{derivation_path}]{acc_xpub2}/*,[{xprv3_fingerprint}/{derivation_path}]{acc_xpub3}/*))"),
            "active": True,
            "range": 1000,
            "next_index": 0,
            "timestamp": "now"
        },
        {
            "desc": descsum_create(f"wsh(multi(2,{xprv1}/{change_derivation_path}/*,[{xprv2_fingerprint}/{change_derivation_path}]{chg_xpub2}/*,[{xprv3_fingerprint}/{change_derivation_path}]{chg_xpub3}/*))"),
            "active": True,
            "internal" : True,
            "range": 1000,
            "next_index": 0,
            "timestamp": "now"
        }])
        assert_equal(res[0]['success'], True)
        assert_equal(res[0]['warnings'][0], MISSING_KEYS_WARNING)
        assert_equal(res[1]['success'], True)
        assert_equal(res[1]['warnings'][0], MISSING_KEYS_WARNING)

        self.nodes[1].createwallet(wallet_name='wmulti_priv2', blank=True)
        wmulti_priv2 = self.nodes[1].get_wallet_rpc('wmulti_priv2')
        res = wmulti_priv2.importdescriptors([
        {
            "desc": descsum_create(f"wsh(multi(2,[{xprv1_fingerprint}/{derivation_path}]{acc_xpub1}/*,{xprv2}/{derivation_path}/*,[{xprv3_fingerprint}/{derivation_path}]{acc_xpub3}/*))"),
            "active": True,
            "range": 1000,
            "next_index": 0,
            "timestamp": "now"
        },
        {
            "desc": descsum_create(f"wsh(multi(2,[{xprv1_fingerprint}/{change_derivation_path}]{chg_xpub1}/*,{xprv2}/{change_derivation_path}/*,[{xprv3_fingerprint}/{change_derivation_path}]{chg_xpub3}/*))"),
            "active": True,
            "internal" : True,
            "range": 1000,
            "next_index": 0,
            "timestamp": "now"
        }])
        assert_equal(res[0]['success'], True)
        assert_equal(res[0]['warnings'][0], MISSING_KEYS_WARNING)
        assert_equal(res[1]['success'], True)
        assert_equal(res[1]['warnings'][0], MISSING_KEYS_WARNING)

        rawtx = self.nodes[1].createrawtransaction([utxo], {w0.getnewaddress(): 9.999})
        tx_signed_1 = wmulti_priv1.signrawtransactionwithwallet(rawtx)
        assert_equal(tx_signed_1['complete'], False)
        tx_signed_2 = wmulti_priv2.signrawtransactionwithwallet(tx_signed_1['hex'])
        assert_equal(tx_signed_2['complete'], True)
        self.nodes[1].sendrawtransaction(tx_signed_2['hex'])

        self.log.info("We can create and use a huge multisig under P2WSH")
        self.nodes[1].createwallet(wallet_name='wmulti_priv_big', blank=True)
        wmulti_priv_big = self.nodes[1].get_wallet_rpc('wmulti_priv_big')
        xprv = ExtendedPrivateKey.generate().to_string()
        xkey = f"{xprv}/*"
        xkey_int = f"{xprv}/1/*"
        res = wmulti_priv_big.importdescriptors([
        {
            "desc": descsum_create(f"wsh(sortedmulti(20,{(xkey + ',') * 19}{xkey}))"),
            "active": True,
            "range": 1000,
            "next_index": 0,
            "timestamp": "now"
        },
        {
            "desc": descsum_create(f"wsh(sortedmulti(20,{(xkey_int + ',') * 19}{xkey_int}))"),
            "active": True,
            "internal": True,
            "range": 1000,
            "next_index": 0,
            "timestamp": "now"
        }])
        assert_equal(res[0]['success'], True)
        assert_equal(res[1]['success'], True)

        addr = wmulti_priv_big.getnewaddress()
        w0.sendtoaddress(addr, 10)
        self.generate(self.nodes[0], 1)
        # It is standard and would relay.
        txid = wmulti_priv_big.sendtoaddress(w0.getnewaddress(), 9.999)
        decoded = wmulti_priv_big.gettransaction(txid=txid, verbose=True)['decoded']
        # 20 sigs + dummy + witness script
        assert_equal(len(decoded['vin'][0]['txinwitness']), 22)


        self.log.info("Under P2SH, multisig are standard with up to 15 "
                      "compressed keys")
        self.nodes[1].createwallet(wallet_name='multi_priv_big_legacy',
                                   blank=True)
        multi_priv_big = self.nodes[1].get_wallet_rpc('multi_priv_big_legacy')
        res = multi_priv_big.importdescriptors([
        {
            "desc": descsum_create(f"sh(multi(15,{(xkey + ',') * 14}{xkey}))"),
            "active": True,
            "range": 1000,
            "next_index": 0,
            "timestamp": "now"
        },
        {
            "desc": descsum_create(f"sh(multi(15,{(xkey_int + ',') * 14}{xkey_int}))"),
            "active": True,
            "internal": True,
            "range": 1000,
            "next_index": 0,
            "timestamp": "now"
        }])
        assert_equal(res[0]['success'], True)
        assert_equal(res[1]['success'], True)

        addr = multi_priv_big.getnewaddress("", "legacy")
        w0.sendtoaddress(addr, 10)
        self.generate(self.nodes[0], 6)
        # It is standard and would relay.
        txid = multi_priv_big.sendtoaddress(w0.getnewaddress(), 10, "", "", True)
        decoded = multi_priv_big.gettransaction(txid=txid, verbose=True)['decoded']

        self.log.info("Amending multisig with new private keys")
        self.nodes[1].createwallet(wallet_name="wmulti_priv3")
        wmulti_priv3 = self.nodes[1].get_wallet_rpc("wmulti_priv3")
        res = wmulti_priv3.importdescriptors([
            {
                "desc": descsum_create(f"wsh(multi(2,{xprv1}/{derivation_path}/*,[{xprv2_fingerprint}/{derivation_path}]{acc_xpub2}/*,[{xprv3_fingerprint}/{derivation_path}]{acc_xpub3}/*))"),
                "active": True,
                "range": 1000,
                "next_index": 0,
                "timestamp": "now"
            }])
        assert_equal(res[0]['success'], True)
        res = wmulti_priv3.importdescriptors([
            {
                "desc": descsum_create(f"wsh(multi(2,{xprv1}/{derivation_path}/*,[{xprv2_fingerprint}/{derivation_path}]{acc_xprv2}/*,[{xprv3_fingerprint}/{derivation_path}]{acc_xpub3}/*))"),
                "active": True,
                "range": 1000,
                "next_index": 0,
                "timestamp": "now"
            }])
        assert_equal(res[0]['success'], True)

        rawtx = self.nodes[1].createrawtransaction([utxo2], {w0.getnewaddress(): 9.999})
        tx = wmulti_priv3.signrawtransactionwithwallet(rawtx)
        assert_equal(tx['complete'], True)
        self.nodes[1].sendrawtransaction(tx['hex'])

        self.log.info("Combo descriptors cannot be active")
        self.test_importdesc({"desc": descsum_create(f"combo({ExtendedPrivateKey.generate().pubkey().to_string()}/*)"),
                              "active": True,
                              "range": 1,
                              "timestamp": "now"},
                              success=False,
                              error_code=-4,
                              error_message="Combo descriptors cannot be set to active")

        self.log.info("Descriptors with no type cannot be active")
        self.test_importdesc({"desc": descsum_create(f"pk({ExtendedPrivateKey.generate().pubkey().to_string()}/*)"),
                              "active": True,
                              "range": 1,
                              "timestamp": "now"},
                              success=True,
                              warnings=["Unknown output type, cannot set descriptor to active."])

        self.log.info("Test importing a descriptor to an encrypted wallet")

        descriptor = {"desc": descsum_create("pkh(" + xpriv + "/1h/*h)"),
                              "timestamp": "now",
                              "active": True,
                              "range": [0,4000],
                              "next_index": 4000}

        self.nodes[0].createwallet("temp_wallet", blank=True)
        temp_wallet = self.nodes[0].get_wallet_rpc("temp_wallet")
        temp_wallet.importdescriptors([descriptor])
        self.generatetoaddress(self.nodes[0], COINBASE_MATURITY + 1, temp_wallet.getnewaddress())
        self.generatetoaddress(self.nodes[0], COINBASE_MATURITY + 1, temp_wallet.getnewaddress())

        self.nodes[0].createwallet("encrypted_wallet", blank=True, passphrase="passphrase")
        encrypted_wallet = self.nodes[0].get_wallet_rpc("encrypted_wallet")

        self.log.info("Wallet must be unlocked to import a descriptor")
        assert_raises_rpc_error(-13, "Error: Please enter the wallet passphrase with walletpassphrase first.",
            encrypted_wallet.importdescriptors, [descriptor])

        self.log.info("A locked wallet rejects an empty importdescriptors request")
        assert_raises_rpc_error(-13, "Error: Please enter the wallet passphrase with walletpassphrase first.",
            encrypted_wallet.importdescriptors, [])

        self.log.info("An unlocked wallet accepts an empty importdescriptors request")
        self.nodes[0].createwallet("unencrypted_wallet", blank=True)
        unencrypted_wallet = self.nodes[0].get_wallet_rpc("unencrypted_wallet")
        assert_equal(unencrypted_wallet.importdescriptors([]), [])

        descriptor["timestamp"] = 0
        descriptor["next_index"] = 0

        encrypted_wallet.walletpassphrase("passphrase", 99999)
        with concurrent.futures.ThreadPoolExecutor(max_workers=1) as thread:
            with self.nodes[0].assert_debug_log(expected_msgs=["Rescan started from block 0f9188f13cb7b2c71f2a335e3a4fc328bf5beb436012afca590b1a11466e2206... (slow variant inspecting all blocks)"], timeout=10):
                importing = thread.submit(encrypted_wallet.importdescriptors, requests=[descriptor])

            # Set the passphrase timeout to 1 to test that the wallet remains unlocked during the rescan
            self.nodes[0].cli("-rpcwallet=encrypted_wallet").walletpassphrase("passphrase", 1)

            try:
                self.nodes[0].cli("-rpcwallet=encrypted_wallet").walletlock()
            except JSONRPCException as e:
                assert_equal(e.error["code"], -4)
                assert "Error: the wallet is currently being used to rescan the blockchain for related transactions. Please call `abortrescan` before locking the wallet." in e.error["message"]

            try:
                self.nodes[0].cli("-rpcwallet=encrypted_wallet").walletpassphrasechange("passphrase", "newpassphrase")
            except JSONRPCException as e:
                assert_equal(e.error["code"], -4)
                assert "Error: the wallet is currently being used to rescan the blockchain for related transactions. Please call `abortrescan` before changing the passphrase." in e.error["message"]

            assert_equal(importing.result(), [{"success": True}])

        assert_equal(temp_wallet.getbalance(), encrypted_wallet.getbalance())

        self.log.info("Multipath descriptors")
        self.nodes[1].createwallet(wallet_name="multipath", blank=True)
        w_multipath = self.nodes[1].get_wallet_rpc("multipath")
        self.nodes[1].createwallet(wallet_name="multipath_split", blank=True)
        w_multisplit = self.nodes[1].get_wallet_rpc("multipath_split")
        timestamp = int(time.time())

        self.test_importdesc({"desc": descsum_create(f"wpkh({xpriv}/<10;20>/0/*)"),
                              "active": True,
                              "range": 10,
                              "timestamp": "now",
                              "label": "some label"},
                              success=False,
                              error_code=-8,
                              error_message="Multipath descriptors should not have a label",
                              wallet=w_multipath)
        self.test_importdesc({"desc": descsum_create(f"wpkh({xpriv}/<10;20>/0/*)"),
                              "active": True,
                              "range": 10,
                              "timestamp": timestamp,
                              "internal": True},
                              success=False,
                              error_code=-5,
                              error_message="Cannot have multipath descriptor while also specifying \'internal\'",
                              wallet=w_multipath)

        self.test_importdesc({"desc": descsum_create(f"wpkh({xpriv}/<10;20>/0/*)"),
                              "active": True,
                              "range": 10,
                              "timestamp": timestamp},
                              success=True,
                              wallet=w_multipath)

        self.test_importdesc({"desc": descsum_create(f"wpkh({xpriv}/10/0/*)"),
                              "active": True,
                              "range": 10,
                              "timestamp": timestamp},
                              success=True,
                              wallet=w_multisplit)
        self.test_importdesc({"desc": descsum_create(f"wpkh({xpriv}/20/0/*)"),
                              "active": True,
                              "range": 10,
                              "internal": True,
                              "timestamp": timestamp},
                              success=True,
                              wallet=w_multisplit)
        for _ in range(0, 10):
            assert_equal(w_multipath.getnewaddress(address_type="bech32"), w_multisplit.getnewaddress(address_type="bech32"))
            assert_equal(w_multipath.getrawchangeaddress(address_type="bech32"), w_multisplit.getrawchangeaddress(address_type="bech32"))
        assert_equal(sorted(w_multipath.listdescriptors()["descriptors"], key=lambda x: x["desc"]), sorted(w_multisplit.listdescriptors()["descriptors"], key=lambda x: x["desc"]))

        self.log.info("Test older() safety")

        for flag in [0, SEQUENCE_LOCKTIME_TYPE_FLAG]:
            self.log.debug("Importing a safe value always works")
            safe_value = (65535 | flag)
            self.test_importdesc(
                {
                    'desc': descsum_create(f"wsh(and_v(v:pk([12345678/0h/0h]{xpub}/*),older({safe_value})))"),
                    'active': True,
                    'range': [0, 2],
                    'timestamp': 'now'
                },
                success=True
            )

            self.log.debug("Importing an unsafe value results in a warning")
            unsafe_value = safe_value + 1
            desc = descsum_create(f"wsh(and_v(v:pk([12345678/0h/0h]{xpub}/*),older({unsafe_value})))")
            expected_warning = (
                f"time-based relative locktime: older({unsafe_value}) > (65535 * 512) seconds is unsafe"
                if flag == SEQUENCE_LOCKTIME_TYPE_FLAG
                else f"height-based relative locktime: older({unsafe_value}) > 65535 blocks is unsafe"
            )
            self.test_importdesc(
                {
                    'desc': desc,
                    'active': True,
                    'range': [0, 2],
                    'timestamp': 'now'
                },
                success=True,
                warnings=[expected_warning],
            )


        self.test_import_unused_key()
        self.test_import_unused_key_existing()
        self.test_import_unused_noprivs()
        self.test_per_item_errors_are_reported_in_order()
        self.test_rescan_fails_import()
        self.test_verify_balance(self.nodes[0], w0)
        self.test_verify_balance_unmatched(self.nodes[0], w0)
        self.test_verify_balance_mempool(self.nodes[0], w0)
        self.test_verify_balance_pruned(self.nodes[0], w0)

if __name__ == '__main__':
    ImportDescriptorsTest(__file__).main()
