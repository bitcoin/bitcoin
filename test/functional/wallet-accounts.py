#!/usr/bin/env python3
# Copyright (c) 2016 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test account RPCs.

RPCs tested are:
    - getaccountaddress
    - getaddressesbyaccount
    - setaccount
    - sendfrom (with account arguments)
    - move (with account arguments)
"""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    start_nodes,
    assert_equal,
)

class WalletAccountsTest(BitcoinTestFramework):

    def __init__(self):
        super().__init__()
        self.setup_clean_chain = True
        self.num_nodes = 1
        self.node_args = [[]]

    def setup_network(self):
        self.nodes = start_nodes(self.num_nodes, self.options.tmpdir, self.node_args)
        self.is_network_split = False

    def run_test (self):
        node = self.nodes[0]
        # Check that there's no UTXO on any of the nodes
        assert_equal(len(node.listunspent()), 0)
        
        node.generate(101)
        
        assert_equal(node.getbalance(), 50)
        
        accounts = ["a","b","c","d","e"]
        amount_to_send = 1.0
        account_addresses = dict()
        for account in accounts:
            address = node.getaccountaddress(account)
            account_addresses[account] = address
            
            node.getnewaddress(account)
            assert_equal(node.getaccount(address), account)
            assert(address in node.getaddressesbyaccount(account))
            
            node.sendfrom("", address, amount_to_send)
        
        node.generate(1)
        
        for i in range(len(accounts)):
            from_account = accounts[i]
            to_account = accounts[(i+1)%len(accounts)]
            to_address = account_addresses[to_account]
            node.sendfrom(from_account, to_address, amount_to_send)
        
        node.generate(1)
        
        for account in accounts:
            address = node.getaccountaddress(account)
            assert(address != account_addresses[account])
            assert_equal(node.getreceivedbyaccount(account), 2)
            node.move(account, "", node.getbalance(account))
        
        node.generate(101)
        
        expected_account_balances = {"": 5200}
        for account in accounts:
            expected_account_balances[account] = 0
        
        assert_equal(node.listaccounts(), expected_account_balances)
        
        assert_equal(node.getbalance(""), 5200)
        
        for account in accounts:
            address = node.getaccountaddress("")
            node.setaccount(address, account)
            assert(address in node.getaddressesbyaccount(account))
            assert(address not in node.getaddressesbyaccount(""))
        
        for account in accounts:
            addresses = []
            for x in range(10):
                addresses.append(node.getnewaddress())
            multisig_address = node.addmultisigaddress(5, addresses, account)
            node.sendfrom("", multisig_address, 50)
        
        node.generate(101)
        
        for account in accounts:
            assert_equal(node.getbalance(account), 50)

if __name__ == '__main__':
    WalletAccountsTest().main ()
