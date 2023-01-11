#!/usr/bin/env python3
# Copyright (c) 2015-2020 The Dash Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
import time
import struct
from test_framework.test_framework import DashTestFramework
from test_framework.messages import CInv, hash256, msg_inv, ser_string, uint256_from_str
from test_framework.p2p import (
  P2PInterface,
)
from test_framework.blocktools import (
  create_block,
  create_coinbase,
)
from test_framework.util import force_finish_mnsync, assert_equal, assert_raises_rpc_error
'''
feature_llmqchainlocks.py

Checks LLMQs based ChainLocks

'''

class TestP2PConn(P2PInterface):
    def __init__(self):
        super().__init__()
        self.clsigs = {}

    def send_clsig(self, clsig):
        clhash = uint256_from_str(hash256(clsig.serialize()))
        self.clsigs[clhash] = clsig

        inv = msg_inv([CInv(20, clhash)])
        self.send_message(inv)

    def on_getdata(self, message):
        for inv in message.inv:
            if inv.hash in self.clsigs:
                self.send_message(self.clsigs[inv.hash])

class LLMQChainLocksTest(DashTestFramework):
    def add_options(self, parser):
        self.add_wallet_options(parser)

    def set_test_params(self):
        self.set_dash_test_params(4, 3, fast_dip3_enforcement=True)

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()
        self.skip_if_no_bdb()

    def run_test(self):
        for i in range(len(self.nodes)):
            force_finish_mnsync(self.nodes[i])
        # Connect all nodes to node1 so that we always have the whole network connected
        # Otherwise only masternode connections will be established between nodes, which won't propagate TXs/blocks
        # Usually node0 is the one that does this, but in this test we isolate it multiple times
        for i in range(len(self.nodes)):
            if i != 1:
                self.connect_nodes(i, 1)
        self.generate(self.nodes[0], 10)
        self.sync_blocks(self.nodes, timeout=60*5)
        self.nodes[0].spork("SPORK_17_QUORUM_DKG_ENABLED", 0)
        self.nodes[0].spork("SPORK_19_CHAINLOCKS_ENABLED", 4070908800)
        self.wait_for_sporks_same()

        self.log.info("Mining 4 quorums")
        for i in range(4):
            self.mine_quorum(mod5=True)
        self.nodes[0].spork("SPORK_19_CHAINLOCKS_ENABLED", 0)
        self.wait_for_sporks_same()
        self.log.info("Mine single block, wait for chainlock")
        cl = self.nodes[0].getbestblockhash()
        self.generate(self.nodes[0], 5)

        self.wait_for_chainlocked_block_all_nodes(cl)

        self.log.info("Mine many blocks, wait for chainlock")
        cl = self.generate(self.nodes[0], 20)[-6]
        # We need more time here due to 20 blocks being generated at once
        self.wait_for_chainlocked_block_all_nodes(cl, timeout=30)

        self.log.info("Assert that all blocks up until the lookback are chainlocked")
        for h in range(1, self.nodes[0].getblockcount()-5):
            block = self.nodes[0].getblock(self.nodes[0].getblockhash(h))
            assert block['chainlock']

        self.log.info("Isolate node, mine on another, and reconnect")
        node0_mining_addr = self.nodes[0].getnewaddress()
        self.isolate_node(self.nodes[0])
        node0_tip = self.nodes[0].getbestblockhash()
        cl = self.generatetoaddress(self.nodes[1], 10, node0_mining_addr, sync_fun=self.no_op)[-6]
        self.wait_for_chainlocked_block(self.nodes[1], cl)
        assert self.nodes[0].getbestblockhash() == node0_tip
        self.reconnect_isolated_node(self.nodes[0], 1)
        cl = self.nodes[1].getbestblockhash()
        self.generatetoaddress(self.nodes[1], 5, node0_mining_addr, sync_fun=self.no_op)
        self.wait_for_chainlocked_block(self.nodes[0], cl)


        self.log.info("Isolate node, mine on both parts of the network, and reconnect")
        self.isolate_node(self.nodes[0])
        bad_cl =  self.generate(self.nodes[0], 15, sync_fun=self.no_op)[-6]
        bad_tip = self.nodes[0].getbestblockhash()
        good_cl = self.nodes[1].getbestblockhash()
        self.generatetoaddress(self.nodes[1], 10, node0_mining_addr, sync_fun=self.no_op)
        self.wait_for_chainlocked_block(self.nodes[1], good_cl)
        assert not self.nodes[0].getblock(bad_cl)["chainlock"]
        self.reconnect_isolated_node(self.nodes[0], 1)
        self.bump_mocktime(5, nodes=self.nodes)
        good_cl = self.nodes[1].getbestblockhash()
        # create a new CLSIG
        good_tip = self.generatetoaddress(self.nodes[1], 5, node0_mining_addr, sync_fun=self.no_op)[-1]
        # since shorter chain was chainlocked and its a valid chain, node with longer chain should switch to it
        self.wait_for_chainlocked_block(self.nodes[0], good_cl)
        self.wait_until(lambda: self.nodes[0].getbestblockhash() == good_tip)
        assert self.nodes[1].getbestblockhash() == good_tip
        self.log.info("The tip mined while this node was isolated should be marked conflicting now")
        found = False
        for tip in self.nodes[0].getchaintips():
            if tip["hash"] == bad_tip:
                assert tip["status"] == "conflicting"
                found = True
                break
        assert found
        self.log.info("Isolate node, mine invalid longer chain(locked) + shorter good chain, and reconnect")
        prevActiveChainlock = self.nodes[0].getchainlocks()["active_chainlock"]["blockhash"]
        # set flag create and accept invalid blocks
        for i in range(len(self.nodes)):
            self.nodes[i].settestparams("1")
        cl = self.generate(self.nodes[0], 10)[-6]
        bad_tip = self.nodes[0].getbestblockhash()
        self.wait_for_chainlocked_block_all_nodes(cl, timeout=30)
        # clear the invalid flag
        for i in range(len(self.nodes)):
            self.nodes[i].settestparams("0")
        # should set chainlock to previous one due to invalid block getting locked
        for i in range(len(self.nodes)):
            self.nodes[i].invalidateblock(good_tip)
            self.nodes[i].reconsiderblock(good_tip)
            assert self.nodes[i].getchainlocks()["active_chainlock"]["blockhash"] == prevActiveChainlock
            assert not self.nodes[i].getblock(cl)["chainlock"]
            assert self.nodes[i].getbestblockhash() == good_tip

        cl = self.nodes[0].getbestblockhash()
        self.generate(self.nodes[0], 5)
        self.wait_for_chainlocked_block_all_nodes(cl, timeout=30)

        self.log.info("bad tip mined while being locked should be invalid now")
        found = False
        for tip in self.nodes[0].getchaintips():
            if tip["hash"] == bad_tip:
                assert tip["status"] == "invalid"
                found = True
                break
        assert found

        self.log.info("Keep node connected and let it try to reorg the chain")
        good_cl = self.nodes[0].getbestblockhash()
        good_tip = self.generate(self.nodes[0], 5)[-1]
        self.wait_for_chainlocked_block_all_nodes(good_cl, timeout=30)
        self.log.info("Restart it so that it forgets all the chainlock messages from the past")
        self.stop_node(0)
        self.start_node(0)
        self.connect_nodes(0, 1)
        force_finish_mnsync(self.nodes[0])
        assert self.nodes[0].getbestblockhash() == good_tip
        self.nodes[0].invalidateblock(good_cl)
        self.log.info("Now try to reorg the chain")
        self.generatetoaddress(self.nodes[0], 5, node0_mining_addr, sync_fun=self.no_op)
        self.wait_until(lambda: self.nodes[1].getbestblockhash() == good_tip)
        bad_cl = self.nodes[0].getbestblockhash()
        bad_tip = self.generatetoaddress(self.nodes[0], 5, node0_mining_addr, sync_fun=self.no_op)[-1]
        self.wait_until(lambda: self.nodes[0].getbestblockhash() == bad_tip)
        self.wait_until(lambda: self.nodes[1].getbestblockhash() == good_tip)

        self.log.info("Now let the node which is on the wrong chain reorg back to the locked chain")
        self.nodes[0].reconsiderblock(good_tip)
        assert self.nodes[0].getbestblockhash() != good_tip
        good_cl = self.nodes[1].getbestblockhash()
        good_tip = self.generatetoaddress(self.nodes[1], 5, node0_mining_addr, sync_fun=self.no_op)[-1]
        self.wait_for_chainlocked_block(self.nodes[0], good_cl)
        assert self.nodes[0].getbestblockhash() == good_tip
        found = False
        foundConflict = False
        for tip in self.nodes[0].getchaintips():
            if tip["hash"] == good_tip:
                assert tip["status"] == "active"
                found = True
            if tip["status"] == "conflicting":
                foundConflict = True
            if tip["status"] == "valid-fork":
                forkChain = tip["hash"]

        assert found and foundConflict
        self.log.info("Should switch to the locked tip on invalidate and stay there across restarts until reconsider is called again")
        self.stop_node(0)
        self.start_node(0)
        # will set valid-fork to active
        self.nodes[0].invalidateblock(good_cl)
        assert self.nodes[0].getbestblockhash() == forkChain
        self.stop_node(0)
        self.start_node(0)
        found = False
        assert self.nodes[0].getbestblockhash() == forkChain
        # reconsider to switch to good_tip but chainlocks should be cleared (invalidate block sets invalid flag on block but not chainlock failure so clear happens)
        self.nodes[0].reconsiderblock(good_cl)
        # back on the good_tip but no locks present anymore
        assert self.nodes[0].getbestblockhash() == good_tip
        # make sure chainlocks are cleared due to chainlocked invalid block
        assert_raises_rpc_error(-32603, 'Unable to find any chainlock', self.nodes[0].getchainlocks)

        self.isolate_node(self.nodes[0])
        txs = []
        for i in range(3):
            txs.append(self.nodes[0].sendtoaddress(self.nodes[0].getnewaddress(), 1))
        txs += self.create_chained_txs(self.nodes[0], 1)
        self.log.info("Assert that after block generation these TXs are included")
        node0_cl = self.nodes[0].getbestblockhash()
        self.generate(self.nodes[0], 5, sync_fun=self.no_op)
        for txid in txs:
            tx = self.nodes[0].getrawtransaction(txid, True)
            assert "confirmations" in tx
        node0_cl_block = self.nodes[0].getblock(node0_cl)
        assert not node0_cl_block["chainlock"]
        node0_cl = self.nodes[0].getbestblockhash()
        self.generate(self.nodes[0], 5, sync_fun=self.no_op)
        self.log.info("Assert that TXs got included now")
        for txid in txs:
            tx = self.nodes[0].getrawtransaction(txid, True)
            assert "confirmations" in tx and tx["confirmations"] > 0
        # Enable network on first node again, which will cause the blocks to propagate
        # for the mined TXs, which will then allow the network to create a CLSIG
        self.log.info("Re-enable network on first node and wait for chainlock")
        self.reconnect_isolated_node(self.nodes[0], 1)
        self.wait_for_chainlocked_block(self.nodes[0], node0_cl)

        self.log.info("Send fake future clsigs and see if this breaks ChainLocks")
        for i in range(len(self.nodes)):
            if i != 0:
                self.connect_nodes(i, 0)
        p2p_node = self.nodes[0].add_p2p_connection(TestP2PConn())
        p2p_node.wait_for_verack()
        self.wait_for_chainlocked_block_all_nodes(node0_cl, timeout=30)

        self.log.info("Shouldn't accept fake clsig when block isn't valid")
        self.create_fake_clsig(1)
        self.bump_mocktime(5, nodes=self.nodes)
        # no locks formed
        for node in self.nodes:
            self.wait_until(lambda: self.nodes[0].getchainlocks()["recent_chainlock"]["blockhash"] == node0_cl)
            self.wait_until(lambda: self.nodes[0].getchainlocks()["active_chainlock"]["blockhash"] == node0_cl)
        # lock as normal on new block
        tip = self.nodes[0].getbestblockhash()
        self.generate(self.nodes[0], 5, sync_fun=self.no_op)
        self.bump_mocktime(5, nodes=self.nodes)
        self.wait_for_chainlocked_block_all_nodes(tip, timeout=30)
        self.log.info("Allow signing on competing chains")
        self.generate(self.nodes[2], 5, sync_fun=self.no_op)
        # create a fork
        self.generate(self.nodes[1], 5, sync_fun=self.no_op)
        time.sleep(5)
        # this just ensures all nodes are on the same tip and continue on getting locked
        # Make sure we are mod of 5 block count before checking for CL
        skip_count = 5 - (self.nodes[0].getblockcount() % 5)
        if skip_count != 0:
            self.generate(self.nodes[0], skip_count)
        new_cl = self.nodes[0].getblockhash(self.nodes[0].getblockcount() - 5)
        self.wait_for_chainlocked_block_all_nodes(new_cl, timeout=30)
        self.nodes[0].disconnect_p2ps()

    def create_fake_clsig(self, height_offset):
        # create a fake block height_offset blocks ahead of the tip
        height = self.nodes[0].getblockcount() + height_offset
        fake_block = create_block(0xff, create_coinbase(height))
        # create a fake clsig for that block
        quorum_hash = self.nodes[0].quorum_list(1)["llmq_test"][0]
        request_id_buf = ser_string(b"clsig") + struct.pack("<I", height)
        request_id_buf += bytes.fromhex(quorum_hash)[::-1]
        request_id = hash256(request_id_buf)[::-1].hex()
        quorum_hash = self.nodes[0].quorum_list(1)["llmq_test"][0]
        for mn in self.mninfo:
            mn.node.quorum_sign(100, request_id, fake_block.hash, quorum_hash)
        rec_sig = self.get_recovered_sig(request_id, fake_block.hash)
        assert_equal(rec_sig, False)

    def create_chained_txs(self, node, amount):
        txid = node.sendtoaddress(node.getnewaddress(), amount)
        tx = node.getrawtransaction(txid, True)
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


if __name__ == '__main__':
    LLMQChainLocksTest().main()
