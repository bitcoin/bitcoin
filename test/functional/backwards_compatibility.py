#!/usr/bin/env python3
# Copyright (c) 2018 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Downgrade functional test

Test various downgrade scenarios. Compile the previous node binaries:

contrib/devtools/previous_release.sh -f v0.15.1 v0.14.2

Due to RPC changes introduced in v0.14 the below tests won't work for older
versions without some patches or workarounds.

Use only the latest patch version of each release, unless a test specifically
needs an older patch version.
"""
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_not_equal,
    assert_raises,
    sync_blocks,
    connect_nodes
)
from shutil import copy2

# Node alias:
V0_15 = 2
V0_14 = 3

class DowngradeTest(BitcoinTestFramework):

    
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 4
        # TODO: add new version after each release
        self.extra_args = [
            [], # Pre-release: use to mine blocks
            [], # Pre-release: use to receive coins, swap wallets, etc
            ["-vbparams=segwit:0:0", "-vbparams=csv:0:0"], # v0.15.1
            ["-vbparams=segwit:0:0", "-vbparams=csv:0:0"], # v0.14.2
        ]

    def setup_nodes(self):
        self.add_nodes(self.num_nodes, extra_args=self.extra_args, binary=[
            None,
            None,
            "build/releases/v0.15.1/src/bitcoind",
            "build/releases/v0.14.2/src/bitcoind",
        ])
        self.start_nodes()
    
    def node_wallet_path(self, node):
        # TODO:
        # * make part of test framework
        # * avoid hardcoded path
        # * don't hardcode node version

        if (node < 2): # >= v0.16
            return self.nodes[1].datadir + "/regtest/wallets/wallet.dat"
        else: # <= v0.15
            return self.nodes[node].datadir + "/regtest/wallet.dat"
    
    def backup_wallet_push(self, node):
        # TODO: actually stack backups
        # TODO: use dynamic wallet load / unload once merged
        self.stop_node(node)
        copy2(self.node_wallet_path(node), self.node_wallet_path(node) + "-backup")
        self.start_node(node)
        connect_nodes(self.nodes[node - 1], node)
        connect_nodes(self.nodes[node], node + 1)
    
    def backup_wallet_pop(self, node):
        # TODO: actually pop backup stack
        # TODO: use dynamic wallet load / unload once merged
        copy2(self.node_wallet_path(node) + "-backup", self.node_wallet_path(node))
        # TODO: start node and restore connections
    
    def copy_wallet(self, origin, destination):
        # TODO: make part of test framework
        # TODO: use dynamic wallet load / unload once merged

        self.stop_node(origin)
        self.stop_node(destination)
        copy2(self.node_wallet_path(origin), self.node_wallet_path(destination))
        self.start_node(origin)
        self.start_node(destination)
        connect_nodes(self.nodes[origin - 1], origin)
        connect_nodes(self.nodes[origin], origin + 1)
        connect_nodes(self.nodes[destination - 1], destination)
        connect_nodes(self.nodes[destination], destination + 1)
        sync_blocks(self.nodes)

    def run_test(self):
        self.nodes[0].generate(101)

        sync_blocks(self.nodes)

        # Santity check the test framework:
        res = self.nodes[self.num_nodes - 1].getblockchaininfo()
        assert_equal(res['blocks'], 101)
        
        # Check that v0.16 wallet can't be used on v0.15 node
        # TODO: use v0.16 node after release
        self.backup_wallet_push(V0_15)
        self.log.debug("Expecting bitcoind to fail with:")
        self.log.debug("Error loading wallet.dat: Wallet requires newer version of Bitcoin Core")
        # TODO: use something like assert_raises_process_error to check
        #       specific bitcoind launch error 
        assert_raises(AssertionError, lambda: self.copy_wallet(1,V0_15))
        self.log.info('Restoring backup')
        self.backup_wallet_pop(V0_15)
        self.start_node(V0_15)
        connect_nodes(self.nodes[V0_15 - 1], V0_15)
        connect_nodes(self.nodes[V0_15], V0_15 + 1)
        sync_blocks(self.nodes)
        
        # Upgrade v0.15.1 wallet from node2 on node1
        self.copy_wallet(V0_15,1)
        
        # SegWit wallet related scenarios from gist:
        # https://gist.github.com/sipa/125cfa1615946d0c3f3eec2ad7f250a2#segwit-wallet-support
        # 1) make backup, upgrade, create segwit address, restore from backup
        # 2) upgrade, make backup, downgrade, restore backup
        # 3) upgrade, new address, downgrade: P2SH-P2WPKH should still exist
        # 4) backup, upgrade, new address, downgrade, restore backup: not supported
        # 5) upgrade, backup, new address, receive coins, restore backup, downgrade

        # Generate and fund a legacy, segwit-p2sh and bech32 address on node1
        address1 = self.nodes[1].getnewaddress(account="legacy", address_type='legacy')
        address2 = self.nodes[1].getnewaddress(account="p2sh", address_type='p2sh-segwit')
        address3 = self.nodes[1].getnewaddress(account="p2sh-nocoin", address_type='p2sh-segwit')
        address4 = self.nodes[1].getnewaddress(account="bech32", address_type='bech32')
        self.nodes[0].sendtoaddress(address1, 1)
        self.nodes[0].sendtoaddress(address2, 1)
        # no coins sent to address3
        self.nodes[0].sendtoaddress(address4, 1)

        self.nodes[0].generate(1)
        sync_blocks(self.nodes)
        assert_equal(self.nodes[1].getbalance("legacy"), 1)
        assert_equal(self.nodes[1].getbalance("p2sh"), 1)
        assert_equal(self.nodes[1].getbalance("bech32"), 1)

        # Copy node1 wallet to node2 and check v0.15.1 can still spend this:
        assert_equal(self.nodes[V0_15].getbalance("legacy"), 0)
        assert_equal(self.nodes[V0_15].getbalance("p2sh"), 0)
        assert_equal(self.nodes[V0_15].getbalance("bech32"), 0)

        self.copy_wallet(1,V0_15)
        assert_equal(self.nodes[V0_15].getbalance("legacy"), 1)
        assert_equal(self.nodes[V0_15].getbalance("p2sh"), 1) # Scenario 3
        
        # bech32 account will appear empty:
        assert_equal(self.nodes[V0_15].getbalance("bech32"), 0) 
        
        # Check address formats:
        assert_equal(self.nodes[V0_15].getaddressesbyaccount("legacy"), [address1]) 
        assert_equal(self.nodes[V0_15].getaddressesbyaccount("p2sh"), [address2]) 
        
        # bech32 address is corrupted until the next upgrade:
        assert_equal(self.nodes[V0_15].getaddressesbyaccount("bech32"), ["3QJmnh"])
        
        # Scenario 1: create and fund address on node 1 before we override with backup
        address5 = self.nodes[1].getnewaddress(account="p2sh-post-backup", address_type='p2sh-segwit')
        address6 = self.nodes[1].getnewaddress(account="bech32-post-backup", address_type='bech32')
        address7 = self.nodes[1].getnewaddress(account="p2sh-disappear", address_type='p2sh-segwit')
        self.nodes[0].sendtoaddress(address5, 1)
        self.nodes[0].sendtoaddress(address6, 1)
        self.nodes[0].sendtoaddress(address7, 1)
        self.nodes[0].generate(1)
        sync_blocks(self.nodes)
        assert_equal(self.nodes[1].getbalance("p2sh-post-backup"), 1)
        assert_equal(self.nodes[1].getbalance("bech32-post-backup"), 1)
        assert_equal(self.nodes[1].getbalance("p2sh-disappear"), 1)

        # Upgrade back to v0.16 by copying to node1:
        # TODO: use v0.16 node after release
        self.copy_wallet(V0_15,1)

        assert_equal(self.nodes[1].getbalance("legacy"), 1)
        assert_equal(self.nodes[1].getbalance("p2sh"), 1) # Scenerio 5

        # Scenario 2
        assert_equal(self.nodes[1].getaddressesbyaccount("p2sh-nocoin"), [address3])
        
        # bech32 account should show funds again (scenerio 2):
        assert_equal(self.nodes[1].getbalance("bech32"), 1) 
        
        # bech32 address format should be restored:
        assert_equal(self.nodes[1].getaddressesbyaccount("bech32"), [address4])
        
        # Check scenario 1, SegWit addresses should come back, even if not explictly requested:
        self.nodes[1].getnewaddress(account="p2sh-post-backup", address_type='p2sh-segwit')
        assert_equal(self.nodes[1].getaddressesbyaccount("p2sh-post-backup"), [address5])
        self.nodes[1].getnewaddress(account="bech32-post-backup", address_type='bech32')
        assert_equal(self.nodes[1].getaddressesbyaccount("bech32-post-backup"), [address6])
        
        self.nodes[1].getnewaddress(account="p2sh-disappear", address_type='bech32') # wrong type!
        self.nodes[1].getnewaddress(account="p2sh-disappear", address_type='p2sh-segwit') # next key...
        assert_not_equal(self.nodes[1].getaddressesbyaccount("p2sh-disappear"), [address7])
        assert_equal(self.nodes[1].getbalance("p2sh-disappear"), 0)

        # Balance won't immedidately update:
        self.nodes[1].rescanblockchain()
        assert_equal(self.nodes[1].getbalance("p2sh-post-backup"), 1)
        assert_equal(self.nodes[1].getbalance("bech32-post-backup"), 1) 
        assert_equal(self.nodes[1].getbalance("p2sh-disappear"), 0)
        
if __name__ == '__main__':
    DowngradeTest().main()
