#!/usr/bin/env python3
# Copyright (c) 2019-2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the generation of UTXO snapshots using `dumptxoutset`.
"""
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal, assert_raises_rpc_error

import hashlib
from pathlib import Path


class DumptxoutsetTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1

    def run_test(self):
        """Test a trivial usage of the dumptxoutset RPC command."""

        # format: test title, kwargs, file hash
        TESTS = [["no_option",      {},
                  'be032e5f248264ba08e11099ac09dbd001f6f87ffc68bf0f87043d8146d50664'],
                 ["all_data",       {"format": []},
                  '5554c7d08c2f9aaacbbc66617eb59f13aab4b8c0574f4d8b12f728c60dc7d287'],
                 ["partial_data_1", {"format": ["txid"]},
                  'eaec3b56b285dcae610be0975d494befa5a6a130211dda0e1ec1ef2c4afa4389'],
                 ["partial_data_order", {"format": ["height", "vout"]},
                  '3e5d6d1cb44595eb7c9d13b3370d14b8826c0d81798c29339794623d4ab6091c'],
                 ["partial_data_double", {"format": ["scriptPubKey", "scriptPubKey"]},
                  '0eb83a3bf6a7580333fdaf7fd6cebebe93096e032d49049229124ca699222919'],
                 ["no_header",      {"format": [], "show_header": False},
                  'ba85c1db5df6de80c783f2c9a617de4bd7e0e92125a0d318532218eaaed28bfa'],
                 ["separator",      {"format": [], "separator": ":"},
                  '3352b4db7a9f63629cf255c1a805241f1bee2b557e5f113993669cd3085e9b0f'],
                 ["all_options",    {"format": [], "show_header": False, "separator": ":"},
                  '7df9588375f8bd01d0b6f902a55e086c2d0549c3f08f389baa28b398e987f8a2']]

        node = self.nodes[0]
        mocktime = node.getblockheader(node.getblockhash(0))['time'] + 1
        node.setmocktime(mocktime)
        node.generate(100)

        for test in TESTS:
            self.log.info(test[0])
            test[1]["path"] = test[0]+'_txoutset.dat'
            out = node.dumptxoutset(**test[1])
            expected_path = Path(node.datadir) / self.chain / test[1]["path"]

            assert expected_path.is_file()

            assert_equal(out['coins_written'], 100)
            assert_equal(out['base_height'], 100)
            assert_equal(out['path'], str(expected_path))
            # Blockhash should be deterministic based on mocked time.
            assert_equal(
                out['base_hash'],
                '6fd417acba2a8738b06fee43330c50d58e6a725046c3d843c8dd7e51d46d1ed6')

            with open(str(expected_path), 'rb') as f:
                digest = hashlib.sha256(f.read()).hexdigest()
                # UTXO snapshot hash should be deterministic based on mocked time.
                assert_equal(digest, test[2])

            # Specifying a path to an existing file will fail.
            assert_raises_rpc_error(
                -8, '{} already exists'.format(test[1]["path"]),  node.dumptxoutset, test[1]["path"])

        # Other failing tests
        assert_raises_rpc_error(
            -8, 'unable to find item \'sample\'',  node.dumptxoutset, path='xxx', format=['sample'])


if __name__ == '__main__':
    DumptxoutsetTest().main()
