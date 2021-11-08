#!/usr/bin/env python3
# Copyright (c) 2021 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the scanblocks rpc call."""
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal, assert_raises_rpc_error

class scanblocksTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 2
        self.extra_args = [["-blockfilterindex=1"],[]]

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        # send 1.0, mempool only
        addr_1 = self.nodes[0].getnewaddress()
        self.nodes[0].sendtoaddress(addr_1, 1.0)

        # send 1.0, mempool only
        self.nodes[0].sendtoaddress("mkS4HXoTYWRTescLGaUTGbtTTYX5EjJyEE", 1.0) # childkey 5 of tpubD6NzVbkrYhZ4WaWSyoBvQwbpLkojyoTZPRsgXELWz3Popb3qkjcJyJUGLnL4qHHoQvao8ESaAstxYSnhyswJ76uZPStJRJCTKvosUCJZL5B

        # mine a block and assure that the mined blockhash is in the filterresult
        blockhash = self.nodes[0].generate(1)[0]
        out = self.nodes[0].scanblocks("start", ["addr("+addr_1+")"])
        assert(blockhash in out['relevant_blocks'])
        assert_equal(self.nodes[0].getblockheader(blockhash)['height'], out['to_height'])
        assert_equal(0, out['from_height'])

        # mine another block
        blockhash_new = self.nodes[0].generate(1)[0]

        # make sure the blockhash is not in the filter result if we set the start_height to the just mined block (unlikely to hit a false positive)
        assert(blockhash not in self.nodes[0].scanblocks("start", ["addr("+addr_1+")"], self.nodes[0].getblockheader(blockhash_new)['height'])['relevant_blocks'])

        # make sure the blockhash is present when using the first mined block as start_height
        assert(blockhash in self.nodes[0].scanblocks("start", ["addr("+addr_1+")"], self.nodes[0].getblockheader(blockhash)['height'])['relevant_blocks'])

        # also test the stop height
        assert(blockhash in self.nodes[0].scanblocks("start", ["addr("+addr_1+")"], self.nodes[0].getblockheader(blockhash)['height'], self.nodes[0].getblockheader(blockhash)['height'])['relevant_blocks'])

        # use the stop_height to exclude the relevent block
        assert(blockhash not in self.nodes[0].scanblocks("start", ["addr("+addr_1+")"], 0, self.nodes[0].getblockheader(blockhash)['height']-1)['relevant_blocks'])

        # make sure the blockhash is present when using the first mined block as start_height
        assert(blockhash in self.nodes[0].scanblocks("start", [{"desc": "pkh(tpubD6NzVbkrYhZ4WaWSyoBvQwbpLkojyoTZPRsgXELWz3Popb3qkjcJyJUGLnL4qHHoQvao8ESaAstxYSnhyswJ76uZPStJRJCTKvosUCJZL5B/*)", "range": [0,100]}], self.nodes[0].getblockheader(blockhash)['height'])['relevant_blocks'])

        # test node with disabled blockfilterindex
        assert_raises_rpc_error(-1, "Index is not enabled for filtertype basic", self.nodes[1].scanblocks, "start", ["addr("+addr_1+")"])

        # test unknown filtertype
        assert_raises_rpc_error(-5, "Unknown filtertype", self.nodes[0].scanblocks, "start", ["addr("+addr_1+")"], 0, 10, "extended")

        # test invalid start_height
        assert_raises_rpc_error(-1, "Invalid start_height", self.nodes[0].scanblocks, "start", ["addr("+addr_1+")"], 100000000)

        # test invalid stop_height
        assert_raises_rpc_error(-1, "Invalid stop_height", self.nodes[0].scanblocks, "start", ["addr("+addr_1+")"], 10, 0)
        assert_raises_rpc_error(-1, "Invalid stop_height", self.nodes[0].scanblocks, "start", ["addr("+addr_1+")"], 10, 100000000)

        # test accessing the status (must be empty)
        assert_equal(self.nodes[0].scanblocks("status"), None)

        # test aborting the current scan (there is no, must return false)
        assert_equal(self.nodes[0].scanblocks("abort"), False)

        # test invalid command
        assert_raises_rpc_error(-8, "Invalid action argument", self.nodes[0].scanblocks, "foobar")

if __name__ == '__main__':
    scanblocksTest().main()
