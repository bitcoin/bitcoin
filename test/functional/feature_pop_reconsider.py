#!/usr/bin/env python3
# Copyright (c) 2014-2019 The Bitcoin Core developers
# Copyright (c) 2019-2020 Xenios SEZC
# https://www.veriblock.org
# Distributed under the MIT software license, see the accompanying
# file LICENSE or http://www.opensource.org/licenses/mit-license.php.

"""
Start 1 node.
Mine 100 blocks.
Endorse block 100, mine pop tx in 101.
Mine 19 blocks.
Invalidate block 101.
Ensure tip is block 100 and is not endorsed.
Mine 200 blocks.
Ensure tip is block 300.
Reconsider block 101.
Ensure shorter fork wins and new tip is block 120.
"""

from test_framework.pop import endorse_block, create_endorsed_chain, mine_until_pop_active
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    wait_until,
    connect_nodes,
    disconnect_nodes, assert_equal,
)


class PopReconsider(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1
        self.extra_args = [["-txindex"]]

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()
        self.skip_if_no_pypoptools()

    def setup_network(self):
        self.setup_nodes()
        mine_until_pop_active(self.nodes[0])
        self.sync_all()

    def get_best_block(self, node):
        hash = node.getbestblockhash()
        return node.getblock(hash)

    def assert_tip(self, hash):
        tip = self.nodes[0].getbestblockhash()
        assert_equal(tip, hash)

    def _invalidate_works(self):
        self.log.warning("starting _invalidate_works()")
        self.lastblock = self.nodes[0].getblockcount()

        # start with lastblock + 100 blocks
        self.chainAtiphash = self.nodes[0].generate(nblocks=100)[-1]
        self.log.info("node mined 100 blocks")

        self.assert_tip(self.chainAtiphash)

        # endorse block 300 (fork A tip)
        addr0 = self.nodes[0].getnewaddress()
        txid = endorse_block(self.nodes[0], self.apm, self.lastblock + 100, addr0)
        self.log.info("node endorsed block %d (fork A tip)", self.lastblock + 100)
        # mine pop tx on node0
        self.chainAtiphash = self.nodes[0].generate(nblocks=1)[-1]
        self.assert_tip(self.chainAtiphash)

        containingblock = self.nodes[0].getblock(self.chainAtiphash)

        tip = self.get_best_block(self.nodes[0])
        assert txid in containingblock['pop']['data']['atvs'], "pop tx is not in containing block"
        self.log.info("node tip is {}".format(tip['height']))

        self.chainAtiphash = self.nodes[0].generate(nblocks=19)[-1]
        self.chainAtipheight = self.nodes[0].getblock(self.chainAtiphash)['height']
        self.forkheight = self.lastblock + 50
        self.forkhash = self.nodes[0].getblockhash(self.forkheight)

        self.log.info("tip={}:{}, fork={}:{}".format(self.chainAtipheight, self.chainAtiphash, self.forkheight, self.forkhash))

        self.invalidatedheight = self.forkheight + 1
        self.invalidated = self.nodes[0].getblockhash(self.invalidatedheight)
        self.log.info("invalidating block next to fork block {}:{}".format(self.invalidatedheight, self.invalidated))
        self.nodes[0].invalidateblock(self.invalidated)

        tip = self.get_best_block(self.nodes[0])
        assert tip['height'] == self.forkheight
        assert tip['hash'] == self.forkhash

        tip = self.nodes[0].getblock(tip['hash'])
        assert not tip['pop']['state']['endorsedBy'], "block should not be endorsed after invalidation"

        # rewrite invalid block with generatetoaddress
        # otherwise next block will be a duplicate
        addr1 = self.nodes[0].getnewaddress()
        self.chainBtiphash = self.nodes[0].generatetoaddress(nblocks=1, address=addr1)[-1]

    def _reconsider_works(self):
        self.log.warning("starting _reconsider_works()")

        self.assert_tip(self.chainBtiphash)

        # start with lastblock + 1 + 199 blocks
        self.chainBtiphash = self.nodes[0].generate(nblocks=199)[-1]
        self.log.info("node mined 199 blocks")

        tip = self.get_best_block(self.nodes[0])
        self.log.info("node tip is {}".format(tip['height']))
        self.assert_tip(tip['hash'])

        # Reconsider invalidated block
        self.log.info("reconsider block {}:{}".format(self.invalidatedheight, self.invalidated))
        self.nodes[0].reconsiderblock(self.invalidated)

        def is_best_block_hash(node, hash):
            tip = node.getbestblockhash()
            return tip == hash

        wait_until(
            predicate=lambda: is_best_block_hash(self.nodes[0], self.chainAtiphash),
            timeout=10
        )

    def run_test(self):
        """Main test logic"""

        from pypoptools.pypopminer import MockMiner
        self.apm = MockMiner()

        self._invalidate_works()
        self._reconsider_works()


if __name__ == '__main__':
    PopReconsider().main()
