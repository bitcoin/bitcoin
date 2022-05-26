#!/usr/bin/env python3
# Copyright (c) 2019-2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
import time
from test_framework.test_framework import SyscoinTestFramework
from test_framework.util import assert_equal, assert_raises_rpc_error, force_finish_mnsync, MAX_INITIAL_BROADCAST_DELAY
from decimal import Decimal
class NEVMDataTest(SyscoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 5
        self.rpc_timeout = 240

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()
        self.skip_if_no_bdb()

    def basic_nevm_data(self):
        # test relay with block
        self.stop_node(4)
        self.nodes[0].syscoincreaterawnevmblob('a37fca4d23174bf6aa93e965f9ff39c8a072a5a241f4e773f67edfa2b6d39edc', 'fdfdfdfdfcfcfcfcaafdfdfdfdfcfcfcfcaafdfdfdfdfcfcfcfcaafdfdfdfdfcfcfcfcaa')
        self.nodes[1].syscoincreaterawnevmblob('7c822321c4ce8a690efe74527773e6de8ad1034b6115bf4f5e81611e2ee3ad8e', 'fdfdfdfdfcfcfcfcaafdfdfdfdfcfcfcfcaafdfdfdfdfcfcfcfcaafdfdfdfdfcfcfcfcab')
        self.nodes[0].syscoincreaterawnevmblob('7745e43153db13aea8803c5ee2250a3a53ae9830abe206201d6622e2a2cf7d7a', 'fdfdfdfdfcfcfcfcaafdfdfdfdfcfcfcfcaafdfdfdfdfcfcfcfcaafdfdfdfdfcfcfcfcac')
        self.nodes[3].syscoincreaterawnevmblob('6404b2e7ed8e17c95c1af05104c15e9fe2854e7d9ec8ceb47bd4e017421ad2b6', 'fdfdfdfdfcfcfcfcaafdfdfdfdfcfcfcfcaafdfdfdfdfcfcfcfcaafdfdfdfdfcfcfcfcad')
        self.nodes[3].syscoincreaterawnevmblob('f40bd0d7f1b38686c799d32c854e11cc3c05d8f203080147f2d7587847da31af', 'fdfdfdfdfcfcfcfcaafdfdfdfdfcfcfcfcaafdfdfdfdfcfcfcfcaafdfdfdfdfcfcfcfcae')
        assert_raises_rpc_error(-20, 'NEVM data already exists in DB', self.nodes[3].syscoincreaterawnevmblob, '6404b2e7ed8e17c95c1af05104c15e9fe2854e7d9ec8ceb47bd4e017421ad2b6', 'aa')
        self.sync_mempools(self.nodes[0:3])
        self.generate(self.nodes[2], 5, sync_fun=self.no_op)
        self.sync_blocks(self.nodes[0:3])
        assert_raises_rpc_error(-20, 'NEVM data already exists in DB', self.nodes[0].syscoincreaterawnevmblob, '6404b2e7ed8e17c95c1af05104c15e9fe2854e7d9ec8ceb47bd4e017421ad2b6', 'aa')
        assert_equal(self.nodes[0].getnevmblobdata('7c822321c4ce8a690efe74527773e6de8ad1034b6115bf4f5e81611e2ee3ad8e', True)['data'], 'fdfdfdfdfcfcfcfcaafdfdfdfdfcfcfcfcaafdfdfdfdfcfcfcfcaafdfdfdfdfcfcfcfcab')
        assert_equal(self.nodes[1].getnevmblobdata('6404b2e7ed8e17c95c1af05104c15e9fe2854e7d9ec8ceb47bd4e017421ad2b6', True)['data'], 'fdfdfdfdfcfcfcfcaafdfdfdfdfcfcfcfcaafdfdfdfdfcfcfcfcaafdfdfdfdfcfcfcfcad')
        assert_equal(self.nodes[1].getnevmblobdata('7c822321c4ce8a690efe74527773e6de8ad1034b6115bf4f5e81611e2ee3ad8e', True)['data'], 'fdfdfdfdfcfcfcfcaafdfdfdfdfcfcfcfcaafdfdfdfdfcfcfcfcaafdfdfdfdfcfcfcfcab')
        # test relay before block creation
        self.nodes[0].syscoincreaterawnevmblob('cfaadb6afa319dade1f54e705767645babee71682aebd5f8f96cf7b5b73a7aee', 'fdfdfdfdfcfcfcfcaafdfdfdfdfcfcfcfcaafdfdfdfdfcfcfcfcaafdfdfdfdfcfcfcfcaa')
        self.nodes[0].syscoincreaterawnevmblob('00b6f7108755f153456b474bf05320610313632d85dadc045393f20028bfb76d', 'fdfdfdfdfcfcfcfcaafdfdfdfdfcfcfcfcaafdfdfdfdfcfcfcfcaafdfdfdfdfcfcfcfcaa')
        self.nodes[0].syscoincreaterawnevmblob('ef54962b45b39600fd68fc07e9103154d5e9038b60e2727cb91a9c0ff79042d4', 'fdfdfdfdfcfcfcfcaafdfdfdfdfcfcfcfcaafdfdfdfdfcfcfcfcaafdfdfdfdfcfcfcfcaa')
        self.nodes[0].syscoincreaterawnevmblob('2d089ea4dbc17e500feae746481ea0ec0efa0f16e63c6f35d3bab352382dc47c', 'fdfdfdfdfcfcfcfcaafdfdfdfdfcfcfcfcaafdfdfdfdfcfcfcfcaafdfdfdfdfcfcfcfcaa')
        self.nodes[0].syscoincreaterawnevmblob('5dbe4b6494d25e75ad9d763958f236b71ce60de60260931bee2befd32c08770c', 'fdfdfdfdfcfcfcfcaafdfdfdfdfcfcfcfcaafdfdfdfdfcfcfcfcaafdfdfdfdfcfcfcfcaa')
        self.nodes[0].syscoincreaterawnevmblob('8f79cab7cf8367d91a06b821c56877c1e0e018795117491a1daa7444437301b7', 'fdfdfdfdfcfcfcfcaafdfdfdfdfcfcfcfcaafdfdfdfdfcfcfcfcaafdfdfdfdfcfcfcfcaa')
        self.nodes[0].syscoincreaterawnevmblob('214bd9691a9012d773af838ecc4d948903ee5591b3b10eaa8c0dad663303d7a5', 'fdfdfdfdfcfcfcfcaafdfdfdfdfcfcfcfcaafdfdfdfdfcfcfcfcaafdfdfdfdfcfcfcfcaa')
        self.nodes[0].syscoincreaterawnevmblob('1ac036a1b200809cfba7df9a3faa345d5787166b0e761610129f8fd48a88534a', 'fdfdfdfdfcfcfcfcaafdfdfdfdfcfcfcfcaafdfdfdfdfcfcfcfcaafdfdfdfdfcfcfcfcaa')
        self.nodes[0].syscoincreaterawnevmblob('cae99142756fc5d59a9d835529085bd04e24526798ef1eced147d072bca5c857', 'fdfdfdfdfcfcfcfcaafdfdfdfdfcfcfcfcaafdfdfdfdfcfcfcfcaafdfdfdfdfcfcfcfcaa')
        self.nodes[0].syscoincreaterawnevmblob('c9e023414c0f376dc2c65cd7c4998a4c801a9142cf927cf97ca9969e4f03ebe8', 'fdfdfdfdfcfcfcfcaafdfdfdfdfcfcfcfcaafdfdfdfdfcfcfcfcaafdfdfdfdfcfcfcfcaa')
        self.sync_mempools(self.nodes[0:3])
        self.generate(self.nodes[2], 5, sync_fun=self.no_op)
        self.sync_blocks(self.nodes[0:3])
        self.restart_node(1, extra_args=["-reindex"])
        force_finish_mnsync(self.nodes[1])
        self.connect_nodes(1, 0)
        self.nodes[1].mockscheduler(MAX_INITIAL_BROADCAST_DELAY)
        self.sync_blocks(self.nodes[0:3])
        assert_equal(self.nodes[1].getnevmblobdata('7c822321c4ce8a690efe74527773e6de8ad1034b6115bf4f5e81611e2ee3ad8e', True)['data'], 'fdfdfdfdfcfcfcfcaafdfdfdfdfcfcfcfcaafdfdfdfdfcfcfcfcaafdfdfdfdfcfcfcfcab')
        assert_equal(self.nodes[1].getnevmblobdata('6404b2e7ed8e17c95c1af05104c15e9fe2854e7d9ec8ceb47bd4e017421ad2b6', True)['data'], 'fdfdfdfdfcfcfcfcaafdfdfdfdfcfcfcfcaafdfdfdfdfcfcfcfcaafdfdfdfdfcfcfcfcad')
        assert_equal(self.nodes[1].getnevmblobdata('7c822321c4ce8a690efe74527773e6de8ad1034b6115bf4f5e81611e2ee3ad8e', True)['data'], 'fdfdfdfdfcfcfcfcaafdfdfdfdfcfcfcfcaafdfdfdfdfcfcfcfcaafdfdfdfdfcfcfcfcab')

        self.start_node(4)
        force_finish_mnsync(self.nodes[4])
        self.connect_nodes(4, 0)
        self.nodes[4].mockscheduler(MAX_INITIAL_BROADCAST_DELAY)
        self.sync_blocks()
        assert_equal(self.nodes[4].getnevmblobdata('7c822321c4ce8a690efe74527773e6de8ad1034b6115bf4f5e81611e2ee3ad8e', True)['data'], 'fdfdfdfdfcfcfcfcaafdfdfdfdfcfcfcfcaafdfdfdfdfcfcfcfcaafdfdfdfdfcfcfcfcab')
        assert_equal(self.nodes[4].getnevmblobdata('6404b2e7ed8e17c95c1af05104c15e9fe2854e7d9ec8ceb47bd4e017421ad2b6', True)['data'], 'fdfdfdfdfcfcfcfcaafdfdfdfdfcfcfcfcaafdfdfdfdfcfcfcfcaafdfdfdfdfcfcfcfcad')
        assert_equal(self.nodes[4].getnevmblobdata('7c822321c4ce8a690efe74527773e6de8ad1034b6115bf4f5e81611e2ee3ad8e', True)['data'], 'fdfdfdfdfcfcfcfcaafdfdfdfdfcfcfcfcaafdfdfdfdfcfcfcfcaafdfdfdfdfcfcfcfcab')

    def mock_forward(self, delta):
        self.mock_time += delta
        self.nodes[0].setmocktime(self.mock_time)
        self.nodes[1].setmocktime(self.mock_time)
        self.nodes[2].setmocktime(self.mock_time)
        self.nodes[3].setmocktime(self.mock_time)

    def run_test(self):
        self.generate(self.nodes[0], 10)
        self.sync_blocks()
        self.generate(self.nodes[1], 10)
        self.sync_blocks()
        self.generate(self.nodes[3], 102)
        self.sync_blocks()
        #self.mock_time = int(time.time())
        #self.mock_forward(5)
        self.basic_nevm_data()


if __name__ == '__main__':
    NEVMDataTest().main()
