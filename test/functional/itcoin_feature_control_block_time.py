#!/usr/bin/env python3
"""Test that the itcoin signet python miner can generate blocks at arbitrary times"""

import datetime
import time
from typing import Any, Dict

# Import the test primitives
from test_framework.test_framework_itcoin import BaseItcoinTest, utc_timestamp
from test_framework.util import assert_equal

# import itcoin's miner
from test_framework.itcoin_abs_import import import_miner

miner = import_miner()


class MinerControlsBlockTimeTest(BaseItcoinTest):
    def set_test_params(self):
        self.num_nodes = 1
        self.signet_num_signers = 1
        self.signet_num_signatures = 1
        super().set_test_params()

    def mine_and_getblockchaininfo(self, blocktime: int) -> Dict[str, Any]:
        args0 = self.node(0).args
        block, _, _ = miner.do_generate_next_block(args0, blocktime=blocktime)
        signed_block = miner.do_sign_block(args0, block, self.signet_challenge)
        miner.do_propagate_block(args0, signed_block)
        return self.nodes[0].getblockchaininfo()

    def run_test(self):
        self.log.info("Test that the itcoin signet python miner can generate blocks at arbitrary times")

        # =============================
        # Verify the initial state
        # - 0 blocks
        # - genesis time (2020-09-01 00:00:00)
        genesis_blocktime = utc_timestamp(2020, 9, 1, 0, 0, 0)
        blockchaininfo = self.nodes[0].getblockchaininfo()
        assert_equal(blockchaininfo["blocks"], 0)
        assert_equal(blockchaininfo["time"], genesis_blocktime)

        # =============================
        # Mine the 1st block at 2000-01-01 00:00:00
        # - its date will be changed to 2020-09-01 00:00:01
        # - we will be in Initial Block Download
        attempted_blocktime = utc_timestamp(2000, 1, 1, 0, 0, 0)
        adjusted_blocktime = utc_timestamp(2020, 9, 1, 0, 0, 1)
        blockchaininfo = self.mine_and_getblockchaininfo(attempted_blocktime)
        assert_equal(blockchaininfo["blocks"], 1)
        assert_equal(blockchaininfo["time"], adjusted_blocktime)
        assert_equal(blockchaininfo["initialblockdownload"], True)

        # =============================
        # Mine the 2nd block at 2021-01-01 00:00:00
        # - the block time will be exactly the one we submitted
        # - we will still be in Initial Block Download
        blocktime = utc_timestamp(2021, 1, 1, 0, 0, 0)
        blockchaininfo = self.mine_and_getblockchaininfo(blocktime)
        assert_equal(blockchaininfo["blocks"], 2)
        assert_equal(blockchaininfo["time"], blocktime)
        assert_equal(blockchaininfo["initialblockdownload"], True)

        # =============================
        # Mine the 3rd block at current date
        # - the block time will exactly be the one we submitted
        # - we will no longer be in Initial Block Download
        blocktime = int(datetime.datetime.today().timestamp())
        blockchaininfo = self.mine_and_getblockchaininfo(blocktime)
        assert_equal(blockchaininfo["blocks"], 3)
        assert_equal(blockchaininfo["time"], blocktime)
        assert_equal(blockchaininfo["initialblockdownload"], False)


if __name__ == '__main__':
    MinerControlsBlockTimeTest().main()
