#!/usr/bin/env python3
# Copyright (c) 2014-2019 The Bitcoin Core developers
# Copyright (c) 2019-2020 Xenios SEZC
# https://www.veriblock.org
# Distributed under the MIT software license, see the accompanying
# file LICENSE or http://www.opensource.org/licenses/mit-license.php.

"""
Start 2 nodes.
Mine 100 blocks on node0.
Disconnect node[1].
node[0] endorses block 100 (fork A tip).
node[0] mines pop tx in block 101 (fork A tip)

Pop is disabled before block 200 therefore can't handle Pop data
"""

from test_framework.pop import endorse_block, create_endorsed_chain, mine_until_pop_active
from test_framework.authproxy import JSONRPCException
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    connect_nodes,
    disconnect_nodes, assert_equal, assert_raises_rpc_error,
)


class PopActivate(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 2
        self.extra_args = [["-txindex"], ["-txindex"]]

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()
        self.skip_if_no_pypoptools()

    def setup_network(self):
        self.setup_nodes()

    def get_best_block(self, node):
        hash = node.getbestblockhash()
        return node.getblock(hash)

    def _cannot_endorse(self):
        self.log.warning("starting _cannot_endorse()")

        # node0 start with 100 blocks
        self.nodes[0].generate(nblocks=100)
        self.nodes[0].waitforblockheight(100)
        assert self.get_best_block(self.nodes[0])['height'] == 100
        self.log.info("node0 mined 100 blocks")

        # endorse block 100 (fork A tip)
        addr0 = self.nodes[0].getnewaddress()
        self.log.info('Should not accept POP data before activation block height')
        assert_raises_rpc_error(-1, 'POP protocol is not active. Current=100, activation height=200', lambda: endorse_block(self.nodes[0], self.apm, 100, addr0))

        self.log.warning("_cannot_endorse() succeeded!")

    def _can_endorse(self):
        self.log.warning("starting _can_endorse()")
        connect_nodes(self.nodes[0], 1)
        self.sync_all()
        disconnect_nodes(self.nodes[1], 0)

        mine_until_pop_active(self.nodes[0])
        lastblock = self.nodes[0].getblockcount()

        # endorse block 200 (fork A tip)
        addr0 = self.nodes[0].getnewaddress()
        txid = endorse_block(self.nodes[0], self.apm, lastblock, addr0)
        self.log.info("node0 endorsed block {} (fork A tip)".format(lastblock))
        # mine pop tx on node0
        self.nodes[0].generate(nblocks=1)
        tip = self.get_best_block(self.nodes[0])
        self.log.info("node0 tip is {}".format(tip['height']))

        self.nodes[1].generate(nblocks=250)
        tip2 = self.get_best_block(self.nodes[1])
        self.log.info("node1 tip is {}".format(tip2['height']))

        connect_nodes(self.nodes[0], 1)
        self.sync_all()
        bestblocks = [self.get_best_block(x) for x in self.nodes]
        assert_equal(bestblocks[0]['hash'], bestblocks[1]['hash'])
        self.log.info("all nodes switched to common block")

        for i in range(len(bestblocks)):
            assert bestblocks[i]['height'] == tip['height'], \
                "node[{}] expected to select shorter chain ({}) with higher pop score\n" \
                "but selected longer chain ({})".format(i, tip['height'], bestblocks[i]['height'])

        self.log.info("all nodes selected fork A as best chain")
        self.log.warning("_can_endorse() succeeded!")

    def run_test(self):
        """Main test logic"""
        from pypoptools.pypopminer import MockMiner
        self.apm = MockMiner()
        self._cannot_endorse()
        self.restart_node(0)
        self._can_endorse()

if __name__ == '__main__':
    PopActivate().main()
