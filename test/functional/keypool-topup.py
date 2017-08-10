#!/usr/bin/env python3
# Copyright (c) 2017 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test HD Wallet keypool restore function.

Two nodes. Node1 is under test. Node0 is providing transactions and generating blocks.

Repeat test twice, once with encrypted wallet and once with unencrypted wallet:

- Start node1, shutdown and backup wallet.
- Generate 110 keys (enough to drain the keypool). Store key 90 (in the initial keypool) and key 110 (beyond the initial keypool). Send funds to key 90 and key 110.
- Stop node1, clear the datadir, move wallet file back into the datadir and restart node1.
- if wallet is unencrypted:
    - connect node1 to node0. Verify that they sync and node1 receives its funds.
- else wallet is encrypted:
    - connect node1 to node0. Verify that node1 shuts down to avoid loss of funds.
"""
import os
import shutil

from test_framework.test_framework import BitcoinTestFramework, BITCOIND_PROC_WAIT_TIMEOUT
from test_framework.util import (
    assert_equal,
    assert_raises_jsonrpc,
    connect_nodes_bi,
    sync_blocks,
)

class KeypoolRestoreTest(BitcoinTestFramework):
    def __init__(self):
        super().__init__()
        self.setup_clean_chain = True
        self.num_nodes = 2
        self.extra_args = [['-usehd=0'], ['-usehd=1', '-keypool=100', '-keypoolmin=20', '-keypoolcritical=20']]

    def run_test(self):
        self.tmpdir = self.options.tmpdir
        self.nodes[0].generate(101)

        self.log.info("Test keypool restore (encrypted wallet)")
        self._test_restore(encrypted=True)

        self.log.info("Test keypool restore (unencrypted wallet)")
        self._test_restore(encrypted=False)

    def _test_restore(self, encrypted):

        self.log.info("Make backup of wallet")

        if encrypted:
            self.nodes[1].encryptwallet('test')
            self.bitcoind_processes[1].wait(timeout=BITCOIND_PROC_WAIT_TIMEOUT)
        else:
            self.stop_node(1)

        shutil.copyfile(self.tmpdir + "/node1/regtest/wallet.dat", self.tmpdir + "/wallet.bak")
        self.nodes[1] = self.start_node(1, self.tmpdir, self.extra_args[1])
        connect_nodes_bi(self.nodes, 0, 1)

        self.log.info("Generate keys for wallet")

        for _ in range(80):
            self.nodes[1].getnewaddress()
        if encrypted:
            # Keypool can't top up because the wallet is encrypted
            assert_raises_jsonrpc(-12, "Keypool is at critical level or has run out, please call keypoolrefill first", self.nodes[1].getnewaddress)
            self.nodes[1].walletpassphrase("test", 10)
        for _ in range(10):
            addr_oldpool = self.nodes[1].getnewaddress()
        for _ in range(20):
            addr_extpool = self.nodes[1].getnewaddress()

        self.log.info("Send funds to wallet")

        self.nodes[0].sendtoaddress(addr_oldpool, 10)
        self.nodes[0].generate(1)
        self.nodes[0].sendtoaddress(addr_extpool, 5)
        self.nodes[0].generate(1)
        sync_blocks(self.nodes)

        self.log.info("Restart node with wallet backup")

        self.stop_node(1)

        shutil.copyfile(self.tmpdir + "/wallet.bak", self.tmpdir + "/node1/regtest/wallet.dat")

        if encrypted:
            self.log.info("Encrypted wallet - verify node shuts down to avoid loss of funds")

            self.assert_start_raises_init_error(1, self.tmpdir, self.extra_args[1], expected_msg="Number of keys in keypool is below critical minimum and the wallet is encrypted. Bitcoin will now shutdown to avoid loss of funds.")

            shutil.rmtree(self.tmpdir + "/node1/regtest/chainstate")
            shutil.rmtree(self.tmpdir + "/node1/regtest/blocks")
            os.remove(self.tmpdir + "/node1/regtest/wallet.dat")
            self.nodes[1] = self.start_node(1, self.tmpdir, self.extra_args[1])

        else:
            self.log.info("Unencrypted wallet - verify keypool is restored and balance is correct")

            self.nodes[1] = self.start_node(1, self.tmpdir, self.extra_args[1])
            connect_nodes_bi(self.nodes, 0, 1)
            self.sync_all()

            assert_equal(self.nodes[1].getbalance(), 15)
            assert_equal(self.nodes[1].listtransactions()[0]['category'], "receive")

            # Check that we have marked all keys up to the used keypool key as used
            assert_equal(self.nodes[1].validateaddress(self.nodes[1].getnewaddress())['hdkeypath'], "m/0'/0'/111'")

if __name__ == '__main__':
    KeypoolRestoreTest().main()
