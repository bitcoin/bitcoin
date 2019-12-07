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
        super().__init__(6, 5, fast_dip3_enforcement=True)

    def run_test(self):

        while self.nodes[0].getblockchaininfo()["bip9_softforks"]["dip0008"]["status"] != "active":
            self.nodes[0].generate(10)
        sync_blocks(self.nodes, timeout=60*5)

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
        isolate_node(self.nodes[0])
        node0_tip = self.nodes[0].getbestblockhash()
        self.nodes[1].generate(5)
        self.wait_for_chainlock_tip(self.nodes[1])
        assert(self.nodes[0].getbestblockhash() == node0_tip)
        reconnect_isolated_node(self.nodes[0], 1)
        self.nodes[1].generate(1)
        self.wait_for_chainlock(self.nodes[0], self.nodes[1].getbestblockhash())

        # Isolate node, mine on both parts of the network, and reconnect
        isolate_node(self.nodes[0])
        self.nodes[0].generate(5)
        self.nodes[1].generate(1)
        good_tip = self.nodes[1].getbestblockhash()
        self.wait_for_chainlock_tip(self.nodes[1])
        assert(not self.nodes[0].getblock(self.nodes[0].getbestblockhash())["chainlock"])
        reconnect_isolated_node(self.nodes[0], 1)
        self.nodes[1].generate(1)
        self.wait_for_chainlock(self.nodes[0], self.nodes[1].getbestblockhash())
        assert(self.nodes[0].getblock(self.nodes[0].getbestblockhash())["previousblockhash"] == good_tip)
        assert(self.nodes[1].getblock(self.nodes[1].getbestblockhash())["previousblockhash"] == good_tip)

        # Keep node connected and let it try to reorg the chain
        good_tip = self.nodes[0].getbestblockhash()
        # Restart it so that it forgets all the chainlocks from the past
        stop_node(self.nodes[0], 0)
        self.nodes[0] = start_node(0, self.options.tmpdir, self.extra_args[0])
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

        # Enable LLMQ bases InstantSend, which also enables checks for "safe" transactions
        self.nodes[0].spork("SPORK_20_INSTANTSEND_LLMQ_BASED", 0)
        self.wait_for_sporks_same()

        # Isolate a node and let it create some transactions which won't get IS locked
        isolate_node(self.nodes[0])
        txs = []
        for i in range(3):
            txs.append(self.nodes[0].sendtoaddress(self.nodes[0].getnewaddress(), 1))
        txs += self.create_chained_txs(self.nodes[0], 1)
        # Assert that after block generation these TXs are NOT included (as they are "unsafe")
        self.nodes[0].generate(1)
        for txid in txs:
            tx = self.nodes[0].getrawtransaction(txid, 1)
            assert("confirmations" not in tx)
        sleep(1)
        assert(not self.nodes[0].getblock(self.nodes[0].getbestblockhash())["chainlock"])
        # Disable LLMQ based InstantSend for a very short time (this never gets propagated to other nodes)
        self.nodes[0].spork("SPORK_20_INSTANTSEND_LLMQ_BASED", 4070908800)
        # Now the TXs should be included
        self.nodes[0].generate(1)
        self.nodes[0].spork("SPORK_20_INSTANTSEND_LLMQ_BASED", 0)
        # Assert that TXs got included now
        for txid in txs:
            tx = self.nodes[0].getrawtransaction(txid, 1)
            assert("confirmations" in tx and tx["confirmations"] > 0)
        # Enable network on first node again, which will cause the blocks to propagate and IS locks to happen retroactively
        # for the mined TXs, which will then allow the network to create a CLSIG
        reconnect_isolated_node(self.nodes[0], 1)
        self.wait_for_chainlock(self.nodes[0], self.nodes[1].getbestblockhash())

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

    def create_chained_txs(self, node, amount):
        txid = node.sendtoaddress(node.getnewaddress(), amount)
        tx = node.getrawtransaction(txid, 1)
        inputs = []
        valueIn = 0
        for txout in tx["vout"]:
            inputs.append({"txid": txid, "vout": txout["n"]})
            valueIn += txout["value"]
        outputs = {
            node.getnewaddress(): round(float(valueIn) - 0.0001, 6)
        }

        rawtx = node.createrawtransaction(inputs, outputs)
        rawtx = node.signrawtransaction(rawtx)
        rawtxid = node.sendrawtransaction(rawtx["hex"])

        return [txid, rawtxid]


if __name__ == '__main__':
    LLMQChainLocksTest().main()
