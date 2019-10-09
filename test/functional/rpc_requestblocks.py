#!/usr/bin/env python3
# Copyright (c) 2017 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.


from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import *

class RequestBlockRequestTest (BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 2
        self.extra_args = [[], ['-autorequestblocks=0']]

    def run_test(self):
        self.nodes[0].generate(50)
        timeout = 20
        ctps = self.nodes[1].getchaintips()
        while timeout > 0:
            ctps = self.nodes[1].getchaintips()
            headerHeightReached = False
            for ct in ctps:
                if ct['status'] == "headers-only":
                    if ct['height'] == 50:
                        headerHeightReached = True
                if ct['status'] == "active":
                    assert(ct['height'] == 0)
            time.sleep(1)
            timeout-=1
            if headerHeightReached == True:
                break
        assert(timeout>0)

        node0bbhash = self.nodes[0].getbestblockhash()
        # best block should not be validated, header must be available
        bh = self.nodes[1].getblockheader(node0bbhash, True)

        assert_equal(bh['validated'], False)
        # block must not be available
        assert_raises_rpc_error(-1, "Block not found on disk", self.nodes[1].getblock, node0bbhash)

        # request best block (auxiliary)
        self.nodes[1].requestblocks("add", [node0bbhash])
        timeout = 20
        while timeout > 0:
            if self.nodes[1].requestblocks("status")['count'] == 0:
                break
            time.sleep(1)
            timeout-=1
        assert(timeout>0)

        # block must now be available
        block = self.nodes[1].getblock(node0bbhash, True)
        assert_equal(block['hash'], node0bbhash)
        assert_equal(block['validated'], False)

        #prevblock must not be available
        assert_raises_rpc_error(-1, "Block not found on disk", self.nodes[1].getblock, block['previousblockhash'])

if __name__ == '__main__':
    RequestBlockRequestTest().main()
