#!/usr/bin/env python3
# Copyright (c) 2015-2022 The Dash Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

'''
feature_llmq_chainlocks.py

Checks LLMQs based ChainLocks

'''

import time

from test_framework.test_framework import DashTestFramework
from test_framework.util import force_finish_mnsync, assert_equal, assert_raises_rpc_error


class LLMQChainLocksTest(DashTestFramework):
    def set_test_params(self):
        self.set_dash_test_params(4, 3, fast_dip3_enforcement=True)

    def run_test(self):

        # Connect all nodes to node1 so that we always have the whole network connected
        # Otherwise only masternode connections will be established between nodes, which won't propagate TXs/blocks
        # Usually node0 is the one that does this, but in this test we isolate it multiple times
        for i in range(len(self.nodes)):
            if i != 1:
                self.connect_nodes(i, 1)

        self.activate_dip8()

        self.test_coinbase_best_cl(self.nodes[0], expected_cl_in_cb=False)

        self.activate_v20(expected_activation_height=1200)
        self.log.info("Activated v20 at height:" + str(self.nodes[0].getblockcount()))

        # v20 is active for the next block, not for the tip
        self.test_coinbase_best_cl(self.nodes[0], expected_cl_in_cb=False)

        # v20 is active, no quorums, no CLs - null CL in CbTx
        self.nodes[0].generate(1)
        self.test_coinbase_best_cl(self.nodes[0], expected_cl_in_cb=True, expected_null_cl=True)

        self.nodes[0].sporkupdate("SPORK_17_QUORUM_DKG_ENABLED", 0)
        self.wait_for_sporks_same()

        self.log.info("Mining 4 quorums")
        for i in range(4):
            self.mine_quorum()

        self.log.info("Mine single block, wait for chainlock")
        self.nodes[0].generate(1)
        self.wait_for_chainlocked_block_all_nodes(self.nodes[0].getbestblockhash())
        self.test_coinbase_best_cl(self.nodes[0])

        self.log.info("Mine many blocks, wait for chainlock")
        self.nodes[0].generate(20)
        # We need more time here due to 20 blocks being generated at once
        self.wait_for_chainlocked_block_all_nodes(self.nodes[0].getbestblockhash(), timeout=30)
        self.test_coinbase_best_cl(self.nodes[0])

        self.log.info("Assert that all blocks up until the tip are chainlocked")
        for h in range(1, self.nodes[0].getblockcount()):
            block = self.nodes[0].getblock(self.nodes[0].getblockhash(h))
            assert block['chainlock']

        # Update spork to SPORK_19_CHAINLOCKS_ENABLED and test its behaviour
        self.nodes[0].sporkupdate("SPORK_19_CHAINLOCKS_ENABLED", 1)
        self.wait_for_sporks_same()

        # Generate new blocks and verify that they are not chainlocked
        previous_block_hash = self.nodes[0].getbestblockhash()
        for _ in range(2):
            block_hash = self.nodes[0].generate(1)[0]
            self.wait_for_chainlocked_block_all_nodes(block_hash, expected=False)
            assert self.nodes[0].getblock(previous_block_hash)["chainlock"]

        self.nodes[0].sporkupdate("SPORK_19_CHAINLOCKS_ENABLED", 0)
        self.wait_for_sporks_same()

        self.log.info("Isolate node, mine on another, and reconnect")
        self.isolate_node(0)
        node0_mining_addr = self.nodes[0].getnewaddress()
        node0_tip = self.nodes[0].getbestblockhash()
        self.nodes[1].generatetoaddress(5, node0_mining_addr)
        self.wait_for_chainlocked_block(self.nodes[1], self.nodes[1].getbestblockhash())
        self.test_coinbase_best_cl(self.nodes[0])
        assert self.nodes[0].getbestblockhash() == node0_tip
        self.reconnect_isolated_node(0, 1)
        self.nodes[1].generatetoaddress(1, node0_mining_addr)
        self.wait_for_chainlocked_block_all_nodes(self.nodes[1].getbestblockhash())
        self.test_coinbase_best_cl(self.nodes[0])

        self.log.info("Isolate node, mine on both parts of the network, and reconnect")
        self.isolate_node(0)
        bad_tip = self.nodes[0].generate(5)[-1]
        self.nodes[1].generatetoaddress(1, node0_mining_addr)
        good_tip = self.nodes[1].getbestblockhash()
        self.wait_for_chainlocked_block(self.nodes[1], good_tip)
        assert not self.nodes[0].getblock(self.nodes[0].getbestblockhash())["chainlock"]
        self.reconnect_isolated_node(0, 1)
        self.nodes[1].generatetoaddress(1, node0_mining_addr)
        self.wait_for_chainlocked_block_all_nodes(self.nodes[1].getbestblockhash())
        self.test_coinbase_best_cl(self.nodes[0])
        assert self.nodes[0].getblock(self.nodes[0].getbestblockhash())["previousblockhash"] == good_tip
        assert self.nodes[1].getblock(self.nodes[1].getbestblockhash())["previousblockhash"] == good_tip

        self.log.info("The tip mined while this node was isolated should be marked conflicting now")
        found = False
        for tip in self.nodes[0].getchaintips(2):
            if tip["hash"] == bad_tip:
                assert tip["status"] == "conflicting"
                found = True
                break
        assert found

        self.log.info("Keep node connected and let it try to reorg the chain")
        good_tip = self.nodes[0].getbestblockhash()
        self.log.info("Restart it so that it forgets all the chainlock messages from the past")
        self.restart_node(0)
        self.connect_nodes(0, 1)
        assert self.nodes[0].getbestblockhash() == good_tip
        self.nodes[0].invalidateblock(good_tip)
        self.log.info("Now try to reorg the chain")
        self.nodes[0].generate(2)
        time.sleep(6)
        assert self.nodes[1].getbestblockhash() == good_tip
        bad_tip = self.nodes[0].generate(2)[-1]
        time.sleep(6)
        assert self.nodes[0].getbestblockhash() == bad_tip
        assert self.nodes[1].getbestblockhash() == good_tip

        self.log.info("Now let the node which is on the wrong chain reorg back to the locked chain")
        self.nodes[0].reconsiderblock(good_tip)
        assert self.nodes[0].getbestblockhash() != good_tip
        good_fork = good_tip
        good_tip = self.nodes[1].generatetoaddress(1, node0_mining_addr)[-1]  # this should mark bad_tip as conflicting
        self.wait_for_chainlocked_block_all_nodes(good_tip)
        self.test_coinbase_best_cl(self.nodes[0])
        assert self.nodes[0].getbestblockhash() == good_tip
        found = False
        for tip in self.nodes[0].getchaintips(2):
            if tip["hash"] == bad_tip:
                assert tip["status"] == "conflicting"
                found = True
                break
        assert found

        self.log.info("Should switch to the best non-conflicting tip (not to the most work chain) on restart")
        assert int(self.nodes[0].getblock(bad_tip)["chainwork"], 16) > int(self.nodes[1].getblock(good_tip)["chainwork"], 16)
        self.restart_node(0)
        self.nodes[0].invalidateblock(good_fork)
        self.restart_node(0)
        time.sleep(1)
        assert self.nodes[0].getbestblockhash() == good_tip

        self.log.info("Isolate a node and let it create some transactions which won't get IS locked")
        force_finish_mnsync(self.nodes[0])
        self.isolate_node(0)
        txs = []
        for i in range(3):
            txs.append(self.nodes[0].sendtoaddress(self.nodes[0].getnewaddress(), 1))
        txs += self.create_chained_txs(self.nodes[0], 1)
        self.log.info("Assert that after block generation these TXs are NOT included (as they are \"unsafe\")")
        node0_tip = self.nodes[0].generate(1)[-1]
        for txid in txs:
            tx = self.nodes[0].getrawtransaction(txid, 1)
            assert "confirmations" not in tx
        time.sleep(1)
        node0_tip_block = self.nodes[0].getblock(node0_tip)
        assert not node0_tip_block["chainlock"]
        assert node0_tip_block["previousblockhash"] == good_tip
        self.log.info("Disable LLMQ based InstantSend for a very short time (this never gets propagated to other nodes)")
        self.nodes[0].sporkupdate("SPORK_2_INSTANTSEND_ENABLED", 4070908800)
        self.log.info("Now the TXs should be included")
        self.nodes[0].generate(1)
        self.nodes[0].sporkupdate("SPORK_2_INSTANTSEND_ENABLED", 0)
        self.log.info("Assert that TXs got included now")
        for txid in txs:
            tx = self.nodes[0].getrawtransaction(txid, 1)
            assert "confirmations" in tx and tx["confirmations"] > 0
        # Enable network on first node again, which will cause the blocks to propagate and IS locks to happen retroactively
        # for the mined TXs, which will then allow the network to create a CLSIG
        self.log.info("Re-enable network on first node and wait for chainlock")
        self.reconnect_isolated_node(0, 1)
        self.wait_for_chainlocked_block(self.nodes[0], self.nodes[0].getbestblockhash(), timeout=30)

        for i in range(2):
            self.log.info(f"{'Disable' if i == 0 else 'Enable'} Chainlock")
            self.nodes[0].sporkupdate("SPORK_19_CHAINLOCKS_ENABLED", 4070908800 if i == 0 else 0)
            self.wait_for_sporks_same()

            self.log.info("Add a new node and let it sync")
            self.dynamically_add_masternode(evo=False)
            added_idx = len(self.nodes) - 1
            assert_raises_rpc_error(-32603, "Unable to find any chainlock", self.nodes[added_idx].getbestchainlock)

            self.log.info("Test that new node can mine without Chainlock info")
            tip_0 = self.nodes[0].getblock(self.nodes[0].getbestblockhash(), 2)
            self.nodes[added_idx].generate(1)
            self.sync_blocks(self.nodes)
            tip_1 = self.nodes[0].getblock(self.nodes[0].getbestblockhash(), 2)
            assert_equal(tip_1['cbTx']['bestCLSignature'], tip_0['cbTx']['bestCLSignature'])
            assert_equal(tip_1['cbTx']['bestCLHeightDiff'], tip_0['cbTx']['bestCLHeightDiff'] + 1)

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
        rawtx = node.signrawtransactionwithwallet(rawtx)
        rawtxid = node.sendrawtransaction(rawtx["hex"])

        return [txid, rawtxid]

    def test_coinbase_best_cl(self, node, expected_cl_in_cb=True, expected_null_cl=False):
        block_hash = node.getbestblockhash()
        block = node.getblock(block_hash, 2)
        cbtx = block["cbTx"]
        assert_equal(int(cbtx["version"]) > 2, expected_cl_in_cb)
        if expected_cl_in_cb:
            cb_height = int(cbtx["height"])
            best_cl_height_diff = int(cbtx["bestCLHeightDiff"])
            best_cl_signature = cbtx["bestCLSignature"]
            assert_equal(expected_null_cl, int(best_cl_signature, 16) == 0)
            if expected_null_cl:
                # Null bestCLSignature is allowed.
                # bestCLHeightDiff must be 0 if bestCLSignature is null
                assert_equal(best_cl_height_diff, 0)
                # Returning as no more tests can be conducted
                return
            best_cl_height = cb_height - best_cl_height_diff - 1
            target_block_hash = node.getblockhash(best_cl_height)
            # Verify CL signature
            assert node.verifychainlock(target_block_hash, best_cl_signature, best_cl_height)
        else:
            assert "bestCLHeightDiff" not in cbtx and "bestCLSignature" not in cbtx


if __name__ == '__main__':
    LLMQChainLocksTest().main()
