#!/usr/bin/env python3
# Copyright (c) 2014-2019 The Bitcoin Core developers
# Copyright (c) 2019-2020 Xenios SEZC
# https://www.veriblock.org
# Distributed under the MIT software license, see the accompanying
# file LICENSE or http://www.opensource.org/licenses/mit-license.php.

"""

"""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.pop import endorse_block
from test_framework.util import assert_raises_rpc_error, connect_nodes, disconnect_nodes


class PopEnableSync(BitcoinTestFramework):
    def set_test_params(self):
        self.chain = 'detregtest'
        self.setup_clean_chain = True
        self.num_nodes = 2

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()
        self.skip_if_no_pypoptools()

    def setup_network(self):
        self.setup_nodes()
        for node in self.nodes:
            node.setmocktime(1617025247)

    def run_test(self):
        from pypoptools.pypopminer import MockMiner
        apm = MockMiner()
        mock_address = "bcrt1quc5k7w4692g0t0sfxc9xgcc25rzngu55zsfwp0"

        self.nodes[0].generatetoaddress(nblocks=900, address=mock_address)
        assert self.nodes[0].getblockcount() == 900
        assert_raises_rpc_error(-1, 'POP protocol is not enabled. Current=900, bootstrap height=1000',
                                endorse_block, self.nodes[0], apm, 800, mock_address)

        connect_nodes(self.nodes[0], 1)
        self.sync_blocks()
        assert self.nodes[1].getblockcount() == 900
        assert_raises_rpc_error(-1, 'POP protocol is not enabled. Current=900, bootstrap height=1000',
                                endorse_block, self.nodes[1], apm, 800, mock_address)

        disconnect_nodes(self.nodes[0], 1)
        self.nodes[0].generatetoaddress(nblocks=200, address=mock_address)
        assert self.nodes[0].getblockcount() == 1100
        assert_raises_rpc_error(-1, 'POP protocol is not active. Current=1100, activation height=1200',
                                endorse_block, self.nodes[0], apm, 1100, mock_address)

        connect_nodes(self.nodes[0], 1)
        self.sync_blocks()
        assert self.nodes[1].getblockcount() == 1100
        assert_raises_rpc_error(-1, 'POP protocol is not active. Current=1100, activation height=1200',
                                endorse_block, self.nodes[1], apm, 1100, mock_address)

        disconnect_nodes(self.nodes[0], 1)
        self.nodes[0].generatetoaddress(nblocks=199, address=mock_address)
        assert self.nodes[0].getblockcount() == 1299
        atv_id = endorse_block(self.nodes[0], apm, 1100, mock_address)
        self.nodes[0].generatetoaddress(nblocks=1, address=mock_address)
        assert self.nodes[0].getblockcount() == 1300

        connect_nodes(self.nodes[0], 1)
        self.sync_all()
        assert self.nodes[1].getblockcount() == 1300
        block_hash = self.nodes[1].getblockhash(1300)
        pop_data = self.nodes[1].getblock(block_hash)['pop']['data']
        assert atv_id in pop_data['atvs']


if __name__ == '__main__':
    PopEnableSync().main()
