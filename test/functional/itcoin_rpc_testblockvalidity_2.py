#!/usr/bin/env python3
# Copyright (c) 2020-2021 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
'''Test testblockvalidity rpc.'''
from test_framework.util import assert_raises_rpc_error
from test_framework.test_framework_itcoin import BaseItcoinTest

# import itcoin's miner
from test_framework.itcoin_abs_import import import_miner
miner = import_miner()


class TestItcoinBlockValidityTest(BaseItcoinTest):

    def set_test_params(self):
        self.num_nodes = 2
        self.signet_num_signers = 2
        self.signet_num_signatures = 2
        super().set_test_params()

    def _log_test_success(self):
        self.log.info("Test success")

    def run_test(self):
        self.test_block_with_incomplete_signature_gives_error()

    def test_block_with_incomplete_signature_gives_error(self):
        """Check that a block with an incomplete signature gives error."""
        self.log.info("Check that a block with an incomplete signature gives error.")
        signet_challenge = self.signet_challenge
        args0 = self.node(0).args
        args1 = self.node(1).args
        block, _, _ = miner.do_generate_next_block(args0)

        block_hex = block.serialize().hex()
        # default value of check_signet_solution is True
        assert_raises_rpc_error(-25, "signet block signature validation failure", self.nodes[0].testblockvalidity, block_hex)
        # check bad-signet-blksig with check_signet_solution=True
        assert_raises_rpc_error(-25, "signet block signature validation failure", self.nodes[0].testblockvalidity, block_hex, True)

        # check block is valid with check_signet_solution=False
        result = self.nodes[0].testblockvalidity(block_hex, False)
        assert result is None
        self._log_test_success()


if __name__ == '__main__':
    TestItcoinBlockValidityTest().main()
