#!/usr/bin/env python3
# Copyright (c) 2019-2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test RPC misc output."""
import xml.etree.ElementTree as ET

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_raises_rpc_error,
    assert_equal,
    assert_greater_than,
    assert_greater_than_or_equal,
)

from test_framework.authproxy import JSONRPCException


class RpcMiscTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.supports_cli = False

    def run_test(self):
        node = self.nodes[0]

        self.log.info("test CHECK_NONFATAL")
        assert_raises_rpc_error(
            -1,
            'Internal bug detected: \'request.params[9].get_str() != "trigger_internal_bug"\'',
            lambda: node.echo(arg9='trigger_internal_bug'),
        )

        self.log.info("test getmemoryinfo")
        memory = node.getmemoryinfo()['locked']
        assert_greater_than(memory['used'], 0)
        assert_greater_than(memory['free'], 0)
        assert_greater_than(memory['total'], 0)
        # assert_greater_than_or_equal() for locked in case locking pages failed at some point
        assert_greater_than_or_equal(memory['locked'], 0)
        assert_greater_than(memory['chunks_used'], 0)
        assert_greater_than(memory['chunks_free'], 0)
        assert_equal(memory['used'] + memory['free'], memory['total'])

        self.log.info("test mallocinfo")
        try:
            mallocinfo = node.getmemoryinfo(mode="mallocinfo")
            self.log.info('getmemoryinfo(mode="mallocinfo") call succeeded')
            tree = ET.fromstring(mallocinfo)
            assert_equal(tree.tag, 'malloc')
        except JSONRPCException:
            self.log.info('getmemoryinfo(mode="mallocinfo") not available')
            assert_raises_rpc_error(-8, 'mallocinfo is only available when compiled with glibc 2.10+', node.getmemoryinfo, mode="mallocinfo")

        assert_raises_rpc_error(-8, "unknown mode foobar", node.getmemoryinfo, mode="foobar")

        self.log.info("test logging")
        assert_equal(node.logging()['qt'], True)
        node.logging(exclude=['qt'])
        assert_equal(node.logging()['qt'], False)
        node.logging(include=['qt'])
        assert_equal(node.logging()['qt'], True)

        self.log.info("test echoipc (testing spawned process in multiprocess build)")
        assert_equal(node.echoipc("hello"), "hello")

        self.log.info("test getindexinfo")
        # Without any indices running the RPC returns an empty object
        assert_equal(node.getindexinfo(), {})

        # Restart the node with indices and wait for them to sync
        self.restart_node(0, ["-txindex", "-blockfilterindex", "-coinstatsindex"])
        self.wait_until(lambda: all(i["synced"] for i in node.getindexinfo().values()))

        # Returns a list of all running indices by default
        values = {"synced": True, "best_block_height": 200}
        assert_equal(
            node.getindexinfo(),
            {
                "txindex": values,
                "basic block filter index": values,
                "coinstatsindex": values,
            }
        )
        # Specifying an index by name returns only the status of that index
        for i in {"txindex", "basic block filter index", "coinstatsindex"}:
            assert_equal(node.getindexinfo(i), {i: values})

        # Specifying an unknown index name returns an empty result
        assert_equal(node.getindexinfo("foo"), {})


if __name__ == '__main__':
    RpcMiscTest().main()
