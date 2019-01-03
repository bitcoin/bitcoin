#!/usr/bin/env python3
# Copyright (c) 2016-2018 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test Hierarchical Deterministic wallet function."""

import os
import shutil

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    connect_nodes_bi,
    assert_raises_rpc_error
)


class WalletHDTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 2
        self.extra_args = [[], ['-keypool=0']]

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        # Make sure we use hd, keep masterkeyid
        masterkeyid = self.nodes[1].getwalletinfo()['hdseedid']
        assert_equal(masterkeyid, self.nodes[1].getwalletinfo()['hdmasterkeyid'])
        assert_equal(len(masterkeyid), 40)

        # create an internal key
        change_addr = self.nodes[1].getrawchangeaddress()
        change_addrV= self.nodes[1].getaddressinfo(change_addr)
        assert_equal(change_addrV["hdkeypath"], "m/0'/1'/0'") #first internal child key

        # Import a non-HD private key in the HD wallet
        non_hd_add = self.nodes[0].getnewaddress()
        self.nodes[1].importprivkey(self.nodes[0].dumpprivkey(non_hd_add))

        # This should be enough to keep the master key and the non-HD key
        self.nodes[1].backupwallet(os.path.join(self.nodes[1].datadir, "hd.bak"))
        #self.nodes[1].dumpwallet(os.path.join(self.nodes[1].datadir, "hd.dump"))

        # Derive some HD addresses and remember the last
        # Also send funds to each add
        self.nodes[0].generate(101)
        hd_add = None
        NUM_HD_ADDS = 10
        for i in range(NUM_HD_ADDS):
            hd_add = self.nodes[1].getnewaddress()
            hd_info = self.nodes[1].getaddressinfo(hd_add)
            assert_equal(hd_info["hdkeypath"], "m/0'/0'/"+str(i)+"'")
            assert_equal(hd_info["hdseedid"], masterkeyid)
            assert_equal(hd_info["hdmasterkeyid"], masterkeyid)
            self.nodes[0].sendtoaddress(hd_add, 1)
            self.nodes[0].generate(1)
        self.nodes[0].sendtoaddress(non_hd_add, 1)
        self.nodes[0].generate(1)

        # create an internal key (again)
        change_addr = self.nodes[1].getrawchangeaddress()
        change_addrV= self.nodes[1].getaddressinfo(change_addr)
        assert_equal(change_addrV["hdkeypath"], "m/0'/1'/1'") #second internal child key

        self.sync_all()
        assert_equal(self.nodes[1].getbalance(), NUM_HD_ADDS + 1)

        self.log.info("Restore backup ...")
        self.stop_node(1)
        # we need to delete the complete regtest directory
        # otherwise node1 would auto-recover all funds in flag the keypool keys as used
        shutil.rmtree(os.path.join(self.nodes[1].datadir, "regtest", "blocks"))
        shutil.rmtree(os.path.join(self.nodes[1].datadir, "regtest", "chainstate"))
        shutil.copyfile(os.path.join(self.nodes[1].datadir, "hd.bak"), os.path.join(self.nodes[1].datadir, "regtest", "wallets", "wallet.dat"))
        self.start_node(1)

        # Assert that derivation is deterministic
        hd_add_2 = None
        for i in range(NUM_HD_ADDS):
            hd_add_2 = self.nodes[1].getnewaddress()
            hd_info_2 = self.nodes[1].getaddressinfo(hd_add_2)
            assert_equal(hd_info_2["hdkeypath"], "m/0'/0'/"+str(i)+"'")
            assert_equal(hd_info_2["hdseedid"], masterkeyid)
            assert_equal(hd_info_2["hdmasterkeyid"], masterkeyid)
        assert_equal(hd_add, hd_add_2)
        connect_nodes_bi(self.nodes, 0, 1)
        self.sync_all()

        # Needs rescan
        self.stop_node(1)
        self.start_node(1, extra_args=self.extra_args[1] + ['-rescan'])
        assert_equal(self.nodes[1].getbalance(), NUM_HD_ADDS + 1)

        # Try a RPC based rescan
        self.stop_node(1)
        shutil.rmtree(os.path.join(self.nodes[1].datadir, "regtest", "blocks"))
        shutil.rmtree(os.path.join(self.nodes[1].datadir, "regtest", "chainstate"))
        shutil.copyfile(os.path.join(self.nodes[1].datadir, "hd.bak"), os.path.join(self.nodes[1].datadir, "regtest", "wallets", "wallet.dat"))
        self.start_node(1, extra_args=self.extra_args[1])
        connect_nodes_bi(self.nodes, 0, 1)
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
        outs = self.nodes[1].decoderawtransaction(self.nodes[1].gettransaction(txid)['hex'])['vout']
        keypath = ""
        for out in outs:
            if out['value'] != 1:
                keypath = self.nodes[1].getaddressinfo(out['scriptPubKey']['addresses'][0])['hdkeypath']

        assert_equal(keypath[0:7], "m/0'/1'")

        # Generate a new HD seed on node 1 and make sure it is set
        orig_masterkeyid = self.nodes[1].getwalletinfo()['hdseedid']
        self.nodes[1].sethdseed()
        new_masterkeyid = self.nodes[1].getwalletinfo()['hdseedid']
        assert orig_masterkeyid != new_masterkeyid
        addr = self.nodes[1].getnewaddress()
        assert_equal(self.nodes[1].getaddressinfo(addr)['hdkeypath'], 'm/0\'/0\'/0\'') # Make sure the new address is the first from the keypool
        self.nodes[1].keypoolrefill(1) # Fill keypool with 1 key

        # Set a new HD seed on node 1 without flushing the keypool
        new_seed = self.nodes[0].dumpprivkey(self.nodes[0].getnewaddress())
        orig_masterkeyid = new_masterkeyid
        self.nodes[1].sethdseed(False, new_seed)
        new_masterkeyid = self.nodes[1].getwalletinfo()['hdseedid']
        assert orig_masterkeyid != new_masterkeyid
        addr = self.nodes[1].getnewaddress()
        assert_equal(orig_masterkeyid, self.nodes[1].getaddressinfo(addr)['hdseedid'])
        assert_equal(self.nodes[1].getaddressinfo(addr)['hdkeypath'], 'm/0\'/0\'/1\'') # Make sure the new address continues previous keypool

        # Check that the next address is from the new seed
        self.nodes[1].keypoolrefill(1)
        next_addr = self.nodes[1].getnewaddress()
        assert_equal(new_masterkeyid, self.nodes[1].getaddressinfo(next_addr)['hdseedid'])
        assert_equal(self.nodes[1].getaddressinfo(next_addr)['hdkeypath'], 'm/0\'/0\'/0\'') # Make sure the new address is not from previous keypool
        assert next_addr != addr

        # Sethdseed parameter validity
        assert_raises_rpc_error(-1, 'sethdseed', self.nodes[0].sethdseed, False, new_seed, 0)
        assert_raises_rpc_error(-5, "Invalid private key", self.nodes[1].sethdseed, False, "not_wif")
        assert_raises_rpc_error(-1, "JSON value is not a boolean as expected", self.nodes[1].sethdseed, "Not_bool")
        assert_raises_rpc_error(-1, "JSON value is not a string as expected", self.nodes[1].sethdseed, False, True)
        assert_raises_rpc_error(-5, "Already have this key", self.nodes[1].sethdseed, False, new_seed)
        assert_raises_rpc_error(-5, "Already have this key", self.nodes[1].sethdseed, False, self.nodes[1].dumpprivkey(self.nodes[1].getnewaddress()))

if __name__ == '__main__':
    WalletHDTest().main ()
