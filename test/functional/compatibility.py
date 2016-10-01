#!/usr/bin/env python3
# Copyright (c) 2016 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    start_nodes,
    stop_nodes,
    connect_nodes_bi,
    mine_large_block,
    sync_chain,
    sync_mempools,
    assert_equal,
)

'''
Script to check if our datadir can be read by older versions and vice versa
'''

class CompatibilityTest(BitcoinTestFramework):

    def __init__(self):
        super().__init__()
        self.setup_clean_chain = True
        self.num_nodes = 2
        self.extra_args = [['-usehd=0']] * self.num_nodes

    def add_options(self, parser):
        parser.add_option("--oldbinary", dest="oldbinary",
                          default=None,
                          help="old bitcoind binary for compatibility testing")

    def setup_network(self):
        assert(self.options.oldbinary is not None)
        self.binary = [None, self.options.oldbinary]
        self.nodes = start_nodes(self.num_nodes, self.options.tmpdir, self.extra_args, binary=self.binary)
        connect_nodes_bi(self.nodes, 0, 1)

    def send(self, seed):
        sender = seed % 2
        self.nodes[sender].sendtoaddress(self.nodes[not sender].getnewaddress(), 0.125)

    def run_test(self):
        # Create some blocks and transactions:
        self.nodes[1].generate(14 * 3)
        sync_chain(self.nodes)
        self.nodes[0].generate(14 * 3)
        self.nodes[0].generate(100)
        [mine_large_block(self.nodes[0]) for _ in range(3)]
        sync_chain(self.nodes)
        [mine_large_block(self.nodes[1]) for _ in range(3)]
        sync_chain(self.nodes)

        [self.send(seed) for seed in range(10)]
        self.nodes[0].generate(1)
        sync_chain(self.nodes)
        self.nodes[1].generate(1)
        sync_chain(self.nodes)
        #assert_equal([0, 0], [n.getmempoolinfo()["size"] for n in self.nodes])

        INFO_CALLS = [
            "getblockchaininfo",
            "gettxoutsetinfo",
            "getmininginfo",
            "getwalletinfo",
            "getinfo",
        ]
        info_dict = {}
        for info in INFO_CALLS:
            info_dict[info] = [getattr(n, info)() for n in self.nodes]

        # Swap binaries
        stop_nodes(self.nodes)
        self.binary.reverse()
        self.nodes = start_nodes(self.num_nodes, self.options.tmpdir, self.extra_args, binary=self.binary)
        connect_nodes_bi(self.nodes, 0, 1)
        for info in INFO_CALLS:
            print("Check {}".format(info))
            # old info should match new info, even though the binary changed
            assert_equal_ignore_some(info_dict[info][0], getattr(self.nodes[0], info)())
            assert_equal_ignore_some(info_dict[info][1], getattr(self.nodes[1], info)())


def assert_equal_ignore_some(dict1, dict2):
    # ignore entries which are unstable across restarts or
    # not supported by both versions
    IGNORE = [
        'currentblockweight',
        'currentblocksize',
        'currentblocktx',
        'bip9_softforks', # Older nodes may not be aware of all softforks
        'softforks',
        'hash_serialized', # This may have changed due to 7848?
        'keypoolsize', # ?
        'errors', # releases don't display the pre-release-warning
        #'warnings',
        'genproclimit', # Removed in 7507
        'generate',
        'testnet', # Removed in 8921
        'subversion',
        'version',
        'protocolversion',
    ]
    for d in [dict1, dict2]:
        for ig in IGNORE:
            d.pop(ig) if ig in d else None
    assert_equal(dict1, dict2)

if __name__ == '__main__':
    CompatibilityTest().main()
