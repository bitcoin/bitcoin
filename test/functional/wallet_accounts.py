#!/usr/bin/env python3
# Copyright (c) 2016-2017 The Bitcoin Core developers
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
        amount_to_send = 1.0

        # Create accounts and make sure subsequent account API calls
        # recognize the account/address associations.
        accounts = [Account(name) for name in ("a", "b", "c", "d", "e")]
        for account in accounts:
            account.add_receive_address(node.getaccountaddress(account.name))
            account.verify(node)

        # Send a transaction to each account, and make sure this forces
        # getaccountaddress to generate a new receiving address.
        for account in accounts:
            node.sendtoaddress(account.receive_address, amount_to_send)
            account.add_receive_address(node.getaccountaddress(account.name))
            account.verify(node)

        # Check the amounts received.
        node.generate(1)
        for account in accounts:
            assert_equal(
                node.getreceivedbyaddress(account.addresses[0]), amount_to_send)
            assert_equal(node.getreceivedbyaccount(account.name), amount_to_send)
        
        # Check that sendfrom account reduces listaccounts balances.
        for i, account in enumerate(accounts):
            to_account = accounts[(i+1) % len(accounts)]
            node.sendfrom(account.name, to_account.receive_address, amount_to_send)
        node.generate(1)
        for account in accounts:
            account.add_receive_address(node.getaccountaddress(account.name))
            account.verify(node)
            assert_equal(node.getreceivedbyaccount(account.name), 2)
            node.move(account.name, "", node.getbalance(account.name))
            account.verify(node)
        node.generate(101)
        expected_account_balances = {"": 5200}
        for account in accounts:
            expected_account_balances[account.name] = 0
        assert_equal(node.listaccounts(), expected_account_balances)
        assert_equal(node.getbalance(""), 5200)
        
        # Check that setaccount can assign an account to a new unused address.
        for account in accounts:
            address = node.getaccountaddress("")
            node.setaccount(address, account.name)
            account.add_address(address)
            account.verify(node)
            assert(address not in node.getaddressesbyaccount(""))
        
        # Check that addmultisigaddress can assign accounts.
        for account in accounts:
            addresses = []
            for x in range(10):
                addresses.append(node.getnewaddress())
            multisig_address = node.addmultisigaddress(5, addresses, account.name)['address']
            account.add_address(multisig_address)
            account.verify(node)
            node.sendfrom("", multisig_address, 50)
        node.generate(101)
        for account in accounts:
            assert_equal(node.getbalance(account.name), 50)

        # Check that setaccount can change the account of an address from a
        # different account.
        change_account(node, accounts[0].addresses[0], accounts[0], accounts[1])

        # Check that setaccount can change the account of an address which
        # is the receiving address of a different account.
        change_account(node, accounts[0].receive_address, accounts[0], accounts[1])

        # Check that setaccount can set the account of an address already
        # in the account. This is a no-op.
        change_account(node, accounts[2].addresses[0], accounts[2], accounts[2])

        # Check that setaccount can set the account of an address which is
        # already the receiving address of the account. It would probably make
        # sense for this to be a no-op, but right now it resets the receiving
        # address, causing getaccountaddress to return a brand new address.
        change_account(node, accounts[2].receive_address, accounts[2], accounts[2])

class Account:
    def __init__(self, name):
        # Account name
        self.name = name
        # Current receiving address associated with this account.
        self.receive_address = None
        # List of all addresses assigned with this account
        self.addresses = []

    def add_address(self, address):
        assert_equal(address not in self.addresses, True)
        self.addresses.append(address)

    def add_receive_address(self, address):
        self.add_address(address)
        self.receive_address = address

    def verify(self, node):
        if self.receive_address is not None:
            assert self.receive_address in self.addresses
            assert_equal(node.getaccountaddress(self.name), self.receive_address)

        for address in self.addresses:
            assert_equal(node.getaccount(address), self.name)

        assert_equal(
            set(node.getaddressesbyaccount(self.name)), set(self.addresses))


def change_account(node, address, old_account, new_account):
    assert_equal(address in old_account.addresses, True)
    node.setaccount(address, new_account.name)

    old_account.addresses.remove(address)
    new_account.add_address(address)

    # Calling setaccount on an address which was previously the receiving
    # address of a different account should reset the receiving address of
    # the old account, causing getaccountaddress to return a brand new
    # address.
    if address == old_account.receive_address:
        new_address = node.getaccountaddress(old_account.name)
        assert_equal(new_address not in old_account.addresses, True)
        assert_equal(new_address not in new_account.addresses, True)
        old_account.add_receive_address(new_address)

    old_account.verify(node)
    new_account.verify(node)


if __name__ == '__main__':
    WalletAccountsTest().main()
