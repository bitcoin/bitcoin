#!/usr/bin/env python3
# Copyright (c) 2021 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the getxpub RPC command."""

from test_framework.authproxy import JSONRPCException
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_raises_rpc_error,
)
from test_framework.descriptors import descsum_create

class WalletGetxpubTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 2

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()
        self.skip_if_no_sqlite()

    def run_test(self):
        self.nodes[0].createwallet(wallet_name="w1")
        w1 = self.nodes[0].get_wallet_rpc("w1")

        self.nodes[1].createwallet(wallet_name="w2", descriptors=True)
        w2 = self.nodes[1].get_wallet_rpc("w2")

        self.log.info("Get xpub")
        res = w2.getxpub("m/87'/0'/0'")
        assert(res['xpub'])
        assert_equal(res['xpub'], w2.getxpub("m/87h/0h/0h")['xpub'])

        self.log.info("Import descriptor using the xpub and fingerprint")
        desc_receive = "tr([" + res['masterfingerprint'] + "/87h/0h/0h]" + res['xpub'] + "/0/*)"
        desc_change = "tr([" + res['masterfingerprint'] + "/87h/0h/0h]" + res['xpub'] + "/1/*)"
        res = w2.importdescriptors([{"desc":descsum_create(desc_receive),
                                     "timestamp": "now",
                                     "active": True,
                                     "internal": False,
                                     "range": [0, 10]},
                                    {"desc":descsum_create(desc_change),
                                     "timestamp": "now",
                                     "active": True,
                                     "internal": True,
                                     "range": [0, 10]}
                                    ])
        assert(res[0]['success'] and res[1]['success'])

        self.log.info("Fund xpub")
        # TODO: if createwallet supports setting a seed without descriptors,
        #       use that here instead. This allows replacing deriveaddresses
        #       with getnewaddress.
        w2_address = self.nodes[1].deriveaddresses(descsum_create(desc_receive), 0)[0]
        w1.sendtoaddress(w2_address, 1)
        self.nodes[0].generate(1)

        self.log.info("Spend from xpub")
        w1_address = self.nodes[0].getnewaddress()
        w2.sendtoaddress(w1_address, 0.1)

if __name__ == '__main__':
    WalletGetxpubTest().main()
