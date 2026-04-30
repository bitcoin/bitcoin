#!/usr/bin/env python3
# Copyright (c) 2014-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test longpolling with getblocktemplate."""

import random
import threading

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal, get_rpc_proxy
from test_framework.wallet import MiniWallet


class LongpollThread(threading.Thread):
    def __init__(self, node):
        threading.Thread.__init__(self)
        # query current longpollid
        template = node.getblocktemplate({'rules': ['segwit']})
        self.longpollid = template['longpollid']
        # create a new connection to the node, we can't use the same
        # connection from two threads
        self.node = get_rpc_proxy(node.url, node.index, timeout=600, coveragedir=node.coverage_dir)
        self.exception = None

    def run(self):
        try:
            self.node.getblocktemplate({'longpollid': self.longpollid, 'rules': ['segwit']})
        except Exception as e:
            self.exception = e


class GetBlockTemplateThread(threading.Thread):
    def __init__(self, node):
        threading.Thread.__init__(self)
        self.node = get_rpc_proxy(node.url, node.index, timeout=600, coveragedir=node.coverage_dir)
        self.exception = None

    def run(self):
        try:
            self.node.getblocktemplate({'rules': ['segwit']})
        except Exception as e:
            self.exception = e


class GetBlockTemplateLPTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 2
        self.extra_args = [["-rpcthreads=2"], []]
        self.supports_cli = False

    def run_test(self):
        self.log.info("Warning: this test will take about 70 seconds in the best case. Be patient.")
        self.log.info("Test that longpollid doesn't change between successive getblocktemplate() invocations if nothing else happens")
        self.generate(self.nodes[0], 10)
        template = self.nodes[0].getblocktemplate({'rules': ['segwit']})
        longpollid = template['longpollid']
        template2 = self.nodes[0].getblocktemplate({'rules': ['segwit']})
        assert_equal(template2['longpollid'], longpollid)

        self.log.info("Test that longpoll waits if we do nothing")
        thr = LongpollThread(self.nodes[0])
        with self.nodes[0].assert_debug_log(["ThreadRPCServer method=getblocktemplate"], timeout=3):
            thr.start()
        # check that thread still lives
        thr.join(5)  # wait 5 seconds or until thread exits
        assert thr.is_alive()

        self.miniwallet = MiniWallet(self.nodes[0])
        self.log.info("Test that longpoll will terminate if another node generates a block")
        self.generate(self.nodes[1], 1)  # generate a block on another node
        # check that thread will exit now that new transaction entered mempool
        thr.join(5)  # wait 5 seconds or until thread exits
        assert not thr.is_alive()
        assert_equal(thr.exception, None)

        self.log.info("Test that longpoll will terminate if we generate a block ourselves")
        thr = LongpollThread(self.nodes[0])
        with self.nodes[0].assert_debug_log(["ThreadRPCServer method=getblocktemplate"], timeout=3):
            thr.start()
        self.generate(self.nodes[0], 1)  # generate a block on own node
        thr.join(5)  # wait 5 seconds or until thread exits
        assert not thr.is_alive()
        assert_equal(thr.exception, None)

        self.log.info("Test that introducing a new transaction into the mempool will terminate the longpoll")
        thr = LongpollThread(self.nodes[0])
        with self.nodes[0].assert_debug_log(["ThreadRPCServer method=getblocktemplate"], timeout=3):
            thr.start()
        # generate a transaction and submit it
        self.miniwallet.send_self_transfer(from_node=random.choice(self.nodes))
        # after one minute, every 10 seconds the mempool is probed, so in 80 seconds it should have returned
        thr.join(60 + 20)
        assert not thr.is_alive()
        assert_equal(thr.exception, None)

        self.log.info("Test that shutdown does not process queued getblocktemplate requests")
        longpoll_threads = [LongpollThread(self.nodes[0]) for _ in range(2)]
        for thr in longpoll_threads:
            thr.start()
        for thr in longpoll_threads:
            thr.join(5)
            assert thr.is_alive()

        queued_gbt_thread = GetBlockTemplateThread(self.nodes[0])
        queued_gbt_thread.start()
        queued_gbt_thread.join(1)
        assert queued_gbt_thread.is_alive()

        self.nodes[0].process.terminate()
        self.nodes[0].wait_until_stopped()
        for thr in [*longpoll_threads, queued_gbt_thread]:
            thr.join(5)
            assert not thr.is_alive()
        assert_equal(queued_gbt_thread.exception.error['code'], -9)

if __name__ == '__main__':
    GetBlockTemplateLPTest(__file__).main()
