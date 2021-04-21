#!/usr/bin/env python3
# Copyright (c) 2014-2019 The Bitcoin Core developers
# Copyright (c) 2019-2020 Xenios SEZC
# https://www.veriblock.org
# Distributed under the MIT software license, see the accompanying
# file LICENSE or http://www.opensource.org/licenses/mit-license.php.

"""
Feature POP popdata max size test

"""
from test_framework.pop import mine_vbk_blocks, endorse_block, mine_until_pop_active
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    connect_nodes,
)

class PopPayouts(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 2
        self.extra_args = [["-txindex"], ["-txindex"]]

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()
        self.skip_if_no_pypoptools()

    def setup_network(self):
        self.setup_nodes()
        mine_until_pop_active(self.nodes[0])

        connect_nodes(self.nodes[0], 1)
        self.sync_all(self.nodes)

    def _test_case_vbk(self, payloads_amount):
        self.log.info("running _test_case_vbk()")

        vbk_blocks = mine_vbk_blocks(self.nodes[0], self.apm, payloads_amount)

        # mine a block on node[1] with this pop tx
        containingblockhash = self.nodes[0].generate(nblocks=1)[0]
        containingblock = self.nodes[0].getblock(containingblockhash)

        assert len(containingblock['pop']['data']['vbkblocks']) == payloads_amount == len(vbk_blocks)

        self.log.info("success! _test_case_vbk()")

    def _test_case_atv(self, payloads_amount):
        self.log.info("running _test_case_vbk()")

        # endorse block lastblock - 5
        lastblock = self.nodes[0].getblockcount()
        assert lastblock >= 5
        addr = self.nodes[0].getnewaddress()
        for i in range(payloads_amount):
            self.log.info("endorsing block {} on node0 by miner {}".format(lastblock - 5, addr))
            endorse_block(self.nodes[0], self.apm, lastblock - 5, addr)

        # mine a block on node[1] with this pop tx
        containingblockhash = self.nodes[0].generate(nblocks=1)[0]
        containingblock = self.nodes[0].getblock(containingblockhash)

        assert len(containingblock['pop']['data']['atvs']) == payloads_amount

        self.log.info("success! _test_case_atv()")

    def run_test(self):
        """Main test logic"""

        self.nodes[0].generate(nblocks=10)
        self.sync_all(self.nodes)

        from pypoptools.pypopminer import MockMiner
        self.apm = MockMiner()

        self._test_case_vbk(113)
        self._test_case_vbk(13)
        self._test_case_vbk(75)

        self._test_case_atv(42)
        self._test_case_atv(135)
        self._test_case_atv(25)

if __name__ == '__main__':
    PopPayouts().main()