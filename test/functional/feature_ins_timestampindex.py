#!/usr/bin/env python3
# Copyright (c) 2014-2015 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

#
# Test timestampindex generation and fetching
#

import time

from test_framework.test_particl import ParticlTestFramework
from test_framework.util import *


class TimestampIndexTest(ParticlTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 4
        self.extra_args = [
            # Nodes 0/1 are "wallet" nodes
            ['-debug',],
            ['-debug','-timestampindex'],
            # Nodes 2/3 are used for testing
            ['-debug',],
            ['-debug','-timestampindex'],]

    def setup_network(self):
        self.add_nodes(self.num_nodes, extra_args=self.extra_args)
        self.start_nodes()

        connect_nodes(self.nodes[0], 1)
        connect_nodes(self.nodes[0], 2)
        connect_nodes(self.nodes[0], 3)

        self.is_network_split = False
        self.sync_all()

    def run_test(self):

        nodes = self.nodes

        # Stop staking
        for i in range(len(nodes)):
            nodes[i].reservebalance(True, 10000000)

        ro = nodes[0].extkeyimportmaster('abandon baby cabbage dad eager fabric gadget habit ice kangaroo lab absorb')
        assert(ro['account_id'] == 'aaaZf2qnNr5T7PWRmqgmusuu5ACnBcX2ev')
        assert(nodes[0].getwalletinfo()['total_balance'] == 100000)

        blockhashes = []
        self.stakeToHeight(1, False)
        blockhashes.append(nodes[0].getblockhash(1))
        time.sleep(3)

        self.stakeToHeight(2, False)
        blockhashes.append(nodes[0].getblockhash(2))
        time.sleep(3)

        self.stakeToHeight(3, False)
        blockhashes.append(nodes[0].getblockhash(3))
        self.sync_all()

        low = self.nodes[1].getblock(blockhashes[0])['time']
        high = low + 76

        print('Checking timestamp index...')
        hashes = self.nodes[1].getblockhashes(high, low)

        assert_equal(len(hashes), len(blockhashes))

        assert_equal(hashes, blockhashes)

        print('Passed\n')


if __name__ == '__main__':
    TimestampIndexTest().main()
