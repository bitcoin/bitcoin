#!/usr/bin/env python3
# Copyright (c) 2016 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test account RPCs.

RPCs tested are:
    - getaccountaddress
    - getaddressesbyaccount
    - listaddressgroupings
    - setaccount
    - sendfrom (with account arguments)
    - move (with account arguments)
"""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal

class WalletAccountsTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1
        self.extra_args = [[]]

    def run_test(self):
        node = self.nodes[0]
        # Check that there's no UTXO on any of the nodes
        assert_equal(len(node.listunspent()), 0)

        # Note each time we call generate, all generated coins go into
        # the same address, so we call twice to get two addresses w/50 each
        node.generate(1)
        node.generate(101)
        assert_equal(node.getbalance(), 100)

        # there should be 2 address groups
        # each with 1 address with a balance of 50 Bitcoins
        address_groups = node.listaddressgroupings()
        assert_equal(len(address_groups), 2)
        # the addresses aren't linked now, but will be after we send to the
        # common address
        linked_addresses = set()
        for address_group in address_groups:
            assert_equal(len(address_group), 1)
            assert_equal(len(address_group[0]), 2)
            assert_equal(address_group[0][1], 50)
            linked_addresses.add(address_group[0][0])

        # send 50 from each address to a third address not in this wallet
        # There's some fee that will come back to us when the miner reward
        # matures.
        common_address = "msf4WtN1YQKXvNtvdFYt9JBnUD2FB41kjr"
        txid = node.sendmany(
            fromaccount="",
            amounts={common_address: 100},
            subtractfeefrom=[common_address],
            minconf=1,
        )
        tx_details = node.gettransaction(txid)
        fee = -tx_details['details'][0]['fee']
        # there should be 1 address group, with the previously
        # unlinked addresses now linked (they both have 0 balance)
        address_groups = node.listaddressgroupings()
        assert_equal(len(address_groups), 1)
        assert_equal(len(address_groups[0]), 2)
        assert_equal(set([a[0] for a in address_groups[0]]), linked_addresses)
        assert_equal([a[1] for a in address_groups[0]], [0, 0])

        node.generate(1)

        # we want to reset so that the "" account has what's expected.
        # otherwise we're off by exactly the fee amount as that's mined
        # and matures in the next 100 blocks
        node.sendfrom("", common_address, fee)
        accounts = ["a", "b", "c", "d", "e"]
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
            to_account = accounts[(i+1) % len(accounts)]
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
    WalletAccountsTest().main()
