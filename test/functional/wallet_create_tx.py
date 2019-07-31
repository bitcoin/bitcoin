#!/usr/bin/env python3
# Copyright (c) 2018 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
)
from test_framework.blocktools import (
    TIME_GENESIS_BLOCK,
)


class CreateTxWalletTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        self.log.info('Create some old blocks')
        self.nodes[0].setmocktime(TIME_GENESIS_BLOCK)
        self.nodes[0].generate(200)
        self.nodes[0].setmocktime(0)

        self.log.info('Check that we have some (old) blocks and that anti-fee-sniping is disabled')
        assert_equal(self.nodes[0].getblockchaininfo()['blocks'], 200)
        txid = self.nodes[0].sendtoaddress(self.nodes[0].getnewaddress(), 1)
        tx = self.nodes[0].decoderawtransaction(self.nodes[0].gettransaction(txid)['hex'])
        assert_equal(tx['locktime'], 0)

        self.log.info('Check that anti-fee-sniping is enabled when we mine a recent block')
        self.nodes[0].generate(1)
        txid = self.nodes[0].sendtoaddress(self.nodes[0].getnewaddress(), 1)
        tx = self.nodes[0].decoderawtransaction(self.nodes[0].gettransaction(txid)['hex'])
        assert 0 < tx['locktime'] <= 201


if __name__ == '__main__':
    CreateTxWalletTest().main()
