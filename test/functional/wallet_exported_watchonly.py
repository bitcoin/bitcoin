#!/usr/bin/env python3
# Copyright (c) 2025-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://www.opensource.org/licenses/mit-license.php.

import os

from test_framework.descriptors import descsum_create
from test_framework.key import H_POINT
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_not_equal,
    assert_raises_rpc_error,
)
from test_framework.wallet_util import generate_keypair

KEYPOOL_SIZE = 10

class WalletExportedWatchOnly(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 2
        self.extra_args = [[], [f"-keypool={KEYPOOL_SIZE}"]]

    def setup_network(self):
        # Setup the nodes but don't connect them to each other
        self.setup_nodes()

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def export_and_restore(self, wallet, export_name):
        export_path = os.path.join(self.export_path, f"{export_name}.dat")
        res = wallet.exportwatchonlywallet(export_path)
        assert_equal(res["exported_file"], export_path)
        self.online.restorewallet(export_name, res["exported_file"])
        return self.online.get_wallet_rpc(export_name)

    def test_basic_export(self):
        self.log.info("Test basic watchonly wallet export")
        self.offline.createwallet("basic")
        offline_wallet = self.offline.get_wallet_rpc("basic")

        # Bad RPC args
        assert_raises_rpc_error(-4, "Error: Export ", offline_wallet.exportwatchonlywallet, "")
        assert_raises_rpc_error(-4, "Error: Export destination '.' already exists", offline_wallet.exportwatchonlywallet, ".")
        assert_raises_rpc_error(-4, f"Error: Export destination '{self.export_path}' already exists", offline_wallet.exportwatchonlywallet, self.export_path)

        # Export the watchonly wallet file and load onto online node
        online_wallet = self.export_and_restore(offline_wallet, "basic_watchonly")

        # Exporting watchonly from a watchonly also works
        online_wallet2 = self.export_and_restore(online_wallet, "basic_watchonly2")

        # Verify that the wallets have the same descriptors
        addr = offline_wallet.getnewaddress()
        assert_equal(addr, online_wallet.getnewaddress())
        assert_equal(addr, online_wallet2.getnewaddress())
        assert_equal(offline_wallet.listdescriptors()["descriptors"], online_wallet.listdescriptors()["descriptors"])
        assert_equal(offline_wallet.listdescriptors()["descriptors"], online_wallet2.listdescriptors()["descriptors"])

        # Verify that online wallet cannot spend, but offline can
        self.funder.sendtoaddress(online_wallet.getnewaddress(), 10)
        self.generate(self.online, 1, sync_fun=self.no_op)
        assert_equal(online_wallet.getbalances()["mine"]["trusted"], 10)
        assert_equal(offline_wallet.getbalances()["mine"]["trusted"], 0)
        funds_addr = self.funder.getnewaddress()
        send_res = online_wallet.send([{funds_addr: 5}])
        assert_equal(send_res["complete"], False)
        assert "psbt" in send_res
        signed_psbt = offline_wallet.walletprocesspsbt(send_res["psbt"])["psbt"]
        finalized = self.online.finalizepsbt(signed_psbt)["hex"]
        self.online.sendrawtransaction(finalized)

        # Verify that the change address is known to both wallets
        dec_tx = self.online.decoderawtransaction(finalized)
        for txout in dec_tx["vout"]:
            if txout["scriptPubKey"]["address"] == funds_addr:
                continue
            assert_equal(online_wallet.getaddressinfo(txout["scriptPubKey"]["address"])["ismine"], True)
            assert_equal(offline_wallet.getaddressinfo(txout["scriptPubKey"]["address"])["ismine"], True)

        self.generate(self.online, 1, sync_fun=self.no_op)
        offline_wallet.unloadwallet()
        online_wallet.unloadwallet()

    def test_export_with_address_book(self):
        self.log.info("Test all address book entries appear in the exported wallet")
        self.offline.createwallet("addrbook")
        offline_wallet = self.offline.get_wallet_rpc("addrbook")

        # Create some address book entries
        receive_addr = offline_wallet.getnewaddress(label="addrbook_receive")
        send_addr = self.funder.getnewaddress()
        offline_wallet.setlabel(send_addr, "addrbook_send") # Sets purpose "send"

        # Export the watchonly wallet file and load onto online node
        online_wallet = self.export_and_restore(offline_wallet, "addrbook_watchonly")

        # Verify the labels are in both wallets
        for wallet in [online_wallet, offline_wallet]:
            for purpose in ["receive", "send"]:
                label = f"addrbook_{purpose}"
                assert_equal(wallet.listlabels(purpose), [label])
                addr = send_addr if purpose == "send" else receive_addr
                assert_equal(wallet.getaddressesbylabel(label), {addr: {"purpose": purpose}})

        offline_wallet.unloadwallet()
        online_wallet.unloadwallet()

    def test_export_with_txs_and_locked_coins(self):
        self.log.info("Test all transactions and locked coins appear in the exported wallet")
        self.offline.createwallet("txs")
        offline_wallet = self.offline.get_wallet_rpc("txs")

        # In order to make transactions in the offline wallet, briefly connect offline to online
        self.connect_nodes(self.offline.index, self.online.index)
        txids = [self.funder.sendtoaddress(offline_wallet.getnewaddress("funds"), i) for i in range(1, 4)]
        self.generate(self.online, 1)
        self.disconnect_nodes(self.offline.index ,self.online.index)

        # lock some coins
        persistent_lock = [{"txid": txids[0], "vout": 0}]
        temp_lock = [{"txid": txids[1], "vout": 0}]
        offline_wallet.lockunspent(unlock=False, transactions=persistent_lock, persistent=True)
        offline_wallet.lockunspent(unlock=False, transactions=temp_lock, persistent=False)

        # Export the watchonly wallet file and load onto online node
        online_wallet = self.export_and_restore(offline_wallet, "txs_watchonly")

        # Verify the transactions are in both wallets
        for txid in txids:
            assert_equal(online_wallet.gettransaction(txid), offline_wallet.gettransaction(txid))

        # Verify that the persistent locked coin is locked in both wallets
        assert_equal(online_wallet.listlockunspent(), persistent_lock)
        assert_equal(sorted(offline_wallet.listlockunspent(), key=lambda x: x["txid"]), sorted(persistent_lock + temp_lock, key=lambda x: x["txid"]))

        offline_wallet.unloadwallet()
        online_wallet.unloadwallet()

    def test_export_imported_descriptors(self):
        self.log.info("Test imported descriptors are exported to the watchonly wallet")
        self.offline.createwallet("imports")
        offline_wallet = self.offline.get_wallet_rpc("imports")

        import_res = offline_wallet.importdescriptors(
            [
                # A single key, non-ranged
                {"desc": descsum_create(f"pkh({generate_keypair(wif=True)[0]})"), "timestamp": "now"},
                # hardened derivation
                {"desc": descsum_create("sh(wpkh(tprv8ZgxMBicQKsPeuVhWwi6wuMQGfPKi9Li5GtX35jVNknACgqe3CY4g5xgkfDDJcmtF7o1QnxWDRYw4H5P26PXq7sbcUkEqeR4fg3Kxp2tigg/0'/*'))"), "timestamp": "now", "active": True},
                # multisig
                {"desc": descsum_create("wsh(multi(1,tpubD6NzVbkrYhZ4WaWSyoBvQwbpLkojyoTZPRsgXELWz3Popb3qkjcJyJUGLnL4qHHoQvao8ESaAstxYSnhyswJ76uZPStJRJCTKvosUCJZL5B/*,tprv8ZgxMBicQKsPeuVhWwi6wuMQGfPKi9Li5GtX35jVNknACgqe3CY4g5xgkfDDJcmtF7o1QnxWDRYw4H5P26PXq7sbcUkEqeR4fg3Kxp2tigg/*))"), "timestamp": "now", "active": True, "internal": True},
                # taproot multi scripts
                {"desc": descsum_create(f"tr({H_POINT},{{pk(tpubD6NzVbkrYhZ4WaWSyoBvQwbpLkojyoTZPRsgXELWz3Popb3qkjcJyJUGLnL4qHHoQvao8ESaAstxYSnhyswJ76uZPStJRJCTKvosUCJZL5B/*),pk(tprv8ZgxMBicQKsPeuVhWwi6wuMQGfPKi9Li5GtX35jVNknACgqe3CY4g5xgkfDDJcmtF7o1QnxWDRYw4H5P26PXq7sbcUkEqeR4fg3Kxp2tigg/0h/*)}})"), "timestamp": "now", "active": True},
                # miniscript
                {"desc": descsum_create(f"tr({H_POINT},or_b(pk(tpubD6NzVbkrYhZ4WaWSyoBvQwbpLkojyoTZPRsgXELWz3Popb3qkjcJyJUGLnL4qHHoQvao8ESaAstxYSnhyswJ76uZPStJRJCTKvosUCJZL5B/1/2/*),s:pk(tprv8ZgxMBicQKsPeuVhWwi6wuMQGfPKi9Li5GtX35jVNknACgqe3CY4g5xgkfDDJcmtF7o1QnxWDRYw4H5P26PXq7sbcUkEqeR4fg3Kxp2tigg/1h/2/*)))"), "timestamp": "now", "active": True, "internal": True},
            ]
        )
        assert_equal(all([r["success"] for r in import_res]), True)

        # Export the watchonly wallet file and load onto online node
        online_wallet = self.export_and_restore(offline_wallet, "imports_watchonly")

        # Verify public descriptors are the same
        assert_equal(offline_wallet.listdescriptors()["descriptors"], online_wallet.listdescriptors()["descriptors"])

        # Verify all the addresses are the same
        for address_type in ["legacy", "p2sh-segwit", "bech32", "bech32m"]:
            for internal in [False, True]:
                if internal:
                    addr = offline_wallet.getrawchangeaddress(address_type=address_type)
                    assert_equal(addr, online_wallet.getrawchangeaddress(address_type=address_type))
                else:
                    addr = offline_wallet.getnewaddress(address_type=address_type)
                    assert_equal(addr, online_wallet.getnewaddress(address_type=address_type))
                self.funder.sendtoaddress(addr, 1)
        self.generate(self.online, 1, sync_fun=self.no_op)

        # The hardened derivation should have KEYPOOL_SIZE - 1 remaining addresses
        for _ in range(KEYPOOL_SIZE - 1):
            online_wallet.getnewaddress(address_type="p2sh-segwit")
        assert_raises_rpc_error(-12, "No addresses available", online_wallet.getnewaddress, address_type="p2sh-segwit")

        # Verify that the offline wallet can sign and send
        send_res = online_wallet.sendall([self.funder.getnewaddress()])
        assert_equal(send_res["complete"], False)
        assert "psbt" in send_res
        signed_psbt = offline_wallet.walletprocesspsbt(send_res["psbt"])["psbt"]
        finalized = self.online.finalizepsbt(signed_psbt)["hex"]
        self.online.sendrawtransaction(finalized)

        self.generate(self.online, 1, sync_fun=self.no_op)
        offline_wallet.unloadwallet()
        online_wallet.unloadwallet()

    def test_avoid_reuse(self):
        self.log.info("Test that the avoid reuse flag appears in the exported wallet")
        self.offline.createwallet(wallet_name="avoidreuse", avoid_reuse=True)
        offline_wallet = self.offline.get_wallet_rpc("avoidreuse")
        assert_equal(offline_wallet.getwalletinfo()["avoid_reuse"], True)

        # The avoid_reuse flag also sets some specific address book entries to track reused addresses
        # In order for these to be set, a few transactions need to be made, so briefly connect offline to online
        self.connect_nodes(self.offline.index, self.online.index)
        addr = offline_wallet.getnewaddress()
        self.funder.sendtoaddress(addr, 1)
        self.generate(self.online, 1)
        # Spend funds in order to mark addr as previously spent
        offline_wallet.sendall([self.funder.getnewaddress()])
        self.funder.sendtoaddress(addr, 1)
        self.generate(self.online, 1)
        assert_equal(offline_wallet.listunspent(addresses=[addr])[0]["reused"], True)
        self.disconnect_nodes(self.offline.index ,self.online.index)

        # Export the watchonly wallet file and load onto online node
        online_wallet = self.export_and_restore(offline_wallet, "avoidreuse_watchonly")

        # check avoid_reuse is still set
        assert_equal(online_wallet.getwalletinfo()["avoid_reuse"], True)
        assert_equal(online_wallet.listunspent(addresses=[addr])[0]["reused"], True)

        offline_wallet.unloadwallet()
        online_wallet.unloadwallet()

    def test_encrypted_wallet(self):
        self.log.info("Test that a watchonly wallet can be exported from a locked wallet")
        self.offline.createwallet(wallet_name="encrypted", passphrase="pass")
        offline_wallet = self.offline.get_wallet_rpc("encrypted")
        assert_equal(offline_wallet.getwalletinfo()["unlocked_until"], 0)

        # Export the watchonly wallet file and load onto online node
        online_wallet = self.export_and_restore(offline_wallet, "encrypted_watchonly")

        # watchonly wallet does not have encryption because it doesn't have private keys
        assert "unlocked_until" not in online_wallet.getwalletinfo()
        # But it still has all of the public descriptors
        assert_equal(offline_wallet.listdescriptors()["descriptors"], online_wallet.listdescriptors()["descriptors"])

        offline_wallet.unloadwallet()
        online_wallet.unloadwallet()

    def run_test(self):
        self.online = self.nodes[0]
        self.offline = self.nodes[1]
        self.funder = self.online.get_wallet_rpc(self.default_wallet_name)
        self.export_path = os.path.join(self.options.tmpdir, "exported_wallets")
        os.makedirs(self.export_path, exist_ok=True)

        # Mine some blocks, and verify disconnected
        self.generate(self.online, 101, sync_fun=self.no_op)
        assert_not_equal(self.online.getbestblockhash(), self.offline.getbestblockhash())
        assert_equal(self.online.getblockcount(), 101)
        assert_equal(self.offline.getblockcount(), 0)

        self.test_basic_export()
        self.test_export_with_address_book()
        self.test_export_with_txs_and_locked_coins()
        self.test_export_imported_descriptors()
        self.test_avoid_reuse()
        self.test_encrypted_wallet()

if __name__ == '__main__':
    WalletExportedWatchOnly(__file__).main()
