#!/usr/bin/env python3
# Copyright (c) 2019 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the importdescriptor RPC.

Test importdescriptor by generating keys on node0, importing descriptors on node1
and then testing the address info for the different address variants."""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.descriptors import descsum_create
from test_framework.util import (
    assert_equal,
    assert_raises_rpc_error
)

class ImportDescriptorTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 2
        self.extra_args = [["-addresstype=legacy"], ["-addresstype=legacy"]]
        self.setup_clean_chain = True

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def setup_network(self):
        self.setup_nodes()

    def run_test(self):
        self.log.info("Mining blocks...")
        self.nodes[0].generate(1)
        self.nodes[1].generate(1)
        timestamp = self.nodes[1].getblock(self.nodes[1].getbestblockhash())['mediantime']

        xpub = "tpubDAXcJ7s7ZwicqjprRaEWdPoHKrCS215qxGYxpusRLLmJuT69ZSicuGdSfyvyKpvUNYBW1s2U3NSrT6vrCYB9e6nZUEvrqnwXPF8ArTCRXMY"

        # Import into native descriptor wallet
        self.log.info("Should import into a descriptor wallet")
        self.nodes[1].createwallet(wallet_name="descriptor", disable_private_keys=True, blank=True, descriptor=True)
        wrpc = self.nodes[1].get_wallet_rpc("descriptor")

        # addr1 = self.nodes[0].getnewaddress()
        # addr2 = self.nodes[0].getnewaddress()
        # pub1 = self.nodes[0].getaddressinfo(addr1)['pubkey']
        # pub2 = self.nodes[0].getaddressinfo(addr2)['pubkey']
        result = wrpc.importdescriptor(descsum_create('wpkh(' + xpub + '/0/*)'), {"timestamp": timestamp, "address_source": True})
        assert_equal(result, {'success': True})
        result = wrpc.importdescriptor(descsum_create('sh(wpkh(' + xpub + '/0/*))'), {"timestamp": timestamp, "address_source": True})
        assert_equal(result, {'success': True})
        result = wrpc.importdescriptor(descsum_create('wpkh(' + xpub + '/1/*)'), {"change": True, "address_source": True})
        assert_equal(result, {'success': True})
        result = wrpc.importdescriptor(descsum_create('sh(wpkh(' + xpub + '/1/*))'), {"change": True, "address_source": True})
        assert_equal(result, {'success': True})


        self.log.info("Should not import the same descriptor twice")
        assert_raises_rpc_error(-4, 'Descriptor already in wallet', wrpc.importdescriptor, descsum_create('wpkh(' + xpub + '/0/*)'))

        self.log.info("Should only import one receive/change descriptor per address type for address source")
        assert_raises_rpc_error(-4, 'Wallet already contains a bech32 receive descriptor.', wrpc.importdescriptor, descsum_create('wpkh(' + xpub + '/2/*)'), {"address_source": True})
        assert_raises_rpc_error(-4, 'Wallet already contains a legacy change descriptor.', wrpc.importdescriptor, descsum_create('sh(wpkh(' + xpub + '/2/*))'), {"change": True, "address_source": True})

        self.log.info("Can import multiple receive/change descriptor per address type for non address source")
        wrpc.importdescriptor(descsum_create('wpkh(' + xpub + '/2/*)'))
        wrpc.importdescriptor(descsum_create('sh(wpkh(' + xpub + '/2/*))'), {"internal": True})

if __name__ == '__main__':
    ImportDescriptorTest().main()
