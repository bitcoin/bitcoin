#!/usr/bin/env python3
# Copyright (c) 2026-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test private broadcast jobs against proxies that cannot carry them: one that offers no
authentication, and one that is not Tor.

The harness is in test_framework/privbcast.py.
"""

from test_framework.privbcast import (
    PrivbcastJobTest,
    drain_socks_commands,
)
from test_framework.socks5 import Command
from test_framework.util import assert_equal


class P2PPrivateBroadcastProxy(PrivbcastJobTest):
    def set_test_params(self):
        super().set_test_params()

    def run_test(self):
        self.setup_wallet()
        self.test_socks_auth_required()
        self.test_not_tor_proxy()

    def test_socks_auth_required(self):
        self.log.info("A proxy offering no authentication: the job resolves nothing and sends nothing")
        # Nothing here depends on time, so a faster clock.
        self.start_proxy({"a.seed.": ["9.0.0.1"]}, {}, proxy_authenticates=False, time_divisor=50)
        # A job offers only username and password, which this proxy lacks, so the proxy answers
        # "no acceptable method": no RESOLVE, so no candidate and no CONNECT.
        tx = self.wallet.create_self_transfer()
        job, report = self.run_job(tx)
        assert_equal((job["state"], job["announced"]), ("done", False))
        assert_equal(report["discovery"]["exit_path_candidates"], 0)
        assert_equal(report["summary"]["connections"], 0)
        assert_equal(drain_socks_commands(self.socks5_server), [])
        self.stop_proxy()

    def test_not_tor_proxy(self):
        self.log.info("An authenticating proxy that is not Tor: no candidates, nothing dialled")
        self.start_proxy({"a.seed.": ["9.0.1.1"]}, {}, tor=False, time_divisor=50)
        # Every RESOLVE is refused as an unsupported command, so there is no candidate to CONNECT to.
        tx = self.wallet.create_self_transfer()
        job, report = self.run_job(tx)
        assert_equal((job["state"], job["announced"]), ("done", False))
        assert_equal(report["discovery"]["exit_path_candidates"], 0)
        assert_equal(report["summary"]["connections"], 0)
        commands = drain_socks_commands(self.socks5_server)
        assert_equal([c.cmd for c in commands], [Command.RESOLVE] * 4)
        assert_equal(self.connects, {})
        self.stop_proxy()


if __name__ == '__main__':
    P2PPrivateBroadcastProxy(__file__).main()
