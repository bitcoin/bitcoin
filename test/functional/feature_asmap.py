#!/usr/bin/env python3
# Copyright (c) 2020-2021 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test asmap config argument for ASN-based IP bucketing.

Verify node behaviour and debug log when launching bitcoind in these cases:

1. `bitcoind` with no -asmap arg, using /16 prefix for IP bucketing

2. `bitcoind -asmap=<absolute path>`, using the unit test skeleton asmap

3. `bitcoind -asmap=<relative path>`, using the unit test skeleton asmap

4. `bitcoind -asmap/-asmap=` with no file specified, using the default asmap

5. `bitcoind -asmap` restart with an addrman containing new and tried entries

6. `bitcoind -asmap` with no file specified and a missing default asmap file

7. `bitcoind -asmap` with an empty (unparsable) default asmap file

The tests are order-independent.

"""
import os
import shutil

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal

DEFAULT_ASMAP_FILENAME = 'ip_asn.map' # defined in src/init.cpp
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

    def test_default_asmap(self):
        shutil.copyfile(self.asmap_raw, self.default_asmap)
        for arg in ['-asmap', '-asmap=']:
            self.log.info(f'Test bitcoind {arg} (using default map file)')
            self.stop_node(0)
            with self.node.assert_debug_log(expected_messages(self.default_asmap)):
                self.start_node(0, [arg])
        os.remove(self.default_asmap)

    def test_asmap_interaction_with_addrman_containing_entries(self):
        self.log.info("Test bitcoind -asmap restart with addrman containing new and tried entries")
        self.stop_node(0)
        shutil.copyfile(self.asmap_raw, self.default_asmap)
        self.start_node(0, ["-asmap", "-checkaddrman=1", "-test=addrman"])
        self.fill_addrman(node_id=0)
        self.restart_node(0, ["-asmap", "-checkaddrman=1", "-test=addrman"])
        with self.node.assert_debug_log(
            expected_msgs=[
                "CheckAddrman: new 2, tried 2, total 4 started",
                "CheckAddrman: completed",
            ]
        ):
            self.node.getnodeaddresses()  # getnodeaddresses re-runs the addrman checks
        os.remove(self.default_asmap)

    def test_default_asmap_with_missing_file(self):
        self.log.info('Test bitcoind -asmap with missing default map file')
        self.stop_node(0)
        msg = f"Error: Could not find asmap file \"{self.default_asmap}\""
        self.node.assert_start_raises_init_error(extra_args=['-asmap'], expected_msg=msg)

    def test_empty_asmap(self):
        self.log.info('Test bitcoind -asmap with empty map file')
        self.stop_node(0)
        with open(self.default_asmap, "w", encoding="utf-8") as f:
            f.write("")
        msg = f"Error: Could not parse asmap file \"{self.default_asmap}\""
        self.node.assert_start_raises_init_error(extra_args=['-asmap'], expected_msg=msg)
        os.remove(self.default_asmap)

    def test_asmap_health_check(self):
        self.log.info('Test bitcoind -asmap logs ASMap Health Check with basic stats')
        shutil.copyfile(self.asmap_raw, self.default_asmap)
        msg = "ASMap Health Check: 4 clearnet peers are mapped to 3 ASNs with 0 peers being unmapped"
        with self.node.assert_debug_log(expected_msgs=[msg]):
            self.start_node(0, extra_args=['-asmap'])
        raw_addrman = self.node.getrawaddrman()
        asns = []
        for _, entries in raw_addrman.items():
            for _, entry in entries.items():
                asn = entry['mapped_as']
                if asn not in asns:
                    asns.append(asn)
        assert_equal(len(asns), 3)
        os.remove(self.default_asmap)

    def run_test(self):
        self.node = self.nodes[0]
        self.datadir = self.node.chain_path
        self.default_asmap = os.path.join(self.datadir, DEFAULT_ASMAP_FILENAME)
        base_dir = self.config["environment"]["SRCDIR"]
        self.asmap_raw = os.path.join(base_dir, ASMAP)

        self.test_without_asmap_arg()
        self.test_asmap_with_absolute_path()
        self.test_asmap_with_relative_path()
        self.test_default_asmap()
        self.test_asmap_interaction_with_addrman_containing_entries()
        self.test_default_asmap_with_missing_file()
        self.test_empty_asmap()
        self.test_asmap_health_check()


if __name__ == '__main__':
    AsmapTest(__file__).main()
