#!/usr/bin/env python3
# Copyright (c) 2024-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""
Test the -port option and its interactions with
-bind.
"""

from test_framework.test_framework import (
    BitcoinTestFramework,
)
from test_framework.util import (
    p2p_port,
)


class PortTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        # Avoid any -bind= on the command line.
        self.bind_to_localhost_only = False
        self.num_nodes = 1

    def run_test(self):
        node = self.nodes[0]
        node.has_explicit_bind = True
        port1 = p2p_port(self.num_nodes)
        port2 = p2p_port(self.num_nodes + 5)

        self.log.info("When starting with -port, bitcoind binds to it and uses port + 1 for an onion bind")
        with node.assert_debug_log(expected_msgs=[f'Bound to 0.0.0.0:{port1}', f'Bound to 127.0.0.1:{port1 + 1}']):
            self.restart_node(0, extra_args=["-listen", f"-port={port1}"])

        self.log.info("When specifying -port multiple times, only the last one is taken")
        with node.assert_debug_log(expected_msgs=[f'Bound to 0.0.0.0:{port2}', f'Bound to 127.0.0.1:{port2 + 1}'], unexpected_msgs=[f'Bound to 0.0.0.0:{port1}']):
            self.restart_node(0, extra_args=["-listen", f"-port={port1}", f"-port={port2}"])

        self.log.info("When specifying ports with both -port and -bind, the one from -port is ignored")
        with node.assert_debug_log(expected_msgs=[f'Bound to 0.0.0.0:{port2}'], unexpected_msgs=[f'Bound to 0.0.0.0:{port1}']):
            self.restart_node(0, extra_args=["-listen", f"-port={port1}", f"-bind=0.0.0.0:{port2}"])

        self.log.info("When -bind specifies no port, the values from -port and -bind are combined")
        with self.nodes[0].assert_debug_log(expected_msgs=[f'Bound to 0.0.0.0:{port1}']):
            self.restart_node(0, extra_args=["-listen", f"-port={port1}", "-bind=0.0.0.0"])

        self.log.info("When an onion bind specifies no port, the value from -port, incremented by 1, is taken")
        with self.nodes[0].assert_debug_log(expected_msgs=[f'Bound to 127.0.0.1:{port1 + 1}']):
            self.restart_node(0, extra_args=["-listen", f"-port={port1}", "-bind=127.0.0.1=onion"])

        self.log.info("Invalid values for -port raise errors")
        self.stop_node(0)
        node.extra_args = ["-listen", "-port=65536"]
        node.assert_start_raises_init_error(expected_msg="Error: Invalid port specified in -port: '65536'")
        node.extra_args = ["-listen", "-port=0"]
        node.assert_start_raises_init_error(expected_msg="Error: Invalid port specified in -port: '0'")


if __name__ == '__main__':
    PortTest(__file__).main()
