#!/usr/bin/env python3
# Copyright (c) 2014-2019 The Bitcoin Core developers
# Copyright (c) 2019-2020 Xenios SEZC
# https://www.veriblock.org
# Distributed under the MIT software license, see the accompanying
# file LICENSE or http://www.opensource.org/licenses/mit-license.php.

"""
Test with multiple nodes, and multiple PoP endorsements, checking to make sure nodes stay in sync.
"""

from test_framework.pop import KEYSTONE_INTERVAL, endorse_block, sync_pop_mempools, mine_until_pop_active
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    connect_nodes,
    sync_mempools,
)


class PoPSync(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 3
        self.extra_args = [["-txindex"], ["-txindex"], ["-txindex"]]

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()
        self.skip_if_no_pypoptools()

    def setup_network(self):
        self.setup_nodes()
        mine_until_pop_active(self.nodes[0])

        for i in range(self.num_nodes - 1):
            connect_nodes(self.nodes[i + 1], i)
        self.sync_all()

    def _check_pop_sync(self):
        self.log.info("running _check_pop_sync()")
        height = self.nodes[0].getblockcount()
        topheight = height + 52
        addr0 = self.nodes[0].getnewaddress()
        addr1 = self.nodes[1].getnewaddress()
        addr2 = self.nodes[2].getnewaddress()

        while height < topheight:
            self.nodes[0].generate(nblocks=1)
            # endorse every block
            self.nodes[2].waitforblockheight(height)
            self.log.info("node2 endorsing block {} by miner {}".format(height, addr2))
            node2_txid = endorse_block(self.nodes[2], self.apm, height, addr2)

            # endorse each keystone
            if height % KEYSTONE_INTERVAL == 0:
                self.nodes[0].waitforblockheight(height)
                self.log.info("node0 endorsing block {} by miner {}".format(height, addr0))
                node0_txid = endorse_block(self.nodes[0], self.apm, height, addr0)

                self.nodes[1].waitforblockheight(height)
                self.log.info("node1 endorsing block {} by miner {}".format(height, addr1))
                node1_txid = endorse_block(self.nodes[1], self.apm, height, addr1)

                # wait until node[1] gets relayed pop tx
                self.sync_all(self.nodes, timeout=20)
                self.log.info("transactions relayed")

                # mine a block on node[1] with this pop tx
                containingblockhash = self.nodes[1].generate(nblocks=1)[0]
                containingblock = self.nodes[1].getblock(containingblockhash)
                self.log.info("node1 mined containing block={}".format(containingblock['hash']))
                self.nodes[0].waitforblockheight(containingblock['height'])
                self.nodes[2].waitforblockheight(containingblock['height'])
                self.log.info("node0 and node2 got containing block over p2p")

                # assert that all txids exist in this block
                for node in self.nodes:
                    self.assert_atvs_in_node(node, containingblockhash, [node0_txid, node1_txid, node2_txid])

                # assert that node height matches
                assert self.nodes[0].getblockcount() == self.nodes[1].getblockcount() == self.nodes[2].getblockcount()

            height += 1
        self.log.info("success! _check_pop_sync()")

    def assert_atvs_in_node(self, node, containingblockhash, expected):
        block = node.getblock(containingblockhash)
        atvs = block['pop']['data']['atvs']

        for atv in expected:
            assert atv in atvs, "containing block {} does not contain ATV {}".format(containingblockhash, atv)

    def run_test(self):
        """Main test logic"""

        self.sync_all(self.nodes)

        from pypoptools.pypopminer import MockMiner
        self.apm = MockMiner()

        self._check_pop_sync()


if __name__ == '__main__':
    PoPSync().main()
