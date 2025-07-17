#!/usr/bin/env python3
# Copyright (c) 2015-2024 The Dash Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

'''
feature_llmq_is_cl_conflicts.py

Checks conflict handling between ChainLocks and InstantSend

'''

import struct

from test_framework.blocktools import create_block_with_mnpayments
from test_framework.messages import CInv, hash256, msg_clsig, msg_inv, ser_string, tx_from_hex, uint256_from_str
from test_framework.p2p import P2PInterface
from test_framework.test_framework import DashTestFramework
from test_framework.util import assert_equal, assert_raises_rpc_error


class TestP2PConn(P2PInterface):
    def __init__(self):
        super().__init__()
        self.clsigs = {}
        self.islocks = {}

    def send_clsig(self, clsig):
        hash = uint256_from_str(hash256(clsig.serialize()))
        self.clsigs[hash] = clsig

        inv = msg_inv([CInv(29, hash)])
        self.send_message(inv)

    def send_isdlock(self, isdlock):
        hash = uint256_from_str(hash256(isdlock.serialize()))
        self.islocks[hash] = isdlock

        inv = msg_inv([CInv(31, hash)])
        self.send_message(inv)

    def on_getdata(self, message):
        for inv in message.inv:
            if inv.hash in self.clsigs:
                self.send_message(self.clsigs[inv.hash])
            if inv.hash in self.islocks:
                self.send_message(self.islocks[inv.hash])


class LLMQ_IS_CL_Conflicts(DashTestFramework):
    def set_test_params(self):
        self.set_dash_test_params(5, 4)
        self.set_dash_llmq_test_params(4, 4)
        self.delay_v20_and_mn_rr(2000)
        self.supports_cli = False

    def run_test(self):
        self.test_node = self.nodes[0].add_p2p_connection(TestP2PConn())

        self.nodes[0].sporkupdate("SPORK_17_QUORUM_DKG_ENABLED", 0)
        self.wait_for_sporks_same()

        self.mine_cycle_quorum()

        # mine single block, wait for chainlock
        self.generate(self.nodes[0], 1, sync_fun=self.no_op)
        self.wait_for_chainlocked_block_all_nodes(self.nodes[0].getbestblockhash())

        self.test_chainlock_overrides_islock(False)
        self.test_chainlock_overrides_islock(True, False)
        self.test_chainlock_overrides_islock(True, True)
        self.test_chainlock_overrides_islock_overrides_nonchainlock()

    def test_chainlock_overrides_islock(self, test_block_conflict, mine_confllicting=False):
        if not test_block_conflict:
            assert not mine_confllicting

        # create three raw TXs, they will conflict with each other
        rawtx1 = self.create_raw_tx(self.nodes[0], self.nodes[0], 1, 1, 100)['hex']
        rawtx2 = self.create_raw_tx(self.nodes[0], self.nodes[0], 1, 1, 100)['hex']
        rawtx1_obj = tx_from_hex(rawtx1)
        rawtx2_obj = tx_from_hex(rawtx2)

        rawtx1_txid = self.nodes[0].sendrawtransaction(rawtx1)
        rawtx2_txid = hash256(bytes.fromhex(rawtx2))[::-1].hex()

        # Create a chained TX on top of tx1
        inputs = []
        n = 0
        for out in rawtx1_obj.vout:
            if out.nValue == 100000000:
                inputs.append({"txid": rawtx1_txid, "vout": n})
            n += 1
        rawtx4 = self.nodes[0].createrawtransaction(inputs, {self.nodes[0].getnewaddress(): 0.999})
        rawtx4 = self.nodes[0].signrawtransactionwithwallet(rawtx4)['hex']
        rawtx4_txid = self.nodes[0].sendrawtransaction(rawtx4)

        # wait for transactions to propagate
        self.bump_mocktime(30)
        self.sync_mempools()
        for node in self.nodes:
            self.wait_for_instantlock(rawtx1_txid, node)
            self.wait_for_instantlock(rawtx4_txid, node)

        block = create_block_with_mnpayments(self.mninfo, self.nodes[0], [rawtx2_obj])
        if test_block_conflict:
            # The block shouldn't be accepted/connected but it should be known to node 0 now
            submit_result = self.nodes[0].submitblock(block.serialize().hex())
            assert submit_result == "conflict-tx-lock"

        cl = self.create_chainlock(self.nodes[0].getblockcount() + 1, block)

        if mine_confllicting:
            # Make sure we won't sent clsig too early
            islock_tip = self.generate(self.nodes[0], 1, sync_fun=lambda: self.sync_blocks())[-1]

        self.test_node.send_clsig(cl)

        for node in self.nodes:
            self.wait_for_best_chainlock(node, block.hash)

        self.sync_blocks()

        if mine_confllicting:
            # The tip with IS-locked txes should be marked conflicting now
            found1 = False
            found2 = False
            for tip in self.nodes[0].getchaintips(2):
                if tip["hash"] == islock_tip:
                    assert tip["status"] == "conflicting"
                    found1 = True
                elif tip["hash"] == block.hash:
                    assert tip["status"] == "active"
                    found2 = True
            assert found1 and found2

        # At this point all nodes should be in sync and have the same "best chainlock"

        submit_result = self.nodes[1].submitblock(block.serialize().hex())
        if test_block_conflict:
            # Node 1 should receive the block from node 0 and should not accept it again via submitblock
            assert submit_result == "duplicate"
        else:
            # The block should get accepted now, and at the same time prune the conflicting ISLOCKs
            assert submit_result is None

        self.wait_for_chainlocked_block_all_nodes(block.hash)

        # Create a chained TX on top of tx2
        inputs = []
        n = 0
        for out in rawtx2_obj.vout:
            if out.nValue == 100000000:
                inputs.append({"txid": rawtx2_txid, "vout": n})
            n += 1
        rawtx5 = self.nodes[0].createrawtransaction(inputs, {self.nodes[0].getnewaddress(): 0.999})
        rawtx5 = self.nodes[0].signrawtransactionwithwallet(rawtx5)['hex']
        rawtx5_txid = self.nodes[0].sendrawtransaction(rawtx5)
        # wait for the transaction to propagate
        self.bump_mocktime(30)
        self.sync_mempools()
        for node in self.nodes:
            self.wait_for_instantlock(rawtx5_txid, node)

        if mine_confllicting:
            # Lets verify that the ISLOCKs got pruned and conflicting txes were mined but never confirmed
            for node in self.nodes:
                rawtx = node.getrawtransaction(rawtx1_txid, True)
                assert not rawtx['chainlock']
                assert not rawtx['instantlock']
                assert not rawtx['instantlock_internal']
                assert_equal(rawtx['confirmations'], 0)
                assert_equal(rawtx['height'], -1)
                rawtx = node.getrawtransaction(rawtx4_txid, True)
                assert not rawtx['chainlock']
                assert not rawtx['instantlock']
                assert not rawtx['instantlock_internal']
                assert_equal(rawtx['confirmations'], 0)
                assert_equal(rawtx['height'], -1)
                rawtx = node.getrawtransaction(rawtx2_txid, True)
                assert rawtx['chainlock']
                assert rawtx['instantlock']
                assert not rawtx['instantlock_internal']
        else:
            # Lets verify that the ISLOCKs got pruned
            for node in self.nodes:
                assert_raises_rpc_error(-5, "No such mempool or blockchain transaction", node.getrawtransaction, rawtx1_txid, True)
                assert_raises_rpc_error(-5, "No such mempool or blockchain transaction", node.getrawtransaction, rawtx4_txid, True)
                rawtx = node.getrawtransaction(rawtx2_txid, True)
                assert rawtx['chainlock']
                assert rawtx['instantlock']
                assert not rawtx['instantlock_internal']

    def test_chainlock_overrides_islock_overrides_nonchainlock(self):
        # create two raw TXs, they will conflict with each other
        rawtx1 = self.create_raw_tx(self.nodes[0], self.nodes[0], 1, 1, 100)['hex']
        rawtx2 = self.create_raw_tx(self.nodes[0], self.nodes[0], 1, 1, 100)['hex']

        rawtx1_txid = hash256(bytes.fromhex(rawtx1))[::-1].hex()
        rawtx2_txid = hash256(bytes.fromhex(rawtx2))[::-1].hex()

        # Create an ISLOCK but don't broadcast it yet
        isdlock = self.create_isdlock(rawtx2)

        # Ensure spork uniqueness in multiple function runs
        self.bump_mocktime(1)
        # Disable ChainLocks to avoid accidental locking
        self.nodes[0].sporkupdate("SPORK_19_CHAINLOCKS_ENABLED", 4070908800)
        self.wait_for_sporks_same()

        # Send tx1, which will later conflict with the ISLOCK
        self.nodes[0].sendrawtransaction(rawtx1)

        # fast forward 11 minutes, so that the TX is considered safe and included in the next block
        self.bump_mocktime(int(60 * 11))

        # Mine the conflicting TX into a block
        good_tip = self.nodes[0].getbestblockhash()
        self.generate(self.nodes[0], 2)

        # Assert that the conflicting tx got mined and the locked TX is not valid
        assert self.nodes[0].getrawtransaction(rawtx1_txid, True)['confirmations'] > 0
        assert_raises_rpc_error(-25, "bad-txns-inputs-missingorspent", self.nodes[0].sendrawtransaction, rawtx2)

        # Create the block and the corresponding clsig but do not relay clsig yet
        cl_block = create_block_with_mnpayments(self.mninfo, self.nodes[0])
        cl = self.create_chainlock(self.nodes[0].getblockcount() + 1, cl_block)
        self.nodes[0].submitblock(cl_block.serialize().hex())
        self.sync_all()
        assert self.nodes[0].getbestblockhash() == cl_block.hash

        # Send the ISLOCK, which should result in the last 2 blocks to be disconnected,
        # even though the nodes don't know the locked transaction yet
        self.test_node.send_isdlock(isdlock)
        for node in self.nodes:
            self.wait_until(lambda: node.getbestblockhash() == good_tip, timeout=10)
            # islock for tx2 is incomplete, tx1 should return in mempool now that blocks are disconnected
            assert rawtx1_txid in set(node.getrawmempool())

        # Should drop tx1 and accept tx2 because there is an isdlock waiting for it
        self.nodes[0].sendrawtransaction(rawtx2)
        # bump mocktime to force tx relay
        self.bump_mocktime(60)
        for node in self.nodes:
            self.wait_for_instantlock(rawtx2_txid, node)

        # Should not allow competing txes now
        assert_raises_rpc_error(-26, "tx-txlock-conflict", self.nodes[0].sendrawtransaction, rawtx1)

        islock_tip = self.generate(self.nodes[0], 1)[0]

        for node in self.nodes:
            self.wait_for_instantlock(rawtx2_txid, node)
            assert_equal(node.getrawtransaction(rawtx2_txid, True)['confirmations'], 1)
            assert_equal(node.getbestblockhash(), islock_tip)

        # Check that the CL-ed block overrides the one with islocks
        self.nodes[0].sporkupdate("SPORK_19_CHAINLOCKS_ENABLED", 0)  # Re-enable ChainLocks to accept clsig
        self.test_node.send_clsig(cl)  # relay clsig ASAP to prevent nodes from locking islock-ed tip
        self.wait_for_sporks_same()
        for node in self.nodes:
            self.wait_for_chainlocked_block(node, cl_block.hash)
            # Previous tip should be marked as conflicting now
            assert_equal(node.getchaintips(2)[1]["status"], "conflicting")

    def create_chainlock(self, height, block):
        request_id_buf = ser_string(b"clsig") + struct.pack("<I", height)
        request_id = hash256(request_id_buf)[::-1].hex()
        message_hash = block.hash

        recSig = self.get_recovered_sig(request_id, message_hash)
        clsig = msg_clsig(height, block.sha256, bytes.fromhex(recSig['sig']))
        return clsig

if __name__ == '__main__':
    LLMQ_IS_CL_Conflicts().main()
