#!/usr/bin/env python3
# Copyright (c) 2016 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test label RPCs.

RPCs tested are:
    - getlabeladdress
    - getaddressesbyaccount
    - listaddressgroupings
    - setlabel
    - sendfrom (with account arguments)
    - move (with account arguments)
"""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal

class WalletLabelsTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1
        self.extra_args = [["-paytxfee=0.0001"]]

    def run_test(self):
        node = self.nodes[0]
        # Check that there's no UTXO on any of the nodes
        assert_equal(len(node.listunspent()), 0)

        # Note each time we call generate, all generated coins go into
        # the same address, so we call twice to get two addresses w/500 each
        node.generate(1)
        node.generate(101)
        assert_equal(node.getbalance(), 1000)

        # there should be 2 address groups
        # each with 1 address with a balance of 500 Dash
        address_groups = node.listaddressgroupings()
        assert_equal(len(address_groups), 2)
        # the addresses aren't linked now, but will be after we send to the
        # common address
        linked_addresses = set()
        for address_group in address_groups:
            assert_equal(len(address_group), 1)
            assert_equal(len(address_group[0]), 2)
            assert_equal(address_group[0][1], 500)
            linked_addresses.add(address_group[0][0])

        # send 500 from each address to a third address not in this wallet
        # There's some fee that will come back to us when the miner reward
        # matures.
        common_address = "yd5KMREs3GLMe6mTJYr3YrH1juwNwrFCfB"
        txid = node.sendmany(
            fromaccount="",
            amounts={common_address: 1000},
            minconf=1,
            addlocked=False,
            comment="",
            subtractfeefrom=[common_address],
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
        node.sendfrom("", common_address, fee)
        amount_to_send = 1.0

        # Create labels and make sure subsequent label API calls
        # recognize the label/address associations.
        labels = [Label(name) for name in ("a", "b", "c", "d", "e")]
        for label in labels:
            label.add_receive_address(node.getlabeladdress(label.name))
            label.verify(node)

        # Send a transaction to each label, and make sure this forces
        # getlabeladdress to generate a new receiving address.
        for label in labels:
            node.sendtoaddress(label.receive_address, amount_to_send)
            label.add_receive_address(node.getlabeladdress(label.name))
            label.verify(node)

        # Check the amounts received.
        node.generate(1)
        for label in labels:
            assert_equal(
                node.getreceivedbyaddress(label.addresses[0]), amount_to_send)
            assert_equal(node.getreceivedbylabel(label.name), amount_to_send)

        # Check that sendfrom label reduces listaccounts balances.
        for i, label in enumerate(labels):
            to_label = labels[(i+1) % len(labels)]
            node.sendfrom(label.name, to_label.receive_address, amount_to_send)
        node.generate(1)
        for label in labels:
            label.add_receive_address(node.getlabeladdress(label.name))
            label.verify(node)
            assert_equal(node.getreceivedbylabel(label.name), 2)
            node.move(label.name, "", node.getbalance(label.name))
            label.verify(node)
        node.generate(101)
        expected_account_balances = {"": 52000}
        for label in labels:
            expected_account_balances[label.name] = 0
        assert_equal(node.listaccounts(), expected_account_balances)
        assert_equal(node.getbalance(""), 52000)

        # Check that setlabel can assign a label to a new unused address.
        for label in labels:
            address = node.getlabeladdress("")
            node.setlabel(address, label.name)
            label.add_address(address)
            label.verify(node)
            assert(address not in node.getaddressesbyaccount(""))

        # Check that addmultisigaddress can assign labels.
        for label in labels:
            addresses = []
            for x in range(10):
                addresses.append(node.getnewaddress())
            multisig_address = node.addmultisigaddress(5, addresses, label.name)['address']
            label.add_address(multisig_address)
            label.verify(node)
            node.sendfrom("", multisig_address, 50)
        node.generate(101)
        for label in labels:
            assert_equal(node.getbalance(label.name), 50)

        # Check that setlabel can change the label of an address from a
        # different label.
        change_label(node, labels[0].addresses[0], labels[0], labels[1])

        # Check that setlabel can change the label of an address which
        # is the receiving address of a different label.
        change_label(node, labels[0].receive_address, labels[0], labels[1])

        # Check that setlabel can set the label of an address already
        # in the label. This is a no-op.
        change_label(node, labels[2].addresses[0], labels[2], labels[2])

        # Check that setlabel can set the label of an address which is
        # already the receiving address of the label. It would probably make
        # sense for this to be a no-op, but right now it resets the receiving
        # address, causing getlabeladdress to return a brand new address.
        change_label(node, labels[2].receive_address, labels[2], labels[2])

class Label:
    def __init__(self, name):
        # Label name
        self.name = name
        # Current receiving address associated with this label.
        self.receive_address = None
        # List of all addresses assigned with this label
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
            assert_equal(node.getlabeladdress(self.name), self.receive_address)

        for address in self.addresses:
            assert_equal(node.getaccount(address), self.name)

        assert_equal(
            set(node.getaddressesbyaccount(self.name)), set(self.addresses))


def change_label(node, address, old_label, new_label):
    assert_equal(address in old_label.addresses, True)
    node.setlabel(address, new_label.name)

    old_label.addresses.remove(address)
    new_label.add_address(address)

    # Calling setlabel on an address which was previously the receiving
    # address of a different label should reset the receiving address of
    # the old label, causing getlabeladdress to return a brand new
    # address.
    if address == old_label.receive_address:
        new_address = node.getlabeladdress(old_label.name)
        assert_equal(new_address not in old_label.addresses, True)
        assert_equal(new_address not in new_label.addresses, True)
        old_label.add_receive_address(new_address)

    old_label.verify(node)
    new_label.verify(node)


if __name__ == '__main__':
    WalletLabelsTest().main()
