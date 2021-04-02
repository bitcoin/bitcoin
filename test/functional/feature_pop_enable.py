#!/usr/bin/env python3
# Copyright (c) 2014-2019 The Bitcoin Core developers
# Copyright (c) 2019-2020 Xenios SEZC
# https://www.veriblock.org
# Distributed under the MIT software license, see the accompanying
# file LICENSE or http://www.opensource.org/licenses/mit-license.php.

"""

"""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_raises_rpc_error


class PopEnable(BitcoinTestFramework):
    def set_test_params(self):
        self.chain = 'detregtest'
        self.setup_clean_chain = True
        self.num_nodes = 1

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()
        self.skip_if_no_pypoptools()

    def setup_network(self):
        self.setup_nodes()
        self.nodes[0].setmocktime(1617025247)

    def _check_pop_not_enabled(self):
        current_height = self.nodes[0].getblockcount()
        assert_raises_rpc_error(-1,
                                'POP protocol is not enabled. Current={}, bootstrap height=1000'.format(current_height),
                                self.nodes[0].getpopdatabyheight, current_height)

    def _check_pop_enabled(self):
        current_height = self.nodes[0].getblockcount()
        self.nodes[0].getpopdatabyheight(current_height)

    def _check_pop_not_active(self):
        current_height = self.nodes[0].getblockcount()
        assert_raises_rpc_error(-1,
                                'POP protocol is not active. Current={}, activation height=1200'.format(current_height),
                                self.nodes[0].submitpopvbk, self.vbk_block)

    def _check_pop_active(self):
        self.nodes[0].submitpopvbk(self.vbk_block)

    def run_test(self):
        from pypoptools.pypopminer import MockMiner
        self.vbk_block = MockMiner().mineVbkBlocks(1).toVbkEncodingHex()

        node = self.nodes[0]
        mock_address = "bcrt1quc5k7w4692g0t0sfxc9xgcc25rzngu55zsfwp0"

        # height 0
        self._check_pop_not_enabled()

        node.generatetoaddress(nblocks=100, address=mock_address)
        # height 100
        self._check_pop_not_enabled()

        node.generatetoaddress(nblocks=400, address=mock_address)
        # height 500
        self._check_pop_not_enabled()

        node.generatetoaddress(nblocks=499, address=mock_address)
        # height 999
        self._check_pop_not_enabled()

        node.generatetoaddress(nblocks=1, address=mock_address)
        # height 1000
        self._check_pop_enabled()
        self._check_pop_not_active()

        node.generatetoaddress(nblocks=100, address=mock_address)
        # height 1100
        self._check_pop_enabled()
        self._check_pop_not_active()

        node.generatetoaddress(nblocks=99, address=mock_address)
        # height 1199
        self._check_pop_enabled()
        self._check_pop_not_active()

        node.generatetoaddress(nblocks=1, address=mock_address)
        # height 1200
        self._check_pop_enabled()
        self._check_pop_active()


if __name__ == '__main__':
    PopEnable().main()
