#!/usr/bin/env python3
# Copyright (c) 2014-2016 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the -alertnotify option."""
import os

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal, wait_until

class ForkNotifyTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 2

    def setup_network(self):
        self.alert_filename = os.path.join(self.options.tmpdir, "alert.txt")
        self.extra_args = [["-alertnotify=echo %%s >> %s" % self.alert_filename],
                           ["-blockversion=211"]]
        super().setup_network()

    def run_test(self):
        # Mine 51 up-version blocks. -alertnotify should trigger on the 51st.
        self.nodes[1].generate(51)
        self.sync_all()

        # Give bitcoind 10 seconds to write the alert notification
        wait_until(lambda: os.path.isfile(self.alert_filename) and os.path.getsize(self.alert_filename), timeout=10)

        with open(self.alert_filename, 'r', encoding='utf8') as f:
            alert_text = f.read()

        # Mine more up-version blocks, should not get more alerts:
        self.nodes[1].generate(2)
        self.sync_all()

        with open(self.alert_filename, 'r', encoding='utf8') as f:
            alert_text2 = f.read()

        self.log.info("-alertnotify should not continue notifying for more unknown version blocks")
        assert_equal(alert_text, alert_text2)

if __name__ == '__main__':
    ForkNotifyTest().main()
