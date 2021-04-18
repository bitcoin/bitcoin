#!/usr/bin/env python3
# Copyright (c) 2020 The Widecoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test asmap config argument for ASN-based IP bucketing.

Verify node behaviour and debug log when launching widecoind in these cases:

1. `widecoind` with no -asmap arg, using /16 prefix for IP bucketing

2. `widecoind -asmap=<absolute path>`, using the unit test skeleton asmap

3. `widecoind -asmap=<relative path>`, using the unit test skeleton asmap

4. `widecoind -asmap/-asmap=` with no file specified, using the default asmap

5. `widecoind -asmap` with no file specified and a missing default asmap file

6. `widecoind -asmap` with an empty (unparsable) default asmap file

The tests are order-independent.

"""
import os
import shutil

from test_framework.test_framework import WidecoinTestFramework

DEFAULT_ASMAP_FILENAME = 'ip_asn.map' # defined in src/init.cpp
ASMAP = '../../src/test/data/asmap.raw' # path to unit test skeleton asmap
VERSION = 'fec61fa21a9f46f3b17bdcd660d7f4cd90b966aad3aec593c99b35f0aca15853'

def expected_messages(filename):
    return ['Opened asmap file "{}" (59 bytes) from disk'.format(filename),
            'Using asmap version {} for IP bucketing'.format(VERSION)]

class AsmapTest(WidecoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = False
        self.num_nodes = 1

    def test_without_asmap_arg(self):
        self.log.info('Test widecoind with no -asmap arg passed')
        self.stop_node(0)
        with self.node.assert_debug_log(['Using /16 prefix for IP bucketing']):
            self.start_node(0)

    def test_asmap_with_absolute_path(self):
        self.log.info('Test widecoind -asmap=<absolute path>')
        self.stop_node(0)
        filename = os.path.join(self.datadir, 'my-map-file.map')
        shutil.copyfile(self.asmap_raw, filename)
        with self.node.assert_debug_log(expected_messages(filename)):
            self.start_node(0, ['-asmap={}'.format(filename)])
        os.remove(filename)

    def test_asmap_with_relative_path(self):
        self.log.info('Test widecoind -asmap=<relative path>')
        self.stop_node(0)
        name = 'ASN_map'
        filename = os.path.join(self.datadir, name)
        shutil.copyfile(self.asmap_raw, filename)
        with self.node.assert_debug_log(expected_messages(filename)):
            self.start_node(0, ['-asmap={}'.format(name)])
        os.remove(filename)

    def test_default_asmap(self):
        shutil.copyfile(self.asmap_raw, self.default_asmap)
        for arg in ['-asmap', '-asmap=']:
            self.log.info('Test widecoind {} (using default map file)'.format(arg))
            self.stop_node(0)
            with self.node.assert_debug_log(expected_messages(self.default_asmap)):
                self.start_node(0, [arg])
        os.remove(self.default_asmap)

    def test_default_asmap_with_missing_file(self):
        self.log.info('Test widecoind -asmap with missing default map file')
        self.stop_node(0)
        msg = "Error: Could not find asmap file \"{}\"".format(self.default_asmap)
        self.node.assert_start_raises_init_error(extra_args=['-asmap'], expected_msg=msg)

    def test_empty_asmap(self):
        self.log.info('Test widecoind -asmap with empty map file')
        self.stop_node(0)
        with open(self.default_asmap, "w", encoding="utf-8") as f:
            f.write("")
        msg = "Error: Could not parse asmap file \"{}\"".format(self.default_asmap)
        self.node.assert_start_raises_init_error(extra_args=['-asmap'], expected_msg=msg)
        os.remove(self.default_asmap)

    def run_test(self):
        self.node = self.nodes[0]
        self.datadir = os.path.join(self.node.datadir, self.chain)
        self.default_asmap = os.path.join(self.datadir, DEFAULT_ASMAP_FILENAME)
        self.asmap_raw = os.path.join(os.path.dirname(os.path.realpath(__file__)), ASMAP)

        self.test_without_asmap_arg()
        self.test_asmap_with_absolute_path()
        self.test_asmap_with_relative_path()
        self.test_default_asmap()
        self.test_default_asmap_with_missing_file()
        self.test_empty_asmap()


if __name__ == '__main__':
    AsmapTest().main()
