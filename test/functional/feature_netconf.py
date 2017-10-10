#!/usr/bin/env python3
# Copyright (c) 2017 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Tests the network-specific config file."""

import os
from test_framework.test_framework import BitcoinTestFramework

class NetConfTest(BitcoinTestFramework):
    def setup_chain(self):
        super().setup_chain()

        with open(os.path.join(self.options.tmpdir, "node0", "regtest", "network.conf"), "w", encoding="utf8") as f:
            f.write("uacomment=net\n")
            f.write("maxmempool=111\n")

        with open(os.path.join(self.options.tmpdir, "node0", "regtest", "alt-network.conf"), "w", encoding="utf8") as f:
            f.write("uacomment=altnet\n")
            f.write("uacomment=extra\n")
            f.write("maxmempool=222\n")

        with open(os.path.join(self.options.tmpdir, "node0", "bitcoin-altnet.conf"), 'a', encoding='utf8') as f:
            for l in open(os.path.join(self.options.tmpdir, "node0", "bitcoin.conf")):
                f.write(l)
            f.write("uacomment=mainalt\n")
            f.write("netconf=alt-network.conf\n")
            f.write("maxmempool=333\n")

        with open(os.path.join(self.options.tmpdir, "node0", "bitcoin.conf"), 'a', encoding='utf8') as f:
            f.write("uacomment=main\n")

    def set_test_params(self):
         self.setup_clean_chain = False
         self.num_nodes = 1

    def check_maxmempool(self, expected):
        mpinfo = self.nodes[0].getmempoolinfo()
        maxmem = mpinfo["maxmempool"]
        assert maxmem == expected*1000000, "Max mempool is %s M not %s M" % (maxmem/1000000, expected)

    def check_subversion(self, *expected):
        nwinfo = self.nodes[0].getnetworkinfo()
        subversion = nwinfo["subversion"]
        want = "(testnode0; " + "; ".join(expected) + ")/"
        assert subversion.endswith(want), "Subversion %s does not end with %s" % (subversion, want)

    def run_test(self):
        # expected behaviour:
        #   single option arguments should adopt the first seen value,
        #   in order of command line, bitcoin.conf, network.conf.
        #   tested using maxmempool setting.
        #
        #   multi option arguments should see all values, in the same
        #   order (command line, bitcoin.conf, network.conf). tested
        #   using uacomment sub-version.

        # by default, loads bitcoin.conf and network.conf
        self.check_subversion("main", "net")
        self.check_maxmempool(111)

        # if bitcoin.conf specifies a network conf, that gets loaded
        self.restart_node(0, ["-conf=bitcoin-altnet.conf"])
        self.check_subversion("mainalt", "altnet", "extra")
        self.check_maxmempool(333)

        # netconf on command line works
        self.restart_node(0, ["-netconf=alt-network.conf"])
        self.check_subversion("main", "altnet", "extra")
        self.check_maxmempool(222)

        # netconf on command line overrides specification in bitcoin.conf
        self.restart_node(0, ["-conf=bitcoin-altnet.conf", "-netconf=network.conf"])
        self.check_subversion("mainalt", "net")
        self.check_maxmempool(333)

        # if file doesn't exist, everything is fine
        self.restart_node(0, ["-netconf=no-network.conf"])
        self.check_subversion("main")
        self.check_maxmempool(300)

if __name__ == '__main__':
    NetConfTest().main()

