#!/usr/bin/env python3
# Copyright (c) 2014-2015 The Bitcoin Core developers
# Copyright (c) 2015-2016 The Bitcoin Unlimited developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.


from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import *


class ThinBlockTest(BitcoinTestFramework):
    def __init__(self):
        self.rep = False
        BitcoinTestFramework.__init__(self)

    def setup_chain(self):
        print ("Initializing test directory " + self.options.tmpdir)
        initialize_chain_clean(self.options.tmpdir, 2)

    def setup_network(self, split=False):
        node_opts = [
            "-rpcservertimeout=0",
            "-debug=thin",
            "-use-thinblocks=1",
            "-excessiveblocksize=6000000",
            "-blockprioritysize=6000000",
            "-blockmaxsize=6000000"]

        self.nodes = [
            start_node(0, self.options.tmpdir, node_opts),
            start_node(1, self.options.tmpdir, node_opts)
        ]
        interconnect_nodes(self.nodes)
        self.is_network_split = False
        self.sync_all()

    def run_test(self):
        self.nodes[0].generate(30)
        self.sync_all()

        gni = self.nodes[0].getnetworkinfo()
        assert "thinblockstats" in gni

        tbs = gni["thinblockstats"]
        assert "enabled" in tbs and tbs["enabled"]

        assert set(tbs) == {"enabled",
                            "summary",
                            "mempool_limiter",
                            "inbound_percent",
                            "outbound_percent",
                            "response_time",
                            "validation_time",
                            "outbound_bloom_filters",
                            "inbound_bloom_filters",
                            "rerequested"}


if __name__ == '__main__':
    ThinBlockTest().main()
