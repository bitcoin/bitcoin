#!/usr/bin/env python3
# Copyright (c) 2026-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""
Test that -bind causes outgoing connections to originate from the bound address.

Requires two routable addresses on the machine. To set up:
Linux:
  ifconfig lo:0 1.1.1.1/32 up && ifconfig lo:1 2.2.2.2/32 up
  ifconfig lo:0 down && ifconfig lo:1 down  # to remove
FreeBSD:
  ifconfig lo0 1.1.1.1/32 alias && ifconfig lo0 2.2.2.2/32 alias
  ifconfig lo0 1.1.1.1 -alias && ifconfig lo0 2.2.2.2 -alias  # to remove
"""

from test_framework.test_framework import BitcoinTestFramework, SkipTest
from test_framework.util import assert_equal, p2p_port

ADDR1 = '1.1.1.1'
ADDR2 = '2.2.2.2'


class BindOutgoingTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.bind_to_localhost_only = False
        self.num_nodes = 2

    def add_options(self, parser):
        parser.add_argument(
            "--ihave1111and2222", action='store_true', dest="ihave1111and2222",
            help=f"Run the test, assuming {ADDR1} and {ADDR2} are configured on the machine",
            default=False)

    def skip_test_if_missing_module(self):
        if not self.options.ihave1111and2222:
            raise SkipTest(
                f"To run this test make sure that {ADDR1} and {ADDR2} (routable addresses) "
                "are assigned to interfaces on this machine and rerun with --ihave1111and2222")

    def setup_network(self):
        self.extra_args = [
            [f'-bind={ADDR1}:{p2p_port(0)}'],
            [f'-bind={ADDR2}:{p2p_port(1)}'],
        ]
        self.setup_nodes()

    def run_test(self):
        self.log.info(f"Node 0 listening on {ADDR1}, node 1 bound to {ADDR2}")

        self.log.info("Connecting node 1 -> node 0")
        target = f"{ADDR1}:{p2p_port(0)}"
        self.nodes[1].addnode(target, "onetry")
        self.wait_until(lambda: len(self.nodes[0].getpeerinfo()) == 1)

        self.log.info("Checking that node 0 sees inbound from ADDR2")
        peers_0 = self.nodes[0].getpeerinfo()
        assert_equal(len(peers_0), 1)
        assert_equal(peers_0[0]['inbound'], True)
        source_ip = peers_0[0]['addr'].split(':')[0]
        assert_equal(source_ip, ADDR2)

        self.log.info("Checking that node 1 reports addrbind as ADDR2")
        peers_1 = self.nodes[1].getpeerinfo()
        assert_equal(len(peers_1), 1)
        assert_equal(peers_1[0]['inbound'], False)
        local_ip = peers_1[0]['addrbind'].split(':')[0]
        assert_equal(local_ip, ADDR2)

        self.log.info(f"Outbound connection correctly originated from {ADDR2}")


if __name__ == '__main__':
    BindOutgoingTest(__file__).main()
