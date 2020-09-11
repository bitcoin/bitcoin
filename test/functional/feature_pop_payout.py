#!/usr/bin/env python3
# Copyright (c) 2014-2019 The Bitcoin Core developers
# Copyright (c) 2019-2020 Xenios SEZC
# https://www.veriblock.org
# Distributed under the MIT software license, see the accompanying
# file LICENSE or http://www.opensource.org/licenses/mit-license.php.

"""
Feature POP Payout test

Consists of multiple test cases.

Case1:
node0 endorses block 5, node1 confirms it and mines blocks until reward for this block is paid.
node0 mines 100 blocks and checks balance.
Expected balance is POW_PAYOUT * 10 + pop payout. (node0 has only 10 mature coinbases)

"""

from test_framework.pop import endorse_block
from test_framework.pop_const import POW_PAYOUT, POP_PAYOUT_DELAY
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
        self.skip_if_no_pypopminer()

    def setup_network(self):
        self.setup_nodes()

        connect_nodes(self.nodes[0], 1)
        self.sync_all(self.nodes)

    def _case1_endorse_keystone_get_paid(self):
        self.log.warning("running _case1_endorse_keystone_get_paid()")

        # endorse block 5
        addr = self.nodes[0].getnewaddress()
        self.log.info("endorsing block 5 on node0 by miner {}".format(addr))
        atv_id = endorse_block(self.nodes[0], self.apm, 5, addr)

        # wait until node[1] gets relayed pop tx
        self.sync_pop_mempools(self.nodes)
        self.log.info("node1 got relayed transaction")

        # mine a block on node[1] with this pop tx
        containingblockhash = self.nodes[1].generate(nblocks=1)[0]
        containingblock = self.nodes[1].getblock(containingblockhash)
        self.log.info("node1 mined containing block={}".format(containingblock['hash']))
        self.nodes[0].waitforblockheight(containingblock['height'])
        self.log.info("node0 got containing block over p2p")

        # assert that txid exists in this block
        block = self.nodes[0].getblock(containingblockhash)

        assert atv_id in block['pop']['data']['atvs']

        # target height is 5 + POP_PAYOUT_DELAY + 1
        n = POP_PAYOUT_DELAY + 6 - block['height']
        payoutblockhash = self.nodes[1].generate(nblocks=n)[-1]
        self.sync_blocks(self.nodes)
        self.log.info("pop rewards paid")

        # check that expected block pays for this endorsement
        block = self.nodes[0].getblock(payoutblockhash)
        coinbasetxhash = block['tx'][0]
        coinbasetx = self.nodes[0].getrawtransaction(coinbasetxhash, 1)
        outputs = coinbasetx['vout']
        assert len(outputs) > 3, "block with payout does not contain pop payout: {}".format(outputs)
        assert outputs[1]['n'] == 1
        assert outputs[1]['value'] > 0, "expected non-zero output at n=1, got: {}".format(outputs[1])


        # mine 100 blocks and check balance
        self.nodes[0].generate(nblocks=100)
        balance = self.nodes[0].getbalance()

        # node[0] has 11 mature coinbases and single pop payout
        assert balance == POW_PAYOUT * 10 + outputs[1]['value']
        self.log.warning("success! _case1_endorse_keystone_get_paid()")

    def run_test(self):
        """Main test logic"""

        self.nodes[0].generate(nblocks=10)
        self.sync_all(self.nodes)

        from pypopminer import MockMiner
        self.apm = MockMiner()

        self._case1_endorse_keystone_get_paid()


if __name__ == '__main__':
    PopPayouts().main()
