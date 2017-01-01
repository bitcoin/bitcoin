#!/usr/bin/env python3
# Copyright (c) 2016 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.


from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import *
from pprint import pprint

class SPVTest (BitcoinTestFramework):
    def __init__(self):
        super().__init__()
        self.setup_clean_chain = True
        self.num_nodes = 2

    def setup_network(self):
        self.nodes = []
        self.nodes.append(start_node(0, self.options.tmpdir, []))
        #connect to a local machine for debugging
        # url = "http://test:test@%s:%d" % ('127.0.0.1', 18332)
        # proxy = AuthServiceProxy(url)
        # proxy.url = url # store URL on proxy for info
        # self.nodes.append(proxy)

        self.nodes.append(start_node(1, self.options.tmpdir, ["-autorequestblocks=0", "-spv=1"]))
        connect_nodes_bi(self.nodes, 0, 1)

    def header_sync(self, node, height):
        timeout = 20
        while timeout > 0:
            for ct in node.getchaintips():
                if ct['status'] == "headers-only":
                    if ct['height'] == height:
                        return;
            time.sleep(1)
            timeout-=1
        assert(timeout>0)

    def active_chain_height(self, node):
        for ct in node.getchaintips():
            if ct['status'] == "active":
                return ct['height']
        return -1
        
    def wait_wallet_spv_sync(self, node, hash):
        timeout = 20
        while timeout > 0:
            if node.getwalletinfo()['spv_bestblock_hash'] == hash:
                return
        assert(timeout>0)

    def run_test(self):
        print("Mining blocks...")
        self.nodes[0].setmocktime(int(time.time()) - 10600)
        self.nodes[0].generate(31)
        self.nodes[0].setmocktime(0)
        self.nodes[0].generate(70)
        self.nodes[0].sendtoaddress(self.nodes[1].getnewaddress(), 2)
        self.nodes[0].generate(1)
        self.nodes[0].sendtoaddress(self.nodes[1].getnewaddress(), 3)
        self.nodes[0].generate(1)
        node0bbhash = self.nodes[0].getbestblockhash()
        self.header_sync(self.nodes[1], 103)
        assert(self.active_chain_height(self.nodes[1]) == 0)

        # spv sync must be possible due to the set -spv mode
        self.wait_wallet_spv_sync(self.nodes[1], node0bbhash)

        # best block should be available at this point due to spv sync
        bh = self.nodes[1].getblockheader(node0bbhash, True)
        assert(bh['validated'] == False)
        bh = self.nodes[1].getblock(node0bbhash, True)

        # confirmations must be -1 because we haven't validated the block
        assert(bh['confirmations'] == -1)

        # fork
        self.nodes[0].invalidateblock(node0bbhash)
        newblocks = self.nodes[0].generate(2)
        node0bbhash = self.nodes[0].getbestblockhash()

        # spy sync
        self.wait_wallet_spv_sync(self.nodes[1], node0bbhash)

        # make sure the transactions are still available
        lt = self.nodes[1].listtransactions()
        assert(len(lt) == 2)
        confForked = -1
        confNonForked = -1
        for t in lt:
            if t['amount'] == Decimal('3.00000000'):
                confForked = t['confirmations']
            if t['amount'] == Decimal('2.00000000'):
                confNonForked = t['confirmations']

        assert(confForked == 2)
        assert(confNonForked == 3)

        # enable auto-request of blocks (normal IBD)
        print("Restart node1...", end="", flush=True)
        stop_node(self.nodes[1], 1)
        self.nodes[1] = start_node(1, self.options.tmpdir, ["-autorequestblocks=0", "-spv=1"])
        connect_nodes_bi(self.nodes, 0, 1)
        print("done")
        self.nodes[1].setautorequestblocks(True)
        sync_blocks(self.nodes)
        lt = self.nodes[1].listtransactions()
        confForked = -1
        confNonForked = -1
        for t in lt:
            assert(t['validated'])
            if t['amount'] == Decimal('3.00000000'):
                confForked = t['confirmations']
            if t['amount'] == Decimal('2.00000000'):
                confNonForked = t['confirmations']

        assert(confForked == 2)
        assert(confNonForked == 3)

if __name__ == '__main__':
    SPVTest ().main ()
