#!/usr/bin/env python3
# Copyright (c) 2015-2021 The Dash Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
import time
from decimal import Decimal

from test_framework.blocktools import get_masternode_payment, create_coinbase, create_block
from test_framework.mininode import *
from test_framework.test_framework import DashTestFramework
from test_framework.util import assert_equal, assert_raises_rpc_error, get_bip9_status

'''
feature_llmq_is_cl_conflicts.py

Checks conflict handling between ChainLocks and InstantSend

'''

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

    def send_islock(self, islock):
        hash = uint256_from_str(hash256(islock.serialize()))
        self.islocks[hash] = islock

        inv = msg_inv([CInv(30, hash)])
        self.send_message(inv)

    def on_getdata(self, message):
        for inv in message.inv:
            if inv.hash in self.clsigs:
                self.send_message(self.clsigs[inv.hash])
            if inv.hash in self.islocks:
                self.send_message(self.islocks[inv.hash])


class LLMQ_IS_CL_Conflicts(DashTestFramework):
    def set_test_params(self):
        self.set_dash_test_params(4, 3, fast_dip3_enforcement=True)
        #disable_mocktime()

    def run_test(self):
        self.activate_dip8()

        self.test_node = self.nodes[0].add_p2p_connection(TestP2PConn())
        network_thread_start()
        self.nodes[0].p2p.wait_for_verack()

        self.nodes[0].spork("SPORK_17_QUORUM_DKG_ENABLED", 0)
        self.wait_for_sporks_same()

        self.mine_quorum()

        # mine single block, wait for chainlock
        self.nodes[0].generate(1)
        self.wait_for_chainlocked_block_all_nodes(self.nodes[0].getbestblockhash())

        self.test_chainlock_overrides_islock(False)
        self.test_chainlock_overrides_islock(True, False)
        self.test_chainlock_overrides_islock(True, True)
        self.test_islock_overrides_nonchainlock()

    def test_chainlock_overrides_islock(self, test_block_conflict, mine_confllicting=False):
        if not test_block_conflict:
            assert not mine_confllicting

        # create three raw TXs, they will conflict with each other
        rawtx1 = self.create_raw_tx(self.nodes[0], self.nodes[0], 1, 1, 100)['hex']
        rawtx2 = self.create_raw_tx(self.nodes[0], self.nodes[0], 1, 1, 100)['hex']
        rawtx1_obj = FromHex(CTransaction(), rawtx1)
        rawtx2_obj = FromHex(CTransaction(), rawtx2)

        rawtx1_txid = self.nodes[0].sendrawtransaction(rawtx1)
        rawtx2_txid = encode(hash256(hex_str_to_bytes(rawtx2))[::-1], 'hex_codec').decode('ascii')

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
        self.sync_mempools()
        for node in self.nodes:
            self.wait_for_instantlock(rawtx1_txid, node)
            self.wait_for_instantlock(rawtx4_txid, node)

        block = self.create_block(self.nodes[0], [rawtx2_obj])
        if test_block_conflict:
            # The block shouldn't be accepted/connected but it should be known to node 0 now
            submit_result = self.nodes[0].submitblock(ToHex(block))
            assert(submit_result == "conflict-tx-lock")

        cl = self.create_chainlock(self.nodes[0].getblockcount() + 1, block)

        if mine_confllicting:
            islock_tip = self.nodes[0].generate(1)[-1]

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

        submit_result = self.nodes[1].submitblock(ToHex(block))
        if test_block_conflict:
            # Node 1 should receive the block from node 0 and should not accept it again via submitblock
            assert(submit_result == "duplicate")
        else:
            # The block should get accepted now, and at the same time prune the conflicting ISLOCKs
            assert(submit_result is None)

        for node in self.nodes:
            self.wait_for_chainlocked_block(node, block.hash)

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

    def test_islock_overrides_nonchainlock(self):
        # create two raw TXs, they will conflict with each other
        rawtx1 = self.create_raw_tx(self.nodes[0], self.nodes[0], 1, 1, 100)['hex']
        rawtx2 = self.create_raw_tx(self.nodes[0], self.nodes[0], 1, 1, 100)['hex']

        rawtx1_txid = encode(hash256(hex_str_to_bytes(rawtx1))[::-1], 'hex_codec').decode('ascii')
        rawtx2_txid = encode(hash256(hex_str_to_bytes(rawtx2))[::-1], 'hex_codec').decode('ascii')

        # Create an ISLOCK but don't broadcast it yet
        islock = self.create_islock(rawtx2)

        # Stop enough MNs so that ChainLocks don't work anymore
        for i in range(2):
            self.stop_node(len(self.nodes) - 1)
            self.nodes.pop(len(self.nodes) - 1)
            self.mninfo.pop(len(self.mninfo) - 1)

        # Send tx1, which will later conflict with the ISLOCK
        self.nodes[0].sendrawtransaction(rawtx1)

        # fast forward 11 minutes, so that the TX is considered safe and included in the next block
        self.bump_mocktime(int(60 * 11))

        # Mine the conflicting TX into a block
        good_tip = self.nodes[0].getbestblockhash()
        self.nodes[0].generate(2)
        self.sync_all()

        # Assert that the conflicting tx got mined and the locked TX is not valid
        assert(self.nodes[0].getrawtransaction(rawtx1_txid, True)['confirmations'] > 0)
        assert_raises_rpc_error(-25, "Missing inputs", self.nodes[0].sendrawtransaction, rawtx2)

        # Send the ISLOCK, which should result in the last 2 blocks to be invalidated, even though the nodes don't know
        # the locked transaction yet
        self.test_node.send_islock(islock)
        time.sleep(5)

        assert(self.nodes[0].getbestblockhash() == good_tip)
        assert(self.nodes[1].getbestblockhash() == good_tip)

        # Send the actual transaction and mine it
        self.nodes[0].sendrawtransaction(rawtx2)
        self.nodes[0].generate(1)
        self.sync_all()

        assert(self.nodes[0].getrawtransaction(rawtx2_txid, True)['confirmations'] > 0)
        assert(self.nodes[1].getrawtransaction(rawtx2_txid, True)['confirmations'] > 0)
        assert(self.nodes[0].getrawtransaction(rawtx2_txid, True)['instantlock'])
        assert(self.nodes[1].getrawtransaction(rawtx2_txid, True)['instantlock'])
        assert(self.nodes[0].getbestblockhash() != good_tip)
        assert(self.nodes[1].getbestblockhash() != good_tip)

    def create_block(self, node, vtx=[]):
        bt = node.getblocktemplate()
        height = bt['height']
        tip_hash = bt['previousblockhash']

        coinbasevalue = bt['coinbasevalue']
        miner_address = node.getnewaddress()
        mn_payee = bt['masternode'][0]['payee']

        # calculate fees that the block template included (we'll have to remove it from the coinbase as we won't
        # include the template's transactions
        bt_fees = 0
        for tx in bt['transactions']:
            bt_fees += tx['fee']

        new_fees = 0
        for tx in vtx:
            in_value = 0
            out_value = 0
            for txin in tx.vin:
                txout = node.gettxout(uint256_to_string(txin.prevout.hash), txin.prevout.n, False)
                in_value += int(txout['value'] * COIN)
            for txout in tx.vout:
                out_value += txout.nValue
            new_fees += in_value - out_value

        # fix fees
        coinbasevalue -= bt_fees
        coinbasevalue += new_fees

        realloc_info = get_bip9_status(self.nodes[0], 'realloc')
        realloc_height = 99999999
        if realloc_info['status'] == 'active':
            realloc_height = realloc_info['since']
        mn_amount = get_masternode_payment(height, coinbasevalue, realloc_height)
        miner_amount = coinbasevalue - mn_amount

        outputs = {miner_address: str(Decimal(miner_amount) / COIN)}
        if mn_amount > 0:
            outputs[mn_payee] = str(Decimal(mn_amount) / COIN)

        coinbase = FromHex(CTransaction(), node.createrawtransaction([], outputs))
        coinbase.vin = create_coinbase(height).vin

        # We can't really use this one as it would result in invalid merkle roots for masternode lists
        if len(bt['coinbase_payload']) != 0:
            cbtx = FromHex(CCbTx(version=1), bt['coinbase_payload'])
            coinbase.nVersion = 3
            coinbase.nType = 5 # CbTx
            coinbase.vExtraPayload = cbtx.serialize()

        coinbase.calc_sha256()

        block = create_block(int(tip_hash, 16), coinbase, nTime=bt['curtime'])
        block.vtx += vtx

        # Add quorum commitments from template
        for tx in bt['transactions']:
            tx2 = FromHex(CTransaction(), tx['data'])
            if tx2.nType == 6:
                block.vtx.append(tx2)

        block.hashMerkleRoot = block.calc_merkle_root()
        block.solve()
        return block

    def create_chainlock(self, height, block):
        request_id_buf = ser_string(b"clsig") + struct.pack("<I", height)
        request_id = hash256(request_id_buf)[::-1].hex()
        message_hash = block.hash

        quorum_member = None
        for mn in self.mninfo:
            res = mn.node.quorum('sign', 100, request_id, message_hash)
            if res and quorum_member is None:
                quorum_member = mn

        recSig = self.get_recovered_sig(request_id, message_hash, node=quorum_member.node)
        clsig = msg_clsig(height, block.sha256, hex_str_to_bytes(recSig['sig']))
        return clsig

if __name__ == '__main__':
    LLMQ_IS_CL_Conflicts().main()
