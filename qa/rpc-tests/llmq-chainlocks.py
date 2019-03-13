#!/usr/bin/env python3
# Copyright (c) 2015-2018 The Dash Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.mininode import *
from test_framework.test_framework import DashTestFramework
from test_framework.util import *
from time import *

'''
llmq-chainlocks.py

Checks LLMQs based ChainLocks

'''

class LLMQChainLocksTest(DashTestFramework):
    def __init__(self):
        super().__init__(11, 10, [], fast_dip3_enforcement=True)

    def run_test(self):

        self.nodes[0].spork("SPORK_17_QUORUM_DKG_ENABLED", 0)
        self.nodes[0].spork("SPORK_19_CHAINLOCKS_ENABLED", 0)
        self.wait_for_sporks_same()

        for i in range(4):
            self.mine_quorum()

        # mine single block, wait for chainlock
        self.nodes[0].generate(1)
        self.wait_for_chainlock_tip_all_nodes()

        # mine many blocks, wait for chainlock
        self.nodes[0].generate(20)
        self.wait_for_chainlock_tip_all_nodes()

        # assert that all blocks up until the tip are chainlocked
        for h in range(1, self.nodes[0].getblockcount()):
            block = self.nodes[0].getblock(self.nodes[0].getblockhash(h))
            assert(block['chainlock'])

        # Isolate node, mine on another, and reconnect
        self.nodes[0].setnetworkactive(False)
        node0_tip = self.nodes[0].getbestblockhash()
        self.nodes[1].generate(5)
        self.wait_for_chainlock_tip(self.nodes[1])
        assert(self.nodes[0].getbestblockhash() == node0_tip)
        self.nodes[0].setnetworkactive(True)
        connect_nodes(self.nodes[0], 1)
        self.nodes[1].generate(1)
        self.wait_for_chainlock(self.nodes[0], self.nodes[1].getbestblockhash())

        # Isolate node, mine on both parts of the network, and reconnect
        self.nodes[0].setnetworkactive(False)
        self.nodes[0].generate(5)
        self.nodes[1].generate(1)
        good_tip = self.nodes[1].getbestblockhash()
        self.wait_for_chainlock_tip(self.nodes[1])
        assert(not self.nodes[0].getblock(self.nodes[0].getbestblockhash())["chainlock"])
        self.nodes[0].setnetworkactive(True)
        connect_nodes(self.nodes[0], 1)
        self.nodes[1].generate(1)
        self.wait_for_chainlock(self.nodes[0], self.nodes[1].getbestblockhash())
        assert(self.nodes[0].getblock(self.nodes[0].getbestblockhash())["previousblockhash"] == good_tip)
        assert(self.nodes[1].getblock(self.nodes[1].getbestblockhash())["previousblockhash"] == good_tip)

        # Keep node connected and let it try to reorg the chain
        good_tip = self.nodes[0].getbestblockhash()
        # Restart it so that it forgets all the chainlocks from the past
        stop_node(self.nodes[0], 0)
        self.nodes[0] = start_node(0, self.options.tmpdir, self.extra_args)
        connect_nodes(self.nodes[0], 1)
        self.nodes[0].invalidateblock(self.nodes[0].getbestblockhash())
        # Now try to reorg the chain
        self.nodes[0].generate(2)
        sleep(6)
        assert(self.nodes[1].getbestblockhash() == good_tip)
        self.nodes[0].generate(2)
        sleep(6)
        assert(self.nodes[1].getbestblockhash() == good_tip)

        # Now let the node which is on the wrong chain reorg back to the locked chain
        self.nodes[0].reconsiderblock(good_tip)
        assert(self.nodes[0].getbestblockhash() != good_tip)
        self.nodes[1].generate(1)
        self.wait_for_chainlock(self.nodes[0], self.nodes[1].getbestblockhash())
        assert(self.nodes[0].getbestblockhash() == self.nodes[1].getbestblockhash())

    def wait_for_chainlock_tip_all_nodes(self):
        for node in self.nodes:
            tip = node.getbestblockhash()
            self.wait_for_chainlock(node, tip)

    def wait_for_chainlock_tip(self, node):
        tip = node.getbestblockhash()
        self.wait_for_chainlock(node, tip)

    def wait_for_chainlock(self, node, block_hash):
        t = time()
        while time() - t < 15:
            try:
                block = node.getblock(block_hash)
                if block["confirmations"] > 0 and block["chainlock"]:
                    return
            except:
                # block might not be on the node yet
                pass
            sleep(0.1)
        raise AssertionError("wait_for_chainlock timed out")


if __name__ == '__main__':
    LLMQChainLocksTest().main()
