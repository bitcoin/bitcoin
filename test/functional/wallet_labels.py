#!/usr/bin/env python3
# Copyright (c) 2016-2017 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test label RPCs.

RPCs tested are:
    - getaccountaddress
    - getaddressesbyaccount/getaddressesbylabel
    - listaddressgroupings
    - setlabel
    - sendfrom (with account arguments)
    - move (with account arguments)

Run the test twice - once using the accounts API and once using the labels API.
The accounts API test can be removed in V0.18.
"""
from collections import defaultdict

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal, assert_raises_rpc_error

class WalletLabelsTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 2
        self.extra_args = [['-deprecatedrpc=accounts'], []]

    def setup_network(self):
        """Don't connect nodes."""
        self.setup_nodes()

    def run_test(self):
        """Run the test twice - once using the accounts API and once using the labels API."""
        self.log.info("Test accounts API")
        self._run_subtest(True, self.nodes[0])
        self.log.info("Test labels API")
        self._run_subtest(False, self.nodes[1])

    def _run_subtest(self, accounts_api, node):
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

        # we want to reset so that the "" label has what's expected.
        # otherwise we're off by exactly the fee amount as that's mined
        # and matures in the next 100 blocks
        if accounts_api:
            node.sendfrom("", common_address, fee)
        amount_to_send = 1.0

        # Create labels and make sure subsequent label API calls
        # recognize the label/address associations.
        labels = [Label(name, accounts_api) for name in ("a", "b", "c", "d", "e")]
        for label in labels:
            if accounts_api:
                address = node.getaccountaddress(label.name)
            else:
                address = node.getnewaddress(label.name)
            label.add_receive_address(address)
            label.verify(node)

        # Check all labels are returned by listlabels.
        assert_equal(node.listlabels(), [label.name for label in labels])

        # Send a transaction to each label, and make sure this forces
        # getaccountaddress to generate a new receiving address.
        for label in labels:
            if accounts_api:
                node.sendtoaddress(label.receive_address, amount_to_send)
                label.add_receive_address(node.getaccountaddress(label.name))
            else:
                node.sendtoaddress(label.addresses[0], amount_to_send)
            label.verify(node)

        # Check the amounts received.
        node.generate(1)
        for label in labels:
            assert_equal(
                node.getreceivedbyaddress(label.addresses[0]), amount_to_send)
            assert_equal(node.getreceivedbylabel(label.name), amount_to_send)

        # Check that sendfrom label reduces listaccounts balances.
        for i, label in enumerate(labels):
            to_label = labels[(i + 1) % len(labels)]
            if accounts_api:
                node.sendfrom(label.name, to_label.receive_address, amount_to_send)
            else:
                node.sendtoaddress(to_label.addresses[0], amount_to_send)
        node.generate(1)
        for label in labels:
            if accounts_api:
                address = node.getaccountaddress(label.name)
            else:
                address = node.getnewaddress(label.name)
            label.add_receive_address(address)
            label.verify(node)
            assert_equal(node.getreceivedbylabel(label.name), 2)
            if accounts_api:
                node.move(label.name, "", node.getbalance(label.name))
            label.verify(node)
        node.generate(101)
        expected_account_balances = {"": 5200}
        for label in labels:
            expected_account_balances[label.name] = 0
        if accounts_api:
            assert_equal(node.listaccounts(), expected_account_balances)
            assert_equal(node.getbalance(""), 5200)

        # Check that setlabel can assign a label to a new unused address.
        for label in labels:
            address = node.getnewaddress()
            node.setlabel(address, label.name)
            label.add_address(address)
            label.verify(node)
            if accounts_api:
                assert address not in node.getaddressesbyaccount("")
            else:
                assert_raises_rpc_error(-11, "No addresses with label", node.getaddressesbylabel, "")

        # Check that addmultisigaddress can assign labels.
        for label in labels:
            addresses = []
            for x in range(10):
                addresses.append(node.getnewaddress())
            multisig_address = node.addmultisigaddress(5, addresses, label.name)['address']
            label.add_address(multisig_address)
            label.purpose[multisig_address] = "send"
            label.verify(node)
            if accounts_api:
                node.sendfrom("", multisig_address, 50)
        node.generate(101)
        if accounts_api:
            for label in labels:
                assert_equal(node.getbalance(label.name), 50)

        # Check that setlabel can change the label of an address from a
        # different label.
        change_label(node, labels[0].addresses[0], labels[0], labels[1], accounts_api)

        # Check that setlabel can set the label of an address already
        # in the label. This is a no-op.
        change_label(node, labels[2].addresses[0], labels[2], labels[2], accounts_api)

        if accounts_api:
            # Check that setaccount can change the label of an address which
            # is the receiving address of a different label.
            change_label(node, labels[0].receive_address, labels[0], labels[1], accounts_api)

            # Check that setaccount can set the label of an address which is
            # already the receiving address of the label. This is a no-op.
            change_label(node, labels[2].receive_address, labels[2], labels[2], accounts_api)

class Label:
    def __init__(self, name, accounts_api):
        # Label name
        self.name = name
        self.accounts_api = accounts_api
        # Current receiving address associated with this label.
        self.receive_address = None
        # List of all addresses assigned with this label
        self.addresses = []
        # Map of address to address purpose
        self.purpose = defaultdict(lambda: "receive")

    def add_address(self, address):
        assert_equal(address not in self.addresses, True)
        self.addresses.append(address)

    def add_receive_address(self, address):
        self.add_address(address)
        if self.accounts_api:
            self.receive_address = address

    def verify(self, node):
        if self.receive_address is not None:
            assert self.receive_address in self.addresses
            if self.accounts_api:
                assert_equal(node.getaccountaddress(self.name), self.receive_address)

        for address in self.addresses:
            assert_equal(
                node.getaddressinfo(address)['labels'][0],
                {"name": self.name,
                 "purpose": self.purpose[address]})
            if self.accounts_api:
                assert_equal(node.getaccount(address), self.name)
            else:
                assert_equal(node.getaddressinfo(address)['label'], self.name)

        assert_equal(
            node.getaddressesbylabel(self.name),
            {address: {"purpose": self.purpose[address]} for address in self.addresses})
        if self.accounts_api:
            assert_equal(set(node.getaddressesbyaccount(self.name)), set(self.addresses))


def change_label(node, address, old_label, new_label, accounts_api):
    assert_equal(address in old_label.addresses, True)
    if accounts_api:
        node.setaccount(address, new_label.name)
    else:
        node.setlabel(address, new_label.name)

    old_label.addresses.remove(address)
    new_label.add_address(address)

    # Calling setaccount on an address which was previously the receiving
    # address of a different account should reset the receiving address of
    # the old account, causing getaccountaddress to return a brand new
    # address.
    if accounts_api:
        if old_label.name != new_label.name and address == old_label.receive_address:
            new_address = node.getaccountaddress(old_label.name)
            assert_equal(new_address not in old_label.addresses, True)
            assert_equal(new_address not in new_label.addresses, True)
            old_label.add_receive_address(new_address)

    old_label.verify(node)
    new_label.verify(node)


if __name__ == '__main__':
    WalletLabelsTest().main()
