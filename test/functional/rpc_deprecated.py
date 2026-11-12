#!/usr/bin/env python3
# Copyright (c) 2017-2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test deprecation of RPC calls."""
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_raises_rpc_error, assert_equal

class DeprecatedRpcTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 3
        self.setup_clean_chain = True
        self.extra_args = [[], ["-deprecatedrpc=permissive_bool"], ["-deprecatedrpc=service"]]

    def run_test(self):
        # This test should be used to verify correct behaviour of deprecated
        # RPC methods with and without the -deprecatedrpc flags. For example:
        #
        # In set_test_params:
        # self.extra_args = [[], ["-deprecatedrpc=generate"]]
        #
        # In run_test:
        # self.log.info("Test generate RPC")
        # assert_raises_rpc_error(-32, 'The wallet generate rpc method is deprecated', self.nodes[0].rpc.generate, 1)
        # self.generate(self.nodes[1], 1)
        # self.log.info("No tested deprecated RPC methods")
        self.test_permissive_bool()
        self.test_malformed_deprecated_service_rawtransactions()

    def test_permissive_bool(self):
        self.log.info("Test -deprecatedrpc=permissive_bool")
        self.generate(self.nodes[0], 1)

        self.log.info("Node 0 (strict): JSON boolean accepted, legacy types rejected")
        assert_equal([], self.nodes[0].protx("list", "valid", True))
        assert_equal([], self.nodes[0].protx("list", "valid", False))
        assert_raises_rpc_error(-8, 'detailed must be a JSON boolean. Pass -deprecatedrpc=permissive_bool to allow legacy boolean parsing.', self.nodes[0].protx, "list", "valid", 1)
        assert_raises_rpc_error(-8, 'detailed must be a JSON boolean. Pass -deprecatedrpc=permissive_bool to allow legacy boolean parsing.', self.nodes[0].protx, "list", "valid", "1")
        assert_raises_rpc_error(-8, 'detailed must be a JSON boolean. Pass -deprecatedrpc=permissive_bool to allow legacy boolean parsing.', self.nodes[0].protx, "list", "valid", 0)
        assert_raises_rpc_error(-8, 'detailed must be a JSON boolean. Pass -deprecatedrpc=permissive_bool to allow legacy boolean parsing.', self.nodes[0].protx, "list", "valid", "0")
        assert_raises_rpc_error(-8, 'detailed must be a JSON boolean. Pass -deprecatedrpc=permissive_bool to allow legacy boolean parsing.', self.nodes[0].protx, "list", "valid", "true")
        assert_raises_rpc_error(-8, 'detailed must be a JSON boolean. Pass -deprecatedrpc=permissive_bool to allow legacy boolean parsing.', self.nodes[0].protx, "list", "valid", "false")
        assert_raises_rpc_error(-8, 'detailed must be a JSON boolean. Pass -deprecatedrpc=permissive_bool to allow legacy boolean parsing.', self.nodes[0].protx, "list", "valid", "yes")
        assert_raises_rpc_error(-8, 'detailed must be a JSON boolean. Pass -deprecatedrpc=permissive_bool to allow legacy boolean parsing.', self.nodes[0].protx, "list", "valid", "no")

        self.log.info("Node 1 (permissive): JSON boolean and legacy types accepted")
        assert_equal([], self.nodes[1].protx("list", "valid", True))
        assert_equal([], self.nodes[1].protx("list", "valid", 1))
        assert_equal([], self.nodes[1].protx("list", "valid", "1"))
        assert_equal([], self.nodes[1].protx("list", "valid", False))
        assert_equal([], self.nodes[1].protx("list", "valid", 0))
        assert_equal([], self.nodes[1].protx("list", "valid", "0"))
        assert_equal([], self.nodes[1].protx("list", "valid", "true"))
        assert_equal([], self.nodes[1].protx("list", "valid", "false"))
        assert_equal([], self.nodes[1].protx("list", "valid", "yes"))
        assert_equal([], self.nodes[1].protx("list", "valid", "no"))

        self.log.info("Node 1 (permissive): invalid values still rejected")
        assert_raises_rpc_error(-8, "detailed must be true, false, yes, no, 1 or 0 (not '2')", self.nodes[1].protx, "list", "valid", 2)
        assert_raises_rpc_error(-8, "detailed must be true, false, yes, no, 1 or 0 (not 'notabool')", self.nodes[1].protx, "list", "valid", "notabool")

    def test_malformed_deprecated_service_rawtransactions(self):
        self.log.info("Test malformed provider payloads do not crash -deprecatedrpc=service")
        node = self.nodes[2]
        block_count = node.getblockcount()
        for tx_hex, payload_field in [
            ("03000100000000000000020000", "proRegTx"),  # ProRegTx with only nVersion=0 payload
            ("03000200000000000000020000", "proUpServTx"),  # ProUpServTx with only nVersion=0 payload
        ]:
            decoded = node.decoderawtransaction(tx_hex)
            assert payload_field not in decoded
            assert_equal(decoded["extraPayload"], "0000")
            assert_equal(node.getblockcount(), block_count)

if __name__ == '__main__':
    DeprecatedRpcTest().main()
