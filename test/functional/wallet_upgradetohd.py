#!/usr/bin/env python3
# Copyright (c) 2016-2025 The Dash Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""
wallet_upgradetohd.py

Test upgrade to a Hierarchical Deterministic wallet via upgradetohd rpc
"""

import shutil
import os

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_raises_rpc_error,
    get_mnemonic,
)


class WalletUpgradeToHDTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.extra_args = [['-usehd=0']]

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def setup_network(self):
        self.add_nodes(self.num_nodes, self.extra_args)
        self.start_nodes()
        self.nodes[0].createwallet(self.default_wallet_name, blank=True, load_on_startup=True)
        self.nodes[0].importprivkey(privkey=self.nodes[0].get_deterministic_priv_key().key, label='coinbase', rescan=True)

    def recover_non_hd(self):
        self.log.info("Recover non-HD wallet to check different upgrade paths")
        node = self.nodes[0]
        self.stop_node(0)
        shutil.copyfile(os.path.join(node.datadir, "non_hd.bak"), os.path.join(node.datadir, self.chain, self.default_wallet_name, self.wallet_data_filename))
        self.start_node(0)
        if not self.options.descriptors:
            assert 'hdchainid' not in node.getwalletinfo()

    def run_test(self):
        node = self.nodes[0]
        node.backupwallet(os.path.join(node.datadir, "non_hd.bak"))

        self.log.info("No mnemonic, no mnemonic passphrase, no wallet passphrase")
        assert 'hdchainid' not in node.getwalletinfo()
        balance_before = node.getbalance()
        assert node.upgradetohd()
        mnemonic = get_mnemonic(node)
        if not self.options.descriptors:
            chainid = node.getwalletinfo()['hdchainid']
            assert_equal(len(chainid), 64)
        assert_equal(balance_before, node.getbalance())

        self.log.info("Should be spendable and should use correct paths")
        for i in range(5):
            txid = node.sendtoaddress(node.getnewaddress(), 1)
            outs = node.decoderawtransaction(node.gettransaction(txid)['hex'])['vout']
            for out in outs:
                if out['value'] == 1:
                    keypath = node.getaddressinfo(out['scriptPubKey']['address'])['hdkeypath']
                    assert_equal(keypath, "m/44'/1'/0'/0/%d" % i)
                else:
                    keypath = node.getaddressinfo(out['scriptPubKey']['address'])['hdkeypath']
                    assert_equal(keypath, "m/44'/1'/0'/1/%d" % i)

        self.bump_mocktime(1)
        self.generate(node, 1, sync_fun=self.no_op)

        self.log.info("Should no longer be able to start it with HD disabled")
        self.stop_node(0)
        node.assert_start_raises_init_error(['-usehd=0'], "Error: Error loading %s: You can't disable HD on an already existing HD wallet" % self.default_wallet_name)
        self.extra_args = []
        self.start_node(0, [])
        balance_after = node.getbalance()

        self.recover_non_hd()

        # We spent some coins from non-HD keys to HD ones earlier
        balance_non_HD = node.getbalance()
        assert balance_before != balance_non_HD

        self.log.info("No mnemonic, no mnemonic passphrase, no wallet passphrase, should result in completely different keys")
        assert node.upgradetohd()
        assert mnemonic != get_mnemonic(node)
        if not self.options.descriptors:
            assert chainid != node.getwalletinfo()['hdchainid']
        assert_equal(balance_non_HD, node.getbalance())
        node.keypoolrefill(5)
        node.rescanblockchain()
        # Completely different keys, no HD coins should be recovered
        assert_equal(balance_non_HD, node.getbalance())

        self.recover_non_hd()

        self.log.info("No mnemonic, no mnemonic passphrase, no wallet passphrase, should result in completely different keys")
        self.restart_node(0, extra_args=['-keypool=10'])
        assert node.upgradetohd("", "", "", True)
        # Completely different keys, no HD coins should be recovered
        assert mnemonic != get_mnemonic(node)
        if not self.options.descriptors:
            assert chainid != node.getwalletinfo()['hdchainid']
        assert_equal(balance_non_HD, node.getbalance())

        self.recover_non_hd()

        self.log.info("Same mnemonic, another mnemonic passphrase, no wallet passphrase, should result in a different set of keys")
        new_mnemonic_passphrase = "somewords"
        assert node.upgradetohd(mnemonic[0], new_mnemonic_passphrase)
        assert_equal(mnemonic[0], get_mnemonic(node)[0])
        assert_equal(new_mnemonic_passphrase, get_mnemonic(node)[1])
        if not self.options.descriptors:
            new_chainid = node.getwalletinfo()['hdchainid']
            assert chainid != new_chainid
        assert_equal(balance_non_HD, node.getbalance())
        node.keypoolrefill(5)
        node.rescanblockchain()
        # A different set of keys, no HD coins should be recovered
        new_addresses = (node.getnewaddress(), node.getrawchangeaddress())
        assert_equal(balance_non_HD, node.getbalance())

        self.recover_non_hd()

        self.log.info("Same mnemonic, another mnemonic passphrase, no wallet passphrase, should result in a different set of keys (again)")
        assert node.upgradetohd(mnemonic[0], new_mnemonic_passphrase)
        assert_equal(mnemonic[0], get_mnemonic(node)[0])
        assert_equal(new_mnemonic_passphrase, get_mnemonic(node)[1])
        if not self.options.descriptors:
            assert_equal(new_chainid, node.getwalletinfo()['hdchainid'])
        assert_equal(balance_non_HD, node.getbalance())
        node.keypoolrefill(5)
        node.rescanblockchain()
        # A different set of keys, no HD coins should be recovered, keys should be the same as they were the previous time
        assert_equal(new_addresses, (node.getnewaddress(), node.getrawchangeaddress()))
        assert_equal(balance_non_HD, node.getbalance())

        self.recover_non_hd()

        self.log.info("Same mnemonic, no mnemonic passphrase, no wallet passphrase, should recover all coins after rescan")
        assert node.upgradetohd(mnemonic[0], mnemonic[1])
        assert_equal(mnemonic, get_mnemonic(node))
        if not self.options.descriptors:
            assert_equal(chainid, node.getwalletinfo()['hdchainid'])
        node.keypoolrefill(5)
        assert balance_after != node.getbalance()
        node.rescanblockchain()
        assert_equal(balance_after, node.getbalance())

        self.recover_non_hd()

        self.log.info("Same mnemonic, no mnemonic passphrase, no wallet passphrase, large enough keepool, should recover all coins with no extra rescan")
        self.restart_node(0, extra_args=['-keypool=10'])
        assert node.upgradetohd(mnemonic[0], mnemonic[1])
        assert_equal(mnemonic, get_mnemonic(node))
        if not self.options.descriptors:
            assert_equal(chainid, node.getwalletinfo()['hdchainid'])
        # All coins should be recovered
        assert_equal(balance_after, node.getbalance())

        self.recover_non_hd()

        self.log.info("Same mnemonic, no mnemonic passphrase, no wallet passphrase, large enough keepool, rescan is skipped initially, should recover all coins after rescanblockchain")
        self.restart_node(0, extra_args=['-keypool=10'])
        assert node.upgradetohd(mnemonic[0], mnemonic[1], "", False)
        assert_equal(mnemonic, get_mnemonic(node))
        if not self.options.descriptors:
            assert_equal(chainid, node.getwalletinfo()['hdchainid'])
        assert balance_after != node.getbalance()
        node.rescanblockchain()
        # All coins should be recovered
        assert_equal(balance_after, node.getbalance())

        self.recover_non_hd()

        self.log.info("Same mnemonic, same mnemonic passphrase, encrypt wallet on upgrade, should recover all coins after rescan")
        walletpass = "111pass222"
        assert node.upgradetohd(mnemonic[0], "", walletpass)
        node.stop()
        node.wait_until_stopped()
        self.start_node(0, extra_args=['-rescan'])
        assert_raises_rpc_error(-13, "Error: Please enter the wallet passphrase with walletpassphrase first.", node.dumphdinfo)
        node.walletpassphrase(walletpass, 100)
        assert_equal(mnemonic, get_mnemonic(node))
        if not self.options.descriptors:
            assert_equal(chainid, node.getwalletinfo()['hdchainid'])
        # Note: wallet encryption results in additional keypool topup,
        # so we can't compare new balance to balance_non_HD here,
        # assert_equal(balance_non_HD, node.getbalance())  # won't work
        assert balance_non_HD != node.getbalance()
        node.keypoolrefill(4)
        node.rescanblockchain()
        # All coins should be recovered
        assert_equal(balance_after, node.getbalance())

        self.recover_non_hd()

        self.log.info("Same mnemonic, same mnemonic passphrase, encrypt wallet first, should recover all coins on upgrade after rescan")
        # Null characters are allowed in wallet passphrases since v23
        walletpass = "111\0pass222"
        node.encryptwallet(walletpass)
        node.stop()
        node.wait_until_stopped()
        self.start_node(0, extra_args=['-rescan'])
        assert_raises_rpc_error(-13, "Error: Wallet encrypted but passphrase not supplied to RPC.", node.upgradetohd, mnemonic[0])
        assert_raises_rpc_error(-14, "Error: The wallet passphrase entered was incorrect", node.upgradetohd, mnemonic[0], "", "111")
        assert node.upgradetohd(mnemonic[0], "", walletpass)
        if not self.options.descriptors:
            assert_raises_rpc_error(-13, "Error: Please enter the wallet passphrase with walletpassphrase first.", node.dumphdinfo)
        else:
            assert_raises_rpc_error(-13, "Error: Please enter the wallet passphrase with walletpassphrase first.", node.listdescriptors, True)
        node.walletpassphrase(walletpass, 100)
        assert_equal(mnemonic, get_mnemonic(node))
        if not self.options.descriptors:
            assert_equal(chainid, node.getwalletinfo()['hdchainid'])
        # Note: wallet encryption results in additional keypool topup,
        # so we can't compare new balance to balance_non_HD here,
        # assert_equal(balance_non_HD, node.getbalance())  # won't work
        assert balance_non_HD != node.getbalance()
        node.keypoolrefill(4)
        node.rescanblockchain()
        # All coins should be recovered
        assert_equal(balance_after, node.getbalance())

        self.log.info("Test upgradetohd with user defined mnemonic")
        custom_mnemonic = "similar behave slot swim scissors throw planet view ghost laugh drift calm"
        # this address belongs to custom mnemonic with no passphrase
        custom_address_1 = "yLpq97zZUsFQ2rdMqhcPKkYT36MoPK4Hob"
        # this address belongs to custom mnemonic with passphrase "custom-passphrase"
        custom_address_2 = "yYBPeZQcqgQHu9dxA5pKBWtYbK2hwfFHxf"
        node.sendtoaddress(custom_address_1, 11)
        node.sendtoaddress(custom_address_2, 12)
        self.generate(node, 1)

        node.createwallet("wallet-11", blank=True)
        w11 = node.get_wallet_rpc("wallet-11")
        w11.upgradetohd(custom_mnemonic)
        assert_equal(11, w11.getbalance())
        w11.unloadwallet()

        node.createwallet("wallet-12", blank=True)
        w12 = node.get_wallet_rpc("wallet-12")
        ret = w12.upgradetohd(custom_mnemonic, "custom-passphrase")
        assert_equal(ret, "Make sure that you have backup of your mnemonic.")
        assert_equal(get_mnemonic(w12)[0], custom_mnemonic)
        assert_equal(get_mnemonic(w12)[1], "custom-passphrase")
        assert_equal(12, w12.getbalance())
        w12.unloadwallet()

        self.log.info("Check if null character at the end of mnemonic-passphrase matters")
        node.createwallet("wallet-null", blank=True)
        w_null = node.get_wallet_rpc("wallet-null")
        ret = w_null.upgradetohd(custom_mnemonic, "custom-passphrase\0")
        assert_equal(ret, "Make sure that you have backup of your mnemonic. Your mnemonic passphrase contains a null character (ie - a zero byte). If the passphrase was created with a version of this software prior to 23.0, please try again with only the characters up to — but not including — the first null character. If this is successful, please set a new passphrase to avoid this issue in the future.")
        assert_equal(0, w_null.getbalance())
        assert_equal(get_mnemonic(w_null)[1], "custom-passphrase\0")
        w_null.unloadwallet()


if __name__ == '__main__':
    WalletUpgradeToHDTest().main ()
