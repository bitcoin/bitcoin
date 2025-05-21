#!/usr/bin/env python3

from test_framework.descriptors import descsum_create
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_approx,
    assert_equal,
    assert_raises_rpc_error,
)


class SilentPaymentsReceivingTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 2

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def test_encrypt_and_decrypt(self):
        self.log.info("Check that a silent payments wallet can be encrypted and decrypted")
        self.log.info("Create encrypted wallet")
        self.nodes[0].createwallet(wallet_name="sp_encrypted", passphrase="unsigned integer", silent_payments=True)
        wallet = self.nodes[0].get_wallet_rpc("sp_encrypted")
        addr = wallet.getnewaddress(address_type="silent-payments")
        self.def_wallet.sendtoaddress(addr, 10)
        self.generate(self.nodes[0], 1)
        self.log.info("Check that we can scan without the wallet being unlocked")
        assert_equal(wallet.getbalance(), 10)
        self.log.info("Check that we get an error if trying to send with the wallet locked")
        assert_raises_rpc_error(-13, "Error: Please enter the wallet passphrase with walletpassphrase first.", wallet.sendtoaddress, addr, 9)
        wallet.walletpassphrase(passphrase="unsigned integer", timeout=3)
        self.log.info("Unlock wallet and send")
        wallet.sendtoaddress(addr, 9)
        self.generate(self.nodes[0], 1)
        assert_approx(wallet.getbalance(), 10, 0.0001)

    def test_encrypting_unencrypted(self):
        self.log.info("Check that a silent payments wallet can be encrypted after creation")
        self.log.info("Create un-encrypted wallet")
        self.nodes[0].createwallet(wallet_name="sp_unencrypted", silent_payments=True)
        wallet = self.nodes[0].get_wallet_rpc("sp_unencrypted")
        addr = wallet.getnewaddress(address_type="silent-payments")
        self.def_wallet.sendtoaddress(addr, 10)
        self.generate(self.nodes[0], 1)
        assert_equal(wallet.getbalance(), 10)
        self.log.info("Add a passphrase to the wallet")
        wallet.encryptwallet(passphrase="unsigned integer")
        self.log.info("Check that we get an error if trying to send with the wallet locked")
        assert_raises_rpc_error(-13, "Error: Please enter the wallet passphrase with walletpassphrase first.", wallet.sendtoaddress, addr, 9)
        wallet.walletpassphrase(passphrase="unsigned integer", timeout=3)
        self.log.info("Unlock wallet and send")
        wallet.sendtoaddress(addr, 9)
        self.generate(self.nodes[0], 1)
        assert_approx(wallet.getbalance(), 10, 0.0001)

    def test_createwallet(self):
        self.log.info("Check createwallet silent payments option")

        self.nodes[0].createwallet(wallet_name="sp", silent_payments=True)
        wallet = self.nodes[0].get_wallet_rpc("sp")
        addr = wallet.getnewaddress(address_type="silent-payments")
        assert addr.startswith("sp")

        self.nodes[0].createwallet(wallet_name="non_sp", silent_payments=False)
        wallet = self.nodes[0].get_wallet_rpc("non_sp")
        assert_raises_rpc_error(-12, "Error: No silent-payments addresses available", wallet.getnewaddress, address_type="silent-payments")

    def test_basic(self):
        self.log.info("Basic receive and send")

        self.nodes[0].createwallet(wallet_name="basic", silent_payments=True)
        wallet = self.nodes[0].get_wallet_rpc("basic")

        addr = wallet.getnewaddress(address_type="silent-payments")
        addr_again = wallet.getnewaddress(address_type="silent-payments")
        assert addr == addr_again
        txid = self.def_wallet.sendtoaddress(addr, 10)
        self.generate(self.nodes[0], 1)

        assert_equal(wallet.getbalance(), 10)
        wallet.gettransaction(txid)

        self.log.info("Test change address")
        change_addr = wallet.getrawchangeaddress(address_type="silent-payments")
        assert addr.startswith("sp")
        txid = self.def_wallet.sendtoaddress(change_addr, 5)
        self.generate(self.nodes[0], 1)
        assert_approx(wallet.getbalance(), 15)
        wallet.gettransaction(txid)

        self.log.info("Test self-transfer")
        txid = wallet.send({addr: 5, change_addr: 5})
        self.generate(self.nodes[0], 1)
        assert_approx(wallet.getbalance(), 15, 0.0001)

        wallet.sendall([self.def_wallet.getnewaddress()])
        self.generate(self.nodes[0], 1)

        assert_equal(wallet.getbalance(), 0)

    def test_wallet_persistence(self):
        self.log.info("Test silent payments wallet persistence after closing and reopening")

        self.nodes[0].createwallet(wallet_name="persistence_test", silent_payments=True)
        wallet = self.nodes[0].get_wallet_rpc("persistence_test")
        addr = wallet.getnewaddress(address_type="silent-payments")
        send_amount = 15
        txid = self.def_wallet.sendtoaddress(addr, send_amount)
        self.generate(self.nodes[0], 1)

        # verify the wallet received the correct amount
        assert_equal(wallet.getbalance(), send_amount)
        tx = wallet.gettransaction(txid)
        self.nodes[0].unloadwallet("persistence_test")

        self.nodes[0].loadwallet("persistence_test")
        wallet = self.nodes[0].get_wallet_rpc("persistence_test")
        assert_equal(wallet.getbalance(), send_amount)
        loaded_tx = wallet.gettransaction(txid)
        assert_equal(tx, loaded_tx)

        self.disconnect_nodes(0, 1)
        txid = self.def_wallet.sendtoaddress(addr, send_amount)
        raw_tx = self.nodes[0].getrawtransaction(txid)
        def do_nothing():
            pass
        self.nodes[1].sendrawtransaction(raw_tx)
        self.generate(self.nodes[1], 1, sync_fun=do_nothing)
        self.nodes[0].unloadwallet("persistence_test")
        self.nodes[0].loadwallet("persistence_test")
        self.connect_nodes(0, 1)
        self.sync_blocks()
        wallet = self.nodes[0].get_wallet_rpc("persistence_test")
        assert_equal(wallet.getbalance(), send_amount * 2)

        self.log.info("Wallet persistence verified successfully")

    def test_import_rescan(self):
        self.log.info("Check import rescan works for silent payments")

        self.nodes[0].createwallet(wallet_name="alice", silent_payments=True)
        self.nodes[0].createwallet(wallet_name="alice_wo", disable_private_keys=True, silent_payments=True)
        alice = self.nodes[0].get_wallet_rpc("alice")
        alice_wo = self.nodes[0].get_wallet_rpc("alice_wo")

        address = alice.getnewaddress(address_type="silent-payments")
        self.def_wallet.sendtoaddress(address, 10)
        blockhash = self.generate(self.nodes[0], 1)[0]
        timestamp = self.nodes[0].getblockheader(blockhash)["time"]
        assert_approx(alice.getbalance(), 10, 0.0001)
        assert_equal(alice_wo.getbalance(), 0)

        alice_sp_desc = [d["desc"] for d in alice.listdescriptors()["descriptors"] if d["desc"].startswith("sp(")][0]
        res = alice_wo.importdescriptors([{
            "desc": alice_sp_desc,
            "active": True,
            "next_index": 0,
            "timestamp": timestamp
        }])
        assert_equal(res[0]["success"], True)

        assert_approx(alice_wo.getbalance(), 10, 0.0001)

        self.log.info("Check descriptor update works for silent payments")
        # Import the same descriptor again to test the update
        res = alice_wo.importdescriptors([{
            "desc": alice_sp_desc,
            "active": True,
            "next_index": 0,
            "timestamp": timestamp
        }])
        assert_equal(res[0]["success"], True)

        assert_approx(alice_wo.getbalance(), 10, 0.0001)

    def test_rbf(self):
        self.log.info("Check bumpfee on tx with sp outputs")

        self.nodes[0].createwallet(wallet_name="craig", silent_payments=True)
        wallet = self.nodes[0].get_wallet_rpc("craig")
        address = wallet.getnewaddress(address_type="silent-payments")

        txid = self.def_wallet.sendtoaddress(address, 49.99, replaceable=True)
        assert_equal(self.nodes[0].getrawmempool(), [txid])
        bumped_txid = self.def_wallet.bumpfee(txid, fee_rate=10000)["txid"]
        assert_equal(self.nodes[0].getrawmempool(), [bumped_txid])

        assert_equal(wallet.getbalance(), 0)
        self.generate(self.nodes[0], 1)
        assert_approx(wallet.getbalance(), 49.99, 0.0001)

        self.log.info("Check bumpfee on tx with sp inputs")

        address = self.def_wallet.getnewaddress()
        txid = wallet.sendtoaddress(address, 10, replaceable=True)
        assert_equal(self.nodes[0].getrawmempool(), [txid])
        bumped_txid = wallet.bumpfee(txid, fee_rate=10000)["txid"]
        assert_equal(self.nodes[0].getrawmempool(), [bumped_txid])

        self.generate(self.nodes[0], 1)
        assert_approx(wallet.getbalance(), 39.9, 0.1)

    def test_createwallet_descriptor(self):
        self.log.info("Check createwalletdescriptor works with silent payments descriptor")

        self.nodes[0].createwallet(wallet_name="sp_desc", silent_payments=True,)
        wallet = self.nodes[0].get_wallet_rpc("sp_desc")
        xpub_info = wallet.gethdkeys(private=True)
        xprv = xpub_info[0]["xprv"]
        expected_descs = []
        for desc in wallet.listdescriptors(private=True)["descriptors"]:
            if desc["desc"].startswith("sp("):
                expected_descs.append(desc["desc"])

        self.nodes[0].createwallet("blank", blank=True)
        blank_wallet = self.nodes[0].get_wallet_rpc("blank")

        # Import one active descriptor
        assert_equal(blank_wallet.importdescriptors([{"desc": descsum_create(f"pkh({xprv}/44h/2h/0h/0/0/*)"), "timestamp": "now", "active": True}])[0]["success"], True)
        assert_equal(len(blank_wallet.listdescriptors()["descriptors"]), 1)
        assert_equal(len(blank_wallet.gethdkeys()), 1)

        blank_wallet.createwalletdescriptor(type="silent-payments", internal=False)
        new_descs = [d for d in blank_wallet.listdescriptors(private=True)["descriptors"] if d["desc"].startswith("sp(")]
        assert_equal([d['desc'] for d in new_descs], expected_descs)
        for desc in new_descs:
            assert_equal(desc["active"], True)
            # Silent Payments descriptors are both internal and external
            # The wallet only checks if a descriptor is internal
            # because it does not expect a descriptor to be both internal and external
            # Hence, this flag will be 'True'
            assert_equal(desc["internal"], True)

    def test_backup_and_restore(self):
        self.log.info("Check backup and restore silent payments wallet")

        self.nodes[0].createwallet(wallet_name="sp_backup", silent_payments=True)
        wallet = self.nodes[0].get_wallet_rpc("sp_backup")
        addr = wallet.getnewaddress(address_type="silent-payments")
        self.def_wallet.sendtoaddress(addr, 10)
        self.generate(self.nodes[0], 1)

        # Backup the wallet
        backup_file = self.nodes[0].datadir_path / "sp_backup.bak"
        wallet.backupwallet(str(backup_file))

        self.nodes[1].restorewallet(
            wallet_name="sp_backup_restored",
            backup_file=str(backup_file)
        )
        restored_wallet = self.nodes[1].get_wallet_rpc("sp_backup_restored")
        assert_equal(restored_wallet.getbalance(), 10)

    def run_test(self):
        self.def_wallet = self.nodes[0].get_wallet_rpc(self.default_wallet_name)
        self.generate(self.nodes[0], 102)

        self.test_rbf()
        self.test_createwallet()
        self.test_encrypt_and_decrypt()
        self.test_encrypting_unencrypted()
        self.test_basic()
        self.test_wallet_persistence()
        self.test_import_rescan()
        self.test_createwallet_descriptor()
        self.test_backup_and_restore()



if __name__ == '__main__':
    SilentPaymentsReceivingTest(__file__).main()
