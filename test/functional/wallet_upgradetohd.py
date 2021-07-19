#!/usr/bin/env python3
# Copyright (c) 2016 The Bitcoin Core developers
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
)


class WalletUpgradeToHDTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1

    def setup_network(self):
        self.add_nodes(self.num_nodes)
        self.start_nodes()

    def recover_non_hd(self):
        self.log.info("Recover non-HD wallet to check different upgrade paths")
        node = self.nodes[0]
        self.stop_node(0)
        shutil.copyfile(os.path.join(node.datadir, "non_hd.bak"), os.path.join(node.datadir, "regtest", "wallets", "wallet.dat"))
        self.start_node(0)
        assert('hdchainid' not in node.getwalletinfo())

    def run_test(self):
        node = self.nodes[0]
        node.backupwallet(os.path.join(node.datadir, "non_hd.bak"))

        self.log.info("No mnemonic, no mnemonic passphrase, no wallet passphrase")
        assert('hdchainid' not in node.getwalletinfo())
        balance_before = node.getbalance()
        assert(node.upgradetohd())
        mnemonic = node.dumphdinfo()['mnemonic']
        chainid = node.getwalletinfo()['hdchainid']
        assert_equal(len(chainid), 64)
        assert_equal(balance_before, node.getbalance())

        self.log.info("Should be spendable and should use correct paths")
        for i in range(5):
            txid = node.sendtoaddress(node.getnewaddress(), 1)
            outs = node.decoderawtransaction(node.gettransaction(txid)['hex'])['vout']
            for out in outs:
                if out['value'] == 1:
                    keypath = node.getaddressinfo(out['scriptPubKey']['addresses'][0])['hdkeypath']
                    assert_equal(keypath, "m/44'/1'/0'/0/%d" % i)
                else:
                    keypath = node.getaddressinfo(out['scriptPubKey']['addresses'][0])['hdkeypath']
                    assert_equal(keypath, "m/44'/1'/0'/1/%d" % i)

        self.bump_mocktime(1)
        node.generate(1)

        self.log.info("Should no longer be able to start it with HD disabled")
        self.stop_node(0)
        node.assert_start_raises_init_error(['-usehd=0'], "Error: Error loading : You can't disable HD on an already existing HD wallet")
        self.start_node(0)
        balance_after = node.getbalance()

        self.recover_non_hd()

        # We spent some coins from non-HD keys to HD ones earlier
        balance_non_HD = node.getbalance()
        assert(balance_before != balance_non_HD)

        self.log.info("No mnemonic, no mnemonic passphrase, no wallet passphrase, should result in completely different keys")
        assert(node.upgradetohd())
        assert(mnemonic != node.dumphdinfo()['mnemonic'])
        assert(chainid != node.getwalletinfo()['hdchainid'])
        assert_equal(balance_non_HD, node.getbalance())
        node.keypoolrefill(5)
        node.rescanblockchain()
        # Completely different keys, no HD coins should be recovered
        assert_equal(balance_non_HD, node.getbalance())

        self.recover_non_hd()

        self.log.info("Same mnemonic, another mnemonic passphrase, no wallet passphrase, should result in a different set of keys")
        new_mnemonic_passphrase = "somewords"
        assert(node.upgradetohd(mnemonic, new_mnemonic_passphrase))
        assert_equal(mnemonic, node.dumphdinfo()['mnemonic'])
        new_chainid = node.getwalletinfo()['hdchainid']
        assert(chainid != new_chainid)
        assert_equal(balance_non_HD, node.getbalance())
        node.keypoolrefill(5)
        node.rescanblockchain()
        # A different set of keys, no HD coins should be recovered
        new_addresses = (node.getnewaddress(), node.getrawchangeaddress())
        assert_equal(balance_non_HD, node.getbalance())

        self.recover_non_hd()

        self.log.info("Same mnemonic, another mnemonic passphrase, no wallet passphrase, should result in a different set of keys (again)")
        assert(node.upgradetohd(mnemonic, new_mnemonic_passphrase))
        assert_equal(mnemonic, node.dumphdinfo()['mnemonic'])
        assert_equal(new_chainid, node.getwalletinfo()['hdchainid'])
        assert_equal(balance_non_HD, node.getbalance())
        node.keypoolrefill(5)
        node.rescanblockchain()
        # A different set of keys, no HD coins should be recovered, keys should be the same as they were the previous time
        assert_equal(new_addresses, (node.getnewaddress(), node.getrawchangeaddress()))
        assert_equal(balance_non_HD, node.getbalance())

        self.recover_non_hd()

        self.log.info("Same mnemonic, no mnemonic passphrase, no wallet passphrase, should recover all coins after rescan")
        assert(node.upgradetohd(mnemonic))
        assert_equal(mnemonic, node.dumphdinfo()['mnemonic'])
        assert_equal(chainid, node.getwalletinfo()['hdchainid'])
        node.keypoolrefill(5)
        node.rescanblockchain()
        assert_equal(balance_after, node.getbalance())

        self.recover_non_hd()

        self.log.info("Same mnemonic, no mnemonic passphrase, no wallet passphrase, large enough keepool, should recover all coins with no extra rescan")
        self.stop_node(0)
        self.start_node(0, extra_args=['-keypool=10'])
        assert(node.upgradetohd(mnemonic))
        assert_equal(mnemonic, node.dumphdinfo()['mnemonic'])
        assert_equal(chainid, node.getwalletinfo()['hdchainid'])
        # All coins should be recovered
        assert_equal(balance_after, node.getbalance())

        self.recover_non_hd()

        self.log.info("Same mnemonic, same mnemonic passphrase, encrypt wallet on upgrade, should recover all coins after rescan")
        walletpass = "111pass222"
        assert node.upgradetohd(mnemonic, "", walletpass)
        node.stop()
        node.wait_until_stopped()
        self.start_node(0, extra_args=['-rescan'])
        assert_raises_rpc_error(-13, "Error: Please enter the wallet passphrase with walletpassphrase first.", node.dumphdinfo)
        node.walletpassphrase(walletpass, 100)
        assert_equal(mnemonic, node.dumphdinfo()['mnemonic'])
        assert_equal(chainid, node.getwalletinfo()['hdchainid'])
        # Note: wallet encryption results in additional keypool topup,
        # so we can't compare new balance to balance_non_HD here,
        # assert_equal(balance_non_HD, node.getbalance())  # won't work
        assert(balance_non_HD != node.getbalance())
        node.keypoolrefill(4)
        node.rescanblockchain()
        # All coins should be recovered
        assert_equal(balance_after, node.getbalance())

        self.recover_non_hd()

        self.log.info("Same mnemonic, same mnemonic passphrase, encrypt wallet first, should recover all coins on upgrade after rescan")
        walletpass = "111pass222"
        node.encryptwallet(walletpass)
        node.stop()
        node.wait_until_stopped()
        self.start_node(0, extra_args=['-rescan'])
        assert_raises_rpc_error(-14, "Cannot upgrade encrypted wallet to HD without the wallet passphrase", node.upgradetohd, mnemonic)
        assert_raises_rpc_error(-14, "The wallet passphrase entered was incorrect", node.upgradetohd, mnemonic, "", "wrongpass")
        assert(node.upgradetohd(mnemonic, "", walletpass))
        assert_raises_rpc_error(-13, "Error: Please enter the wallet passphrase with walletpassphrase first.", node.dumphdinfo)
        node.walletpassphrase(walletpass, 100)
        assert_equal(mnemonic, node.dumphdinfo()['mnemonic'])
        assert_equal(chainid, node.getwalletinfo()['hdchainid'])
        # Note: wallet encryption results in additional keypool topup,
        # so we can't compare new balance to balance_non_HD here,
        # assert_equal(balance_non_HD, node.getbalance())  # won't work
        assert(balance_non_HD != node.getbalance())
        node.keypoolrefill(4)
        node.rescanblockchain()
        # All coins should be recovered
        assert_equal(balance_after, node.getbalance())


if __name__ == '__main__':
    WalletUpgradeToHDTest().main ()
