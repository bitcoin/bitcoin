#!/usr/bin/env python3
# Copyright (c) 2020-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test asmap config argument for ASN-based IP bucketing.

Verify node behaviour and debug log when launching bitcoind with different
`-asmap` and `-noasmap` arg values, including absolute and relative paths, and
with missing and unparseable files.

The tests are order-independent.

"""
import os
import shutil

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal

ASMAP = 'src/test/data/asmap.raw' # path to unit test skeleton asmap
VERSION = 'fec61fa21a9f46f3b17bdcd660d7f4cd90b966aad3aec593c99b35f0aca15853'

def expected_messages(filename):
    return [f'Opened asmap file "{filename}" (59 bytes) from disk',
            f'Using asmap version {VERSION} for IP bucketing']

class AsmapTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        # Do addrman checks on all operations and use deterministic addrman
        self.extra_args = [["-checkaddrman=1", "-test=addrman"]]

    def fill_addrman(self, node_id):
        """Add 2 tried addresses to the addrman, followed by 2 new addresses."""
        for addr, tried in [[0, True], [1, True], [2, False], [3, False]]:
            self.nodes[node_id].addpeeraddress(address=f"101.{addr}.0.0", tried=tried, port=8333)

    def test_without_asmap_arg(self):
        self.log.info('Test bitcoind with no -asmap arg passed')
        self.stop_node(0)
        with self.node.assert_debug_log(['Using /16 prefix for IP bucketing']):
            self.start_node(0)

    def test_noasmap_arg(self):
        self.log.info('Test bitcoind with -noasmap arg passed')
        self.stop_node(0)
        with self.node.assert_debug_log(['Using /16 prefix for IP bucketing']):
            self.start_node(0, ["-noasmap"])

    def test_asmap_with_absolute_path(self):
        self.log.info('Test bitcoind -asmap=<absolute path>')
        self.stop_node(0)
        filename = os.path.join(self.datadir, 'my-map-file.map')
        shutil.copyfile(self.asmap_raw, filename)
        with self.node.assert_debug_log(expected_messages(filename)):
            self.start_node(0, [f'-asmap={filename}'])
        os.remove(filename)

    def test_asmap_with_relative_path(self):
        self.log.info('Test bitcoind -asmap=<relative path>')
        self.stop_node(0)
        name = 'ASN_map'
        filename = os.path.join(self.datadir, name)
        shutil.copyfile(self.asmap_raw, filename)
        with self.node.assert_debug_log(expected_messages(filename)):
            self.start_node(0, [f'-asmap={name}'])
        os.remove(filename)

    def test_unspecified_asmap(self):
        msg = "Error: -asmap requires a file path. Use -asmap=<file>."
        for arg in ['-asmap', '-asmap=']:
            self.log.info(f'Test bitcoind {arg} (and no filename specified)')
            self.stop_node(0)
            self.node.assert_start_raises_init_error(extra_args=[arg], expected_msg=msg)

    def test_asmap_interaction_with_addrman_containing_entries(self):
        self.log.info("Test bitcoind -asmap restart with addrman containing new and tried entries")
        self.stop_node(0)
        self.start_node(0, [f"-asmap={self.asmap_raw}", "-checkaddrman=1", "-test=addrman"])
        self.fill_addrman(node_id=0)
        self.restart_node(0, [f"-asmap={self.asmap_raw}", "-checkaddrman=1", "-test=addrman"])
        with self.node.assert_debug_log(
            expected_msgs=[
                "CheckAddrman: new 2, tried 2, total 4 started",
                "CheckAddrman: completed",
            ]
        ):
            self.node.getnodeaddresses()  # getnodeaddresses re-runs the addrman checks

    def test_asmap_with_missing_file(self):
        self.log.info('Test bitcoind -asmap with missing map file')
        self.stop_node(0)
        msg = f"Error: Could not find asmap file \"{self.datadir}{os.sep}missing\""
        self.node.assert_start_raises_init_error(extra_args=['-asmap=missing'], expected_msg=msg)

    def test_empty_asmap(self):
        self.log.info('Test bitcoind -asmap with empty map file')
        self.stop_node(0)
        empty_asmap = os.path.join(self.datadir, "ip_asn.map")
        with open(empty_asmap, "w") as f:
            f.write("")
        msg = f"Error: Could not parse asmap file \"{empty_asmap}\""
        self.node.assert_start_raises_init_error(extra_args=[f'-asmap={empty_asmap}'], expected_msg=msg)
        os.remove(empty_asmap)

    def test_asmap_health_check(self):
        self.log.info('Test bitcoind -asmap logs ASMap Health Check with basic stats')
        msg = "ASMap Health Check: 4 clearnet peers are mapped to 3 ASNs with 0 peers being unmapped"
        with self.node.assert_debug_log(expected_msgs=[msg]):
            self.start_node(0, extra_args=[f'-asmap={self.asmap_raw}'])
        raw_addrman = self.node.getrawaddrman()
        asns = []
        for _, entries in raw_addrman.items():
            for _, entry in entries.items():
                asn = entry['mapped_as']
                if asn not in asns:
                    asns.append(asn)
        assert_equal(len(asns), 3)

    def run_test(self):
        self.node = self.nodes[0]
        self.datadir = self.node.chain_path
        base_dir = self.config["environment"]["SRCDIR"]
        self.asmap_raw = os.path.join(base_dir, ASMAP)

        self.test_without_asmap_arg()
        self.test_noasmap_arg()
        self.test_asmap_with_absolute_path()
        self.test_asmap_with_relative_path()
        self.test_unspecified_asmap()
        self.test_asmap_interaction_with_addrman_containing_entries()
        self.test_asmap_with_missing_file()
        self.test_empty_asmap()
        self.test_asmap_health_check()


if __name__ == '__main__':
    AsmapTest(__file__).main()
