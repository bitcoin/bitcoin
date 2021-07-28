#!/usr/bin/env python3
# Copyright (c) 2014-2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the listtransactions API."""
from decimal import Decimal

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_array_result,
    assert_equal,
)

class ListTransactionsTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 2
        # This test isn't testing txn relay/timing, so set whitelist on the
        # peers for instant txn relay. This speeds up the test run time 2-3x.
        self.extra_args = [["-whitelist=noban@127.0.0.1"]] * self.num_nodes

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        self.log.info("Test simple send from node0 to node1")
        txid = self.nodes[0].sendtoaddress(self.nodes[1].getnewaddress(), 0.1)
        self.sync_all()
        assert_array_result(self.nodes[0].listtransactions(),
                            {"txid": txid},
                            {"category": "send", "amount": Decimal("-0.1"), "confirmations": 0})
        assert_array_result(self.nodes[1].listtransactions(),
                            {"txid": txid},
                            {"category": "receive", "amount": Decimal("0.1"), "confirmations": 0})
        self.log.info("Test confirmations change after mining a block")
        blockhash = self.nodes[0].generate(1)[0]
        blockheight = self.nodes[0].getblockheader(blockhash)['height']
        self.sync_all()
        assert_array_result(self.nodes[0].listtransactions(),
                            {"txid": txid},
                            {"category": "send", "amount": Decimal("-0.1"), "confirmations": 1, "blockhash": blockhash, "blockheight": blockheight})
        assert_array_result(self.nodes[1].listtransactions(),
                            {"txid": txid},
                            {"category": "receive", "amount": Decimal("0.1"), "confirmations": 1, "blockhash": blockhash, "blockheight": blockheight})

        self.log.info("Test send-to-self on node0")
        txid = self.nodes[0].sendtoaddress(self.nodes[0].getnewaddress(), 0.2)
        assert_array_result(self.nodes[0].listtransactions(),
                            {"txid": txid, "category": "send"},
                            {"amount": Decimal("-0.2")})
        assert_array_result(self.nodes[0].listtransactions(),
                            {"txid": txid, "category": "receive"},
                            {"amount": Decimal("0.2")})

        self.log.info("Test sendmany from node1: twice to self, twice to node0")
        send_to = {self.nodes[0].getnewaddress(): 0.11,
                   self.nodes[1].getnewaddress(): 0.22,
                   self.nodes[0].getnewaddress(): 0.33,
                   self.nodes[1].getnewaddress(): 0.44}
        txid = self.nodes[1].sendmany("", send_to)
        self.sync_all()
        assert_array_result(self.nodes[1].listtransactions(),
                            {"category": "send", "amount": Decimal("-0.11")},
                            {"txid": txid})
        assert_array_result(self.nodes[0].listtransactions(),
                            {"category": "receive", "amount": Decimal("0.11")},
                            {"txid": txid})
        assert_array_result(self.nodes[1].listtransactions(),
                            {"category": "send", "amount": Decimal("-0.22")},
                            {"txid": txid})
        assert_array_result(self.nodes[1].listtransactions(),
                            {"category": "receive", "amount": Decimal("0.22")},
                            {"txid": txid})
        assert_array_result(self.nodes[1].listtransactions(),
                            {"category": "send", "amount": Decimal("-0.33")},
                            {"txid": txid})
        assert_array_result(self.nodes[0].listtransactions(),
                            {"category": "receive", "amount": Decimal("0.33")},
                            {"txid": txid} )
        assert_array_result(self.nodes[1].listtransactions(),
                            {"category": "send", "amount": Decimal("-0.44")},
                            {"txid": txid} )
        assert_array_result(self.nodes[1].listtransactions(),
                            {"category": "receive", "amount": Decimal("0.44")},
                            {"txid": txid} )

        if not self.options.descriptors:
            # include_watchonly is a legacy wallet feature, so don't test it for descriptor wallets
            self.log.info("Test 'include_watchonly' feature (legacy wallet)")
            pubkey = self.nodes[1].getaddressinfo(self.nodes[1].getnewaddress())['pubkey']
            multisig = self.nodes[1].createmultisig(1, [pubkey])
            self.nodes[0].importaddress(multisig["redeemScript"], "watchonly", False, True)
            txid = self.nodes[1].sendtoaddress(multisig["address"], 0.1)
            self.nodes[1].generate(1)
            self.sync_all()
            assert_equal(len(self.nodes[0].listtransactions(label="watchonly", include_watchonly=True)), 1)
            assert_equal(len(self.nodes[0].listtransactions(dummy="watchonly", include_watchonly=True)), 1)
            assert len(self.nodes[0].listtransactions(label="watchonly", count=100, include_watchonly=False)) == 0
            assert_array_result(self.nodes[0].listtransactions(label="watchonly", count=100, include_watchonly=True),
                                {"category": "receive", "amount": Decimal("0.1")},
                                {"txid": txid, "label": "watchonly"})

if __name__ == '__main__':
    ListTransactionsTest().main()
