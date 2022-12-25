#!/usr/bin/env python3
# Copyright (c) 2016-2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test Hierarchical Deterministic wallet function."""

import os
import shutil

from test_framework.blocktools import COINBASE_MATURITY
from test_framework.test_framework import SyscoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_raises_rpc_error,
)


class WalletHDTest(SyscoinTestFramework):
    def add_options(self, parser):
        self.add_wallet_options(parser)

    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 2
        self.extra_args = [[], ['-keypool=0']]
        # whitelist peers to speed up tx relay / mempool sync
        for args in self.extra_args:
            args.append("-whitelist=noban@127.0.0.1")

        self.supports_cli = False

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        # Make sure we use hd, keep masterkeyid
        hd_fingerprint = self.nodes[1].getaddressinfo(self.nodes[1].getnewaddress())['hdmasterfingerprint']
        assert_equal(len(hd_fingerprint), 8)

        # create an internal key
        change_addr = self.nodes[1].getrawchangeaddress()
        change_addrV = self.nodes[1].getaddressinfo(change_addr)
        if self.options.descriptors:
            assert_equal(change_addrV["hdkeypath"], "m/84'/1'/0'/1/0")
        else:
            assert_equal(change_addrV["hdkeypath"], "m/0'/1'/0'")  #first internal child key

        # Import a non-HD private key in the HD wallet
        non_hd_add = 'bcrt1qmevj8zfx0wdvp05cqwkmr6mxkfx60yezwjksmt'
        non_hd_key = 'cS9umN9w6cDMuRVYdbkfE4c7YUFLJRoXMfhQ569uY4odiQbVN8Rt'
        self.nodes[1].importprivkey(non_hd_key)

        # This should be enough to keep the master key and the non-HD key
        self.nodes[1].backupwallet(os.path.join(self.nodes[1].datadir, "hd.bak"))
        #self.nodes[1].dumpwallet(os.path.join(self.nodes[1].datadir, "hd.dump"))

        # Derive some HD addresses and remember the last
        # Also send funds to each add
        self.generate(self.nodes[0], COINBASE_MATURITY + 1)
        hd_add = None
        NUM_HD_ADDS = 10
        for i in range(1, NUM_HD_ADDS + 1):
            hd_add = self.nodes[1].getnewaddress()
            hd_info = self.nodes[1].getaddressinfo(hd_add)
            if self.options.descriptors:
                assert_equal(hd_info["hdkeypath"], "m/84'/1'/0'/0/" + str(i))
            else:
                assert_equal(hd_info["hdkeypath"], "m/0'/0'/" + str(i) + "'")
            assert_equal(hd_info["hdmasterfingerprint"], hd_fingerprint)
            self.nodes[0].sendtoaddress(hd_add, 1)
            self.generate(self.nodes[0], 1)
        self.nodes[0].sendtoaddress(non_hd_add, 1)
        self.generate(self.nodes[0], 1)

        # create an internal key (again)
        change_addr = self.nodes[1].getrawchangeaddress()
        change_addrV = self.nodes[1].getaddressinfo(change_addr)
        if self.options.descriptors:
            assert_equal(change_addrV["hdkeypath"], "m/84'/1'/0'/1/1")
        else:
            assert_equal(change_addrV["hdkeypath"], "m/0'/1'/1'")  #second internal child key

        self.sync_all()
        assert_equal(self.nodes[1].getbalance(), NUM_HD_ADDS + 1)

        self.log.info("Restore backup ...")
        self.stop_node(1)
        # we need to delete the complete chain directory
        # otherwise node1 would auto-recover all funds in flag the keypool keys as used
        shutil.rmtree(os.path.join(self.nodes[1].datadir, self.chain, "blocks"))
        shutil.rmtree(os.path.join(self.nodes[1].datadir, self.chain, "chainstate"))
        shutil.copyfile(
            os.path.join(self.nodes[1].datadir, "hd.bak"),
            os.path.join(self.nodes[1].datadir, self.chain, 'wallets', self.default_wallet_name, self.wallet_data_filename),
        )
        self.start_node(1)

        # Assert that derivation is deterministic
        hd_add_2 = None
        for i in range(1, NUM_HD_ADDS + 1):
            hd_add_2 = self.nodes[1].getnewaddress()
            hd_info_2 = self.nodes[1].getaddressinfo(hd_add_2)
            if self.options.descriptors:
                assert_equal(hd_info_2["hdkeypath"], "m/84'/1'/0'/0/" + str(i))
            else:
                assert_equal(hd_info_2["hdkeypath"], "m/0'/0'/" + str(i) + "'")
            assert_equal(hd_info_2["hdmasterfingerprint"], hd_fingerprint)
        assert_equal(hd_add, hd_add_2)
        self.connect_nodes(0, 1)
        self.sync_all()

        # Needs rescan
        self.nodes[1].rescanblockchain()
        assert_equal(self.nodes[1].getbalance(), NUM_HD_ADDS + 1)

        # Try a RPC based rescan
        self.stop_node(1)
        shutil.rmtree(os.path.join(self.nodes[1].datadir, self.chain, "blocks"))
        shutil.rmtree(os.path.join(self.nodes[1].datadir, self.chain, "chainstate"))
        shutil.copyfile(
            os.path.join(self.nodes[1].datadir, "hd.bak"),
            os.path.join(self.nodes[1].datadir, self.chain, "wallets", self.default_wallet_name, self.wallet_data_filename),
        )
        self.start_node(1, extra_args=self.extra_args[1])
        self.connect_nodes(0, 1)
        self.sync_all()
        # Wallet automatically scans blocks older than key on startup
        assert_equal(self.nodes[1].getbalance(), NUM_HD_ADDS + 1)
        out = self.nodes[1].rescanblockchain(0, 1)
        assert_equal(out['start_height'], 0)
        assert_equal(out['stop_height'], 1)
        out = self.nodes[1].rescanblockchain()
        assert_equal(out['start_height'], 0)
        assert_equal(out['stop_height'], self.nodes[1].getblockcount())
        assert_equal(self.nodes[1].getbalance(), NUM_HD_ADDS + 1)

        # send a tx and make sure its using the internal chain for the changeoutput
        txid = self.nodes[1].sendtoaddress(self.nodes[0].getnewaddress(), 1)
        outs = self.nodes[1].gettransaction(txid=txid, verbose=True)['decoded']['vout']
        keypath = ""
        for out in outs:
            if out['value'] != 1:
                keypath = self.nodes[1].getaddressinfo(out['scriptPubKey']['address'])['hdkeypath']

        if self.options.descriptors:
            assert_equal(keypath[0:14], "m/84'/1'/0'/1/")
        else:
            assert_equal(keypath[0:7], "m/0'/1'")

        if not self.options.descriptors:
            # Generate a new HD seed on node 1 and make sure it is set
            orig_masterkeyid = self.nodes[1].getwalletinfo()['hdseedid']
            self.nodes[1].sethdseed()
            new_masterkeyid = self.nodes[1].getwalletinfo()['hdseedid']
            assert orig_masterkeyid != new_masterkeyid
            addr = self.nodes[1].getnewaddress()
            # Make sure the new address is the first from the keypool
            assert_equal(self.nodes[1].getaddressinfo(addr)['hdkeypath'], 'm/0\'/0\'/0\'')
            self.nodes[1].keypoolrefill(1)  # Fill keypool with 1 key

            # Set a new HD seed on node 1 without flushing the keypool
            new_seed = self.nodes[0].dumpprivkey(self.nodes[0].getnewaddress())
            orig_masterkeyid = new_masterkeyid
            self.nodes[1].sethdseed(False, new_seed)
            new_masterkeyid = self.nodes[1].getwalletinfo()['hdseedid']
            assert orig_masterkeyid != new_masterkeyid
            addr = self.nodes[1].getnewaddress()
            assert_equal(orig_masterkeyid, self.nodes[1].getaddressinfo(addr)['hdseedid'])
            # Make sure the new address continues previous keypool
            assert_equal(self.nodes[1].getaddressinfo(addr)['hdkeypath'], 'm/0\'/0\'/1\'')

            # Check that the next address is from the new seed
            self.nodes[1].keypoolrefill(1)
            next_addr = self.nodes[1].getnewaddress()
            assert_equal(new_masterkeyid, self.nodes[1].getaddressinfo(next_addr)['hdseedid'])
            # Make sure the new address is not from previous keypool
            assert_equal(self.nodes[1].getaddressinfo(next_addr)['hdkeypath'], 'm/0\'/0\'/0\'')
            assert next_addr != addr

            # Sethdseed parameter validity
            assert_raises_rpc_error(-1, 'sethdseed', self.nodes[0].sethdseed, False, new_seed, 0)
            assert_raises_rpc_error(-5, "Invalid private key", self.nodes[1].sethdseed, False, "not_wif")
            assert_raises_rpc_error(-3, "JSON value of type string is not of expected type bool", self.nodes[1].sethdseed, "Not_bool")
            assert_raises_rpc_error(-3, "JSON value of type bool is not of expected type string", self.nodes[1].sethdseed, False, True)
            assert_raises_rpc_error(-5, "Already have this key", self.nodes[1].sethdseed, False, new_seed)
            assert_raises_rpc_error(-5, "Already have this key", self.nodes[1].sethdseed, False, self.nodes[1].dumpprivkey(self.nodes[1].getnewaddress()))

            self.log.info('Test sethdseed restoring with keys outside of the initial keypool')
            self.generate(self.nodes[0], 10)
            # Restart node 1 with keypool of 3 and a different wallet
            self.nodes[1].createwallet(wallet_name='origin', blank=True)
            self.restart_node(1, extra_args=['-keypool=3', '-wallet=origin'])
            self.connect_nodes(0, 1)

            # sethdseed restoring and seeing txs to addresses out of the keypool
            origin_rpc = self.nodes[1].get_wallet_rpc('origin')
            seed = self.nodes[0].dumpprivkey(self.nodes[0].getnewaddress())
            origin_rpc.sethdseed(True, seed)

            self.nodes[1].createwallet(wallet_name='restore', blank=True)
            restore_rpc = self.nodes[1].get_wallet_rpc('restore')
            restore_rpc.sethdseed(True, seed)  # Set to be the same seed as origin_rpc
            restore_rpc.sethdseed(True)  # Rotate to a new seed, making original `seed` inactive

            self.nodes[1].createwallet(wallet_name='restore2', blank=True)
            restore2_rpc = self.nodes[1].get_wallet_rpc('restore2')
            restore2_rpc.sethdseed(True, seed)  # Set to be the same seed as origin_rpc
            restore2_rpc.sethdseed(True)  # Rotate to a new seed, making original `seed` inactive

            # Check persistence of inactive seed by reloading restore. restore2 is still loaded to test the case where the wallet is not reloaded
            restore_rpc.unloadwallet()
            self.nodes[1].loadwallet('restore')
            restore_rpc = self.nodes[1].get_wallet_rpc('restore')

            # Empty origin keypool and get an address that is beyond the initial keypool
            origin_rpc.getnewaddress()
            origin_rpc.getnewaddress()
            last_addr = origin_rpc.getnewaddress()  # Last address of initial keypool
            addr = origin_rpc.getnewaddress()  # First address beyond initial keypool

            # Check that the restored seed has last_addr but does not have addr
            info = restore_rpc.getaddressinfo(last_addr)
            assert_equal(info['ismine'], True)
            info = restore_rpc.getaddressinfo(addr)
            assert_equal(info['ismine'], False)
            info = restore2_rpc.getaddressinfo(last_addr)
            assert_equal(info['ismine'], True)
            info = restore2_rpc.getaddressinfo(addr)
            assert_equal(info['ismine'], False)
            # Check that the origin seed has addr
            info = origin_rpc.getaddressinfo(addr)
            assert_equal(info['ismine'], True)

            # Send a transaction to addr, which is out of the initial keypool.
            # The wallet that has set a new seed (restore_rpc) should not detect this transaction.
            txid = self.nodes[0].sendtoaddress(addr, 1)
            origin_rpc.sendrawtransaction(self.nodes[0].gettransaction(txid)['hex'])
            self.generate(self.nodes[0], 1)
            origin_rpc.gettransaction(txid)
            assert_raises_rpc_error(-5, 'Invalid or non-wallet transaction id', restore_rpc.gettransaction, txid)
            out_of_kp_txid = txid

            # Send a transaction to last_addr, which is in the initial keypool.
            # The wallet that has set a new seed (restore_rpc) should detect this transaction and generate 3 new keys from the initial seed.
            # The previous transaction (out_of_kp_txid) should still not be detected as a rescan is required.
            txid = self.nodes[0].sendtoaddress(last_addr, 1)
            origin_rpc.sendrawtransaction(self.nodes[0].gettransaction(txid)['hex'])
            self.generate(self.nodes[0], 1)
            origin_rpc.gettransaction(txid)
            restore_rpc.gettransaction(txid)
            assert_raises_rpc_error(-5, 'Invalid or non-wallet transaction id', restore_rpc.gettransaction, out_of_kp_txid)
            restore2_rpc.gettransaction(txid)
            assert_raises_rpc_error(-5, 'Invalid or non-wallet transaction id', restore2_rpc.gettransaction, out_of_kp_txid)

            # After rescanning, restore_rpc should now see out_of_kp_txid and generate an additional key.
            # addr should now be part of restore_rpc and be ismine
            restore_rpc.rescanblockchain()
            restore_rpc.gettransaction(out_of_kp_txid)
            info = restore_rpc.getaddressinfo(addr)
            assert_equal(info['ismine'], True)
            restore2_rpc.rescanblockchain()
            restore2_rpc.gettransaction(out_of_kp_txid)
            info = restore2_rpc.getaddressinfo(addr)
            assert_equal(info['ismine'], True)

            # Check again that 3 keys were derived.
            # Empty keypool and get an address that is beyond the initial keypool
            origin_rpc.getnewaddress()
            origin_rpc.getnewaddress()
            last_addr = origin_rpc.getnewaddress()
            addr = origin_rpc.getnewaddress()

            # Check that the restored seed has last_addr but does not have addr
            info = restore_rpc.getaddressinfo(last_addr)
            assert_equal(info['ismine'], True)
            info = restore_rpc.getaddressinfo(addr)
            assert_equal(info['ismine'], False)
            info = restore2_rpc.getaddressinfo(last_addr)
            assert_equal(info['ismine'], True)
            info = restore2_rpc.getaddressinfo(addr)
            assert_equal(info['ismine'], False)


if __name__ == '__main__':
    WalletHDTest().main()
