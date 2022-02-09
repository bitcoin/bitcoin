#!/usr/bin/env python3
# Copyright (c) 2019-2021 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the generation of UTXO snapshots using `dumptxoutset`.
"""

from test_framework.blocktools import COINBASE_MATURITY
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal, assert_raises_rpc_error

import hashlib
import os
from pathlib import Path


class DumptxoutsetTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1

    @staticmethod
    def check_output_file(path, is_human_readable, expected_digest):
        with open(str(path), 'rb') as f:
            content = f.read()

            if is_human_readable:
                # Normalise platform EOL to \n, while making sure any stray \n becomes a literal backslash+n to avoid a false positive
                # This ensures the platform EOL and only the platform EOL produces the expected hash
                linesep = os.linesep.encode('utf8')
                content = b'\n'.join(line.replace(b'\n', b'\\n') for line in content.split(linesep))

            digest = hashlib.sha256(content).hexdigest()
            # UTXO snapshot hash should be deterministic based on mocked time.
            assert_equal(digest, expected_digest)

    def test_dump_file(self, testname, params, expected_digest):
        node = self.nodes[0]

        self.log.info(testname)
        filename = testname + '_txoutset.dat'
        is_human_readable = not params.get('format') is None

        out = node.dumptxoutset(path=filename, **params)
        expected_path = Path(node.datadir) / self.chain / filename

        assert expected_path.is_file()

        assert_equal(out['coins_written'], 100)
        assert_equal(out['base_height'], 100)
        assert_equal(out['path'], str(expected_path))
        # Blockhash should be deterministic based on mocked time.
        assert_equal(
            out['base_hash'],
            '6fd417acba2a8738b06fee43330c50d58e6a725046c3d843c8dd7e51d46d1ed6')

        self.check_output_file(expected_path, is_human_readable, expected_digest)

        assert_equal(
            out['txoutset_hash'], 'd4b614f476b99a6e569973bf1c0120d88b1a168076f8ce25691fb41dd1cef149')
        assert_equal(out['nchaintx'], 101)

        self.log.info("Test that a path to an existing file will fail")
        assert_raises_rpc_error(
            -8, '{} already exists'.format(filename),  node.dumptxoutset, filename)

        if params.get('format') == ():
            with open(expected_path, 'r', encoding='utf-8') as f:
                content = f.readlines()
                sep = params.get('separator', ',')
                if params.get('show_header', True):
                    assert_equal(content.pop(0).rstrip(),
                        "#(blockhash 6fd417acba2a8738b06fee43330c50d58e6a725046c3d843c8dd7e51d46d1ed6 ) txid{s}vout{s}value{s}coinbase{s}height{s}scriptPubKey".format(s=sep))
                assert_equal(content[0].rstrip(),
                    "213ecbdfe837a2c8ffc0812da62d4de94efce8894c67e22ff658517ecf104e03{s}0{s}5000000000{s}1{s}81{s}76a9142b4569203694fc997e13f2c0a1383b9e16c77a0d88ac".format(s=sep))

    def run_test(self):
        """Test a trivial usage of the dumptxoutset RPC command."""
        node = self.nodes[0]
        mocktime = node.getblockheader(node.getblockhash(0))['time'] + 1
        node.setmocktime(mocktime)
        self.generate(node, COINBASE_MATURITY)

        self.test_dump_file('no_option',           {},
                            '7ae82c986fa5445678d2a21453bb1c86d39e47af13da137640c2b1cf8093691c')
        self.test_dump_file('all_data',            {'format': ()},
                            '5554c7d08c2f9aaacbbc66617eb59f13aab4b8c0574f4d8b12f728c60dc7d287')
        self.test_dump_file('partial_data_1',      {'format': ('txid',)},
                            'eaec3b56b285dcae610be0975d494befa5a6a130211dda0e1ec1ef2c4afa4389')
        self.test_dump_file('partial_data_order',  {'format': ('height', 'vout')},
                            '3e5d6d1cb44595eb7c9d13b3370d14b8826c0d81798c29339794623d4ab6091c')
        self.test_dump_file('partial_data_double', {'format': ('scriptPubKey', 'scriptPubKey')},
                            '0eb83a3bf6a7580333fdaf7fd6cebebe93096e032d49049229124ca699222919')
        self.test_dump_file('no_header',           {'format': (), 'show_header': False},
                            'ba85c1db5df6de80c783f2c9a617de4bd7e0e92125a0d318532218eaaed28bfa')
        self.test_dump_file('separator',           {'format': (), 'separator': ':'},
                            '3352b4db7a9f63629cf255c1a805241f1bee2b557e5f113993669cd3085e9b0f')
        self.test_dump_file('all_options',         {'format': (), 'show_header': False, 'separator': ':'},
                            '7df9588375f8bd01d0b6f902a55e086c2d0549c3f08f389baa28b398e987f8a2')

        # Other failing tests
        assert_raises_rpc_error(
            -8, 'unable to find item \'sample\'',  node.dumptxoutset, path='xxx', format=['sample'])

if __name__ == '__main__':
    DumptxoutsetTest().main()
