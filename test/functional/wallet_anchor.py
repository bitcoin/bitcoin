#!/usr/bin/env python3
# Copyright (c) 2025-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://www.opensource.org/licenses/mit-license.php.

import time

from test_framework.blocktools import MAX_FUTURE_BLOCK_TIME
from test_framework.descriptors import descsum_create
from test_framework.messages import (
    COutPoint,
    CTxIn,
    CTxInWitness,
    CTxOut,
)
from test_framework.script_util import PAY_TO_ANCHOR
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_raises_rpc_error,
)
from test_framework.wallet import MiniWallet

ANCHOR_ADDRESS = "bcrt1pfeesnyr2tx"

class WalletAnchorTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def test_0_value_anchor_listunspent(self):
        self.log.info("Test that 0-value anchor outputs are detected as UTXOs")

        # Create an anchor output, and spend it
        sender = MiniWallet(self.nodes[0])
        anchor_tx = sender.create_self_transfer(fee_rate=0, version=3)["tx"]
        anchor_tx.vout.append(CTxOut(0, PAY_TO_ANCHOR))
        anchor_spend = sender.create_self_transfer(version=3)["tx"]
        anchor_spend.vin.append(CTxIn(COutPoint(anchor_tx.txid_int, 1), b""))
        anchor_spend.wit.vtxinwit.append(CTxInWitness())
        submit_res = self.nodes[0].submitpackage([anchor_tx.serialize().hex(), anchor_spend.serialize().hex()])
        assert_equal(submit_res["package_msg"], "success")
        anchor_txid = anchor_tx.txid_hex
        anchor_spend_txid = anchor_spend.txid_hex

        # Mine each tx in separate blocks
        self.generateblock(self.nodes[0], sender.get_address(), [anchor_tx.serialize().hex()])
        anchor_tx_height = self.nodes[0].getblockcount()
        self.generateblock(self.nodes[0], sender.get_address(), [anchor_spend.serialize().hex()])

        # Mock time forward and generate some blocks to avoid rescanning of latest blocks
        self.nodes[0].setmocktime(int(time.time()) + MAX_FUTURE_BLOCK_TIME + 1)
        self.generate(self.nodes[0], 10)

        self.nodes[0].createwallet(wallet_name="anchor", disable_private_keys=True)
        wallet = self.nodes[0].get_wallet_rpc("anchor")
        import_res = wallet.importdescriptors([{"desc": descsum_create(f"addr({ANCHOR_ADDRESS})"), "timestamp": "now"}])
        assert_equal(import_res[0]["success"], True)

        # The wallet should have no UTXOs, and not know of the anchor tx or its spend
        assert_equal(wallet.listunspent(), [])
        assert_raises_rpc_error(-5, "Invalid or non-wallet transaction id", wallet.gettransaction, anchor_txid)
        assert_raises_rpc_error(-5, "Invalid or non-wallet transaction id", wallet.gettransaction, anchor_spend_txid)

        # Rescanning the block containing the anchor so that listunspent will list the output
        wallet.rescanblockchain(0, anchor_tx_height)
        utxos = wallet.listunspent()
        assert_equal(len(utxos), 1)
        assert_equal(utxos[0]["txid"], anchor_txid)
        assert_equal(utxos[0]["address"], ANCHOR_ADDRESS)
        assert_equal(utxos[0]["amount"], 0)
        wallet.gettransaction(anchor_txid)
        assert_raises_rpc_error(-5, "Invalid or non-wallet transaction id", wallet.gettransaction, anchor_spend_txid)

        # Rescan the rest of the blockchain to see the anchor was spent
        wallet.rescanblockchain()
        assert_equal(wallet.listunspent(), [])
        wallet.gettransaction(anchor_spend_txid)

    def test_import_anchors(self):
        self.log.info("Test that descriptors for anchor outputs can only be imported to watchonly wallets")
        self.nodes[0].createwallet(wallet_name="import_anchor_privkeys", blank=True)
        wallet = self.nodes[0].get_wallet_rpc("import_anchor_privkeys")
        import_res = wallet.importdescriptors([
            {"desc": descsum_create(f"addr({ANCHOR_ADDRESS})"), "timestamp": "now"},
            {"desc": descsum_create(f"raw({PAY_TO_ANCHOR.hex()})"), "timestamp": "now"}
        ])
        assert_equal(import_res[0]["success"], False)
        assert_equal(import_res[1]["success"], False)

        self.nodes[0].createwallet(wallet_name="import_anchor", disable_private_keys=True)
        wallet = self.nodes[0].get_wallet_rpc("import_anchor")
        import_res = wallet.importdescriptors([
            {"desc": descsum_create(f"addr({ANCHOR_ADDRESS})"), "timestamp": "now"},
            {"desc": descsum_create(f"raw({PAY_TO_ANCHOR.hex()})"), "timestamp": "now"}
        ])
        assert_equal(import_res[0]["success"], True)
        assert_equal(import_res[1]["success"], True)

    def test_cannot_sign_anchors(self):
        self.log.info("Test that the wallet cannot spend anchor outputs")
        self.nodes[0].createwallet(wallet_name="anchor_spend", disable_private_keys=True)
        wallet = self.nodes[0].get_wallet_rpc("anchor_spend")
        import_res = wallet.importdescriptors([
            {"desc": descsum_create(f"addr({ANCHOR_ADDRESS})"), "timestamp": "now"},
            {"desc": descsum_create(f"raw({PAY_TO_ANCHOR.hex()})"), "timestamp": "now"}
        ])
        assert_equal(import_res[0]["success"], True)
        assert_equal(import_res[1]["success"], True)

        anchor_txid = self.default_wallet.sendtoaddress(ANCHOR_ADDRESS, 1)
        self.generate(self.nodes[0], 1)

        utxos = wallet.listunspent()
        assert_equal(len(utxos), 1)
        assert_equal(utxos[0]["txid"], anchor_txid)
        assert_equal(utxos[0]["address"], ANCHOR_ADDRESS)
        assert_equal(utxos[0]["amount"], 1)

        assert_raises_rpc_error(-4, "Missing solving data for estimating transaction size", wallet.send, [{self.default_wallet.getnewaddress(): 0.9999}])
        assert_raises_rpc_error(-4, "Error: Private keys are disabled for this wallet", wallet.sendtoaddress, self.default_wallet.getnewaddress(), 0.9999)
        assert_raises_rpc_error(-4, "Unable to determine the size of the transaction, the wallet contains unsolvable descriptors", wallet.sendall, recipients=[self.default_wallet.getnewaddress()], inputs=utxos)

    def run_test(self):
        self.default_wallet = self.nodes[0].get_wallet_rpc(self.default_wallet_name)
        self.test_0_value_anchor_listunspent()
        self.test_import_anchors()
        self.test_cannot_sign_anchors()

if __name__ == '__main__':
    WalletAnchorTest(__file__).main()
