#!/usr/bin/env python3
# Copyright (c) 2014-2021 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test longpolling with getblocktemplate."""

import random
import threading

from test_framework.test_framework import SyscoinTestFramework
from test_framework.util import get_rpc_proxy
from test_framework.wallet import MiniWallet


class LongpollThread(threading.Thread):
    def __init__(self, node):
        threading.Thread.__init__(self)
        # query current longpollid
        template = node.getblocktemplate({'rules': ['segwit']})
        self.longpollid = template['longpollid']
        # create a new connection to the node, we can't use the same
        # connection from two threads
        self.node = get_rpc_proxy(node.url, 1, timeout=600, coveragedir=node.coverage_dir)

    def run(self):
        self.node.getblocktemplate({'longpollid': self.longpollid, 'rules': ['segwit']})

class GetBlockTemplateLPTest(SyscoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 2
        self.supports_cli = False

    def run_test(self):
        self.log.info("Warning: this test will take about 70 seconds in the best case. Be patient.")
        self.log.info("Test that longpollid doesn't change between successive getblocktemplate() invocations if nothing else happens")
        self.generate(self.nodes[0], 10)
        template = self.nodes[0].getblocktemplate({'rules': ['segwit']})
        longpollid = template['longpollid']
        template2 = self.nodes[0].getblocktemplate({'rules': ['segwit']})
        assert template2['longpollid'] == longpollid

        self.log.info("Test that longpoll waits if we do nothing")
        thr = LongpollThread(self.nodes[0])
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

        self.log.info("Test that longpoll will terminate if we generate a block ourselves")
        thr = LongpollThread(self.nodes[0])
        thr.start()
        self.generate(self.nodes[0], 1)  # generate a block on own node
        thr.join(5)  # wait 5 seconds or until thread exits
        assert not thr.is_alive()

        self.log.info("Test that introducing a new transaction into the mempool will terminate the longpoll")
        thr = LongpollThread(self.nodes[0])
        thr.start()
        # generate a transaction and submit it
        self.miniwallet.send_self_transfer(from_node=random.choice(self.nodes))
        # after one minute, every 10 seconds the mempool is probed, so in 80 seconds it should have returned
        thr.join(60 + 20)
        assert not thr.is_alive()

if __name__ == '__main__':
    GetBlockTemplateLPTest().main()
