#!/usr/bin/env python3
# Copyright (c) 2019 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the wallet implicit segwit feature."""

import test_framework.address as address
from test_framework.test_framework import BitcoinTestFramework

# TODO: Might be nice to test p2pk here too
address_types = ('legacy', 'bech32', 'p2sh-segwit')

def key_to_address(key, address_type):
    if address_type == 'legacy':
        return address.key_to_p2pkh(key)
    elif address_type == 'p2sh-segwit':
        return address.key_to_p2sh_p2wpkh(key)
    elif address_type == 'bech32':
        return address.key_to_p2wpkh(key)

def send_a_to_b(receive_node, send_node):
    keys = {}
    for a in address_types:
        a_address = receive_node.getnewaddress(address_type=a)
        pubkey = receive_node.getaddressinfo(a_address)['pubkey']
        keys[a] = pubkey
        for b in address_types:
            b_address = key_to_address(pubkey, b)
            send_node.sendtoaddress(address=b_address, amount=1)
    return keys

def check_implicit_transactions(implicit_keys, implicit_node):
    # The implicit segwit node allows conversion all possible ways
    txs = implicit_node.listtransactions(None, 99999)
    for a in address_types:
        pubkey = implicit_keys[a]
        for b in address_types:
            b_address = key_to_address(pubkey, b)
            assert(('receive', b_address) in tuple((tx['category'], tx['address']) for tx in txs))

class ImplicitSegwitTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 2
        self.supports_cli = False

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        self.log.info("Manipulating addresses and sending transactions to all variations")
        implicit_keys = send_a_to_b(self.nodes[0], self.nodes[1])

        self.sync_all()

        self.log.info("Checking that transactions show up correctly without a restart")
        check_implicit_transactions(implicit_keys, self.nodes[0])

        self.log.info("Checking that transactions still show up correctly after a restart")
        self.restart_node(0)
        self.restart_node(1)

        check_implicit_transactions(implicit_keys, self.nodes[0])

if __name__ == '__main__':
    ImplicitSegwitTest().main()
