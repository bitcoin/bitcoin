#!/usr/bin/env python3
# Copyright (c) 2016 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.


from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import *

class AuxiliaryBlockRequestTest (BitcoinTestFramework):
    def __init__(self):
        super().__init__()
        self.setup_clean_chain = True
        self.num_nodes = 2

    def setup_network(self):
        self.nodes = []
        self.nodes.append(start_node(0, self.options.tmpdir, []))
        self.nodes.append(start_node(1, self.options.tmpdir, ["-autorequestblocks=0"]))
        connect_nodes(self.nodes[0], 1)

    def run_test(self):
        print("Mining blocks...")
        self.nodes[0].generate(101)
        self.nodes[0].sendtoaddress(self.nodes[1].getnewaddress(), 1)
        time.sleep(5)
        ctps = self.nodes[1].getchaintips()
        headersheight = -1
        chaintipheight = -1
        for ct in ctps:
            if ct['status'] == "headers-only":
                headersheight = ct['height']
            if ct['status'] == "active":
                chaintipheight = ct['height']
        assert(headersheight == 101)
        assert(chaintipheight == 0)

        node0bbhash = self.nodes[0].getbestblockhash()
        # best block should not be validated, header must be available
        bh = self.nodes[1].getblockheader(node0bbhash, True)
        assert(bh['validated'] == False)
        # block must not be available
        try:
            bh = self.nodes[1].getblock(node0bbhash, True)
            raise AssertionError('Block must not be available')
        except JSONRPCException as e:
            assert(e.error['code']==-32603)

        # request best block (auxiliary)
        self.nodes[1].requestblocks("start", [node0bbhash])
        timeout = 20
        while timeout > 0:
            if self.nodes[1].requestblocks("status")['request_present'] == 0:
                break;
            time.sleep(1)
            timeout-=1
        assert(timeout>0)

        # block must now be available
        block = self.nodes[1].getblock(node0bbhash, True)
        assert(block['hash'] == node0bbhash)
        assert(block['validated'] == False)

        # enable auto-request of blocks
        self.nodes[1].setautorequestblocks(True)
        sync_blocks(self.nodes)

        ctps = self.nodes[1].getchaintips()
        # same block must now be available with mode validated=true
        block = self.nodes[1].getblock(node0bbhash, True)
        assert(block['hash'] == node0bbhash)
        assert(block['validated'] == True)

        chaintipheight = -1
        for ct in ctps:
            if ct['status'] == "active":
                chaintipheight = ct['height']
        assert(chaintipheight == 101)
        
if __name__ == '__main__':
    AuxiliaryBlockRequestTest ().main ()
