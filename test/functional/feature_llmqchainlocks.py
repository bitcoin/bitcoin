#!/usr/bin/env python3
# Copyright (c) 2015-2020 The Dash Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
import time
import struct
from test_framework.test_framework import DashTestFramework
from test_framework.messages import CInv, CTransaction, CTxOut, hash256, msg_btccsig, msg_clsig, msg_getdata, msg_inv, ser_string, tx_from_hex, uint256_from_str
from test_framework.p2p import (
  P2PInterface,
)
from test_framework.authproxy import JSONRPCException
from test_framework.blocktools import (
  create_block,
  create_coinbase,
  add_witness_commitment,
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

class TestP2PBTCCObserver(P2PInterface):
    def __init__(self):
        super().__init__()
        self.btccsigs = {}
        self.last_inv = []

    def on_inv(self, message):
        want = []
        for inv in message.inv:
            if inv.type == 21:  # MSG_BTCCSIG
                want.append(inv)
        self.last_inv = want
        if want:
            self.send_message(msg_getdata(want))

    def on_btccsig(self, message):
        btcc_hash = uint256_from_str(hash256(message.serialize()))
        self.btccsigs[btcc_hash] = message

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
            for j in range(i + 1, len(self.nodes)):
                self.connect_nodes(i, j, wait_for_connect=False)
            
        self.generate(self.nodes[0], 10)
        self.sync_blocks(self.nodes, timeout=60*5)
        self.nodes[0].spork("SPORK_17_QUORUM_DKG_ENABLED", 0)
        self.nodes[0].spork("SPORK_19_CHAINLOCKS_ENABLED", 0)
        self.wait_for_sporks_same()

        self.log.info("Mining 4 quorums")
        for i in range(4):
            self.mine_quorum()

        self.wait_for_sporks_same()
        self.log.info("Mine single block, wait for chainlock")
        cl = self.nodes[0].getbestblockhash()
        self.generate(self.nodes[0], 5)

        self.wait_for_chainlocked_block_all_nodes(cl)

        # BTC checkpoint sigs must be relayed via standard INV/GETDATA so that non-quorum-relay nodes
        # (e.g. miners) can reliably learn the aggregated btccsig without requesting recovered sigs.
        self.log.info("Wait for BTCCSIG INV relay to non-masternode node")
        p2p_btcc = self.nodes[3].add_p2p_connection(TestP2PBTCCObserver())
        self.generate(self.nodes[0], 22)
        self.sync_blocks(self.nodes, timeout=60*5)
        self.wait_until(lambda: len(p2p_btcc.btccsigs) > 0, timeout=60)
        assert any(m.height >= 0 and len(m.sig) == 96 for m in p2p_btcc.btccsigs.values())
        # Also ensure we saw at least one BTCCSIG inv (share or aggregate) via the standard inv/getdata path.
        assert len(p2p_btcc.last_inv) > 0

        self.log.info("Stop half the nodes to ensure chainlocks don't form")
        cl0_before_split = self.nodes[0].getbestchainlock()['blockhash']
        cl1_before_split = self.nodes[1].getbestchainlock()['blockhash']
        self.stop_node(2)
        self.stop_node(3)
        self.log.info("Check recent_chainlock forms but not an active chainlock")
        self.generate(self.nodes[0], 5, sync_fun=self.no_op)
        time.sleep(5)
        # bump cleanup for seen shares
        self.bump_mocktime(30, self.nodes[0:2])
        assert self.nodes[0].getbestchainlock()['blockhash'] == cl0_before_split
        assert self.nodes[1].getbestchainlock()['blockhash'] == cl1_before_split
        self.log.info("Start nodes again and check for chainlock")
        self.start_node(2, extra_args=["-mocktime=" + str(self.mocktime), '-reindex', *self.extra_args[2]])
        self.start_node(3, extra_args=["-mocktime=" + str(self.mocktime), '-reindex', *self.extra_args[3]])
        for i in range(len(self.nodes)):
            force_finish_mnsync(self.nodes[i])
        for i in range(len(self.nodes)):
            for j in range(i + 1, len(self.nodes)):
                self.connect_nodes(i, j, wait_for_connect=False)
        self.sync_all()
        node0_mining_addr = self.nodes[0].getnewaddress()
        cl = self.mine_until_new_chainlock(self.nodes[0], node0_mining_addr, sync_nodes=self.nodes)
        self.wait_for_chainlocked_block_all_nodes(cl)
        self.log.info("Restart miner and ensure chainlock is sent via P2P")
        self.stop_node(0)
        self.start_node(0)
        self.connect_nodes(0, 1)
        # just mnsync the first blockchain step and skip over governance
        self.sync_mnsync([self.nodes[0]], "blockchain")
        force_finish_mnsync(self.nodes[0])
        self.wait_for_chainlocked_block(self.nodes[0], cl)
        
        self.log.info("Mine many blocks, wait for chainlock")

        self.generate(self.nodes[0], 20)
        self.sync_blocks(self.nodes, timeout=60*5)
        cl = self.mine_until_new_chainlock(self.nodes[0], node0_mining_addr, max_blocks=20, sync_nodes=self.nodes)
        self.wait_for_chainlocked_block_all_nodes(cl, timeout=60)
        self.log.info("Assert that blocks up to current chainlock height are chainlocked")
        chainlocked_height = self.nodes[0].getbestchainlock()["height"]
        for h in range(1, chainlocked_height + 1):
            block = self.nodes[0].getblock(self.nodes[0].getblockhash(h))
            assert block['chainlock']

        self.log.info("Isolate node, mine on another, and reconnect")
        self.isolate_node(self.nodes[0])
        for i in range(1, len(self.nodes)):
            for j in range(i + 1, len(self.nodes)):
                self.connect_nodes(i, j, wait_for_connect=False)
        node0_tip = self.nodes[0].getbestblockhash()
        prev_cl = self.nodes[1].getbestchainlock()["blockhash"]
        self.generatetoaddress(self.nodes[1], 10, node0_mining_addr, sync_fun=self.no_op)
        self.wait_until(lambda: self.nodes[1].getbestchainlock()["blockhash"] != prev_cl, timeout=120)
        cl = self.nodes[1].getbestchainlock()["blockhash"]
        self.wait_for_chainlocked_block(self.nodes[1], cl)
        assert self.nodes[0].getbestblockhash() == node0_tip
        self.reconnect_isolated_node(self.nodes[0], 1)
        for i in range(1, len(self.nodes)):
            self.connect_nodes(0, i, wait_for_connect=False)
        self.wait_until(lambda: self.nodes[0].getbestblockhash() == self.nodes[1].getbestblockhash())

        self.log.info("Isolate node, mine on another, reconnect and submit CL via RPC")
        self.isolate_node(self.nodes[0])
        for i in range(1, len(self.nodes)):
            for j in range(i + 1, len(self.nodes)):
                self.connect_nodes(i, j, wait_for_connect=False)
        cl = self.mine_until_new_chainlock(self.nodes[1], node0_mining_addr, sync_nodes=self.nodes[1:])
        self.wait_for_chainlocked_block(self.nodes[1], cl)
        best_0 = self.nodes[0].getbestchainlock()
        best_1 = self.nodes[1].getbestchainlock()
        assert best_0['blockhash'] != best_1['blockhash']
        assert best_0['height'] != best_1['height']
        assert best_0['signature'] != best_1['signature']
        assert_equal(best_0['known_block'], True)
        node_height = self.nodes[1].submitchainlock(best_0['blockhash'], best_0['signature'], best_0['signers'])
        # future CLSIG, will not submit
        assert_raises_rpc_error(-8, 'mismatch-clsig-height', self.nodes[0].submitchainlock, best_1['blockhash'], best_1['signature'], best_1['signers'])
        assert_equal(best_1['height'], node_height)
        best_0 = self.nodes[0].getbestchainlock()
        assert best_0['blockhash'] != best_1['blockhash']
        assert best_0['height'] != best_1['height']
        assert best_0['signature'] != best_1['signature']
        assert_equal(best_0['known_block'], True)
        self.reconnect_isolated_node(self.nodes[0], 1)
        for i in range(1, len(self.nodes)):
            self.connect_nodes(0, i, wait_for_connect=False)
        self.wait_until(lambda: self.nodes[0].getbestblockhash() == self.nodes[1].getbestblockhash())
        cl = self.mine_until_new_chainlock(self.nodes[1], node0_mining_addr, sync_nodes=self.nodes[1:])
        self.wait_for_chainlocked_block(self.nodes[1], cl)
        self.wait_for_chainlocked_block(self.nodes[0], cl)
        
        self.log.info("Isolate node, mine on both parts of the network, and reconnect")
        self.isolate_node(self.nodes[0])
        for i in range(1, len(self.nodes)):
            for j in range(i + 1, len(self.nodes)):
                self.connect_nodes(i, j, wait_for_connect=False)
        # node 0 creates longer chain of 15 blocks but no chainlocks
        self.generate(self.nodes[0], 15, sync_fun=self.no_op)
        bad_cl = self.nodes[0].getbestblockhash()
        bad_tip = self.nodes[0].getbestblockhash()
        # node 1 creates shorter chain of 10 blocks which is chainlocked (this is the canonical chain because sentry nodes see it)
        node1_mining_addr = self.nodes[1].get_deterministic_priv_key().address
        prev_cl = self.nodes[1].getbestchainlock()["blockhash"]
        self.generatetoaddress(self.nodes[1], 10, node1_mining_addr, sync_fun=self.no_op)
        self.wait_until(lambda: self.nodes[1].getbestchainlock()["blockhash"] != prev_cl, timeout=120)
        good_cl = self.nodes[1].getbestchainlock()["blockhash"]
        self.wait_for_chainlocked_block(self.nodes[1], good_cl, timeout=120)
        assert not self.nodes[0].getblock(bad_cl)["chainlock"]
        self.reconnect_isolated_node(self.nodes[0], 1)
        for i in range(1, len(self.nodes)):
            self.connect_nodes(0, i, wait_for_connect=False)
        self.bump_mocktime(5, nodes=self.nodes)
        prev_cl = self.nodes[1].getbestchainlock()["blockhash"]
        # create a new CLSIG
        good_tip = self.generatetoaddress(self.nodes[1], 5, node1_mining_addr, sync_fun=self.no_op)[-1]
        self.wait_until(lambda: self.nodes[1].getbestchainlock()["blockhash"] != prev_cl, timeout=120)
        good_cl = self.nodes[1].getbestchainlock()["blockhash"]
        # since shorter chain was chainlocked and its the canonical valid chain, node with longer chain should switch to it
        self.wait_for_chainlocked_block(self.nodes[0], good_cl, timeout=120)
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

        self.log.info("Keep node connected and let it try to reorg the chain")
        good_cl = self.mine_until_new_chainlock(self.nodes[0], node0_mining_addr, sync_nodes=self.nodes)
        good_tip = self.nodes[0].getbestblockhash()
        self.wait_for_chainlocked_block_all_nodes(good_cl, timeout=120)
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
        prev_cl = self.nodes[1].getbestchainlock()["blockhash"]
        self.generatetoaddress(self.nodes[1], 5, node0_mining_addr, sync_fun=self.no_op)
        self.wait_until(lambda: self.nodes[1].getbestchainlock()["blockhash"] != prev_cl, timeout=120)
        good_cl = self.nodes[1].getbestchainlock()["blockhash"]
        good_tip = self.nodes[1].getbestblockhash()
        self.wait_for_chainlocked_block(self.nodes[0], good_cl)
        assert self.nodes[0].getbestblockhash() == good_tip
        found = False
        foundConflict = False
        forkChain = None
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
        force_finish_mnsync(self.nodes[0])
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
        for i in range(1, len(self.nodes)):
            self.connect_nodes(0, i, wait_for_connect=False)
        node0_cl = self.mine_until_new_chainlock(self.nodes[0], node0_mining_addr, sync_nodes=self.nodes)
        self.wait_for_chainlocked_block(self.nodes[0], node0_cl)

        self.log.info("Send fake future clsigs and see if this breaks ChainLocks")
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
        new_cl = self.mine_until_new_chainlock(self.nodes[0], node0_mining_addr, sync_nodes=self.nodes)
        self.wait_for_chainlocked_block_all_nodes(new_cl, timeout=120)
        self.log.info("Allow signing on competing chains")
        self.generate(self.nodes[2], 5, sync_fun=self.no_op)
        # create a fork
        self.generate(self.nodes[1], 5, sync_fun=self.no_op)
        time.sleep(5)
        # Continue by observing the next real chainlock instead of assuming a fixed offset.
        new_cl = self.mine_until_new_chainlock(self.nodes[0], node0_mining_addr, sync_nodes=self.nodes)
        self.wait_for_chainlocked_block_all_nodes(new_cl, timeout=60)

        self.log.info("Reject bad BTC checkpoint receipt linkage during IBD and reorg away from garbage receipts")
        self.test_clreceipt_ibd_reject_and_chainlock_reorg()

        self.nodes[0].disconnect_p2ps()

    def mine_until_new_chainlock(self, miner, address, *, max_blocks=40, sync_nodes=None):
        prev_cl = None
        try:
            prev_cl = miner.getbestchainlock()["blockhash"]
        except JSONRPCException:
            # Some scenarios intentionally clear chainlock state.
            pass
        for _ in range(max_blocks):
            self.generatetoaddress(miner, 1, address, sync_fun=self.no_op)
            if sync_nodes:
                self.sync_blocks(sync_nodes, timeout=60)
            # Give chainlock scheduler/recovery time after each new block, without
            # repeatedly emitting wait_until timeout noise in logs.
            for _ in range(4):
                self.bump_mocktime(1, nodes=self.nodes)
                try:
                    cur_cl = miner.getbestchainlock()["blockhash"]
                    if prev_cl is None or cur_cl != prev_cl:
                        return cur_cl
                except JSONRPCException:
                    pass
                time.sleep(0.25)
        raise AssertionError("No new chainlock after mining additional blocks")

    def make_block_from_gbt_with_extradata(self, node, extradata: bytes):
        """
        Build a fully consensus-valid block from getblocktemplate(), preserving the
        coinbase outputs (e.g. masternode payees), but overriding the Syscoin
        extraData push (used by GetSyscoinData) via the witness commitment output.
        """
        tmpl = node.getblocktemplate({"rules": ["segwit"]})

        base_extra = bytes.fromhex(tmpl.get("default_witness_commitment_extra", ""))
        if base_extra == b"":
            raise AssertionError("getblocktemplate missing default_witness_commitment_extra")

        # Syscoin GBT does not necessarily expose coinbasetxn; construct coinbase from template fields.
        coinbase = create_coinbase(height=tmpl["height"])
        coinbase.nVersion = tmpl.get("version_coinbase", coinbase.nVersion)
        coinbase.vout[0].nValue = tmpl["coinbasevalue"]

        for mn_out in tmpl.get("masternode", []):
            coinbase.vout.append(CTxOut(mn_out["amount"], bytes.fromhex(mn_out["script"])))
        for sb_out in tmpl.get("superblock", []):
            coinbase.vout.append(CTxOut(sb_out["amount"], bytes.fromhex(sb_out["script"])))

        # Use the witness-commitment extra push for Syscoin data.
        # Start from the template-provided extra (which includes quorum commitment payload when required),
        # then replace the clreceipt segment as requested by this test.
        coinbase.extraData = extradata if extradata is not None else base_extra
        coinbase.rehash()

        txlist = []
        for e in tmpl.get("transactions", []):
            txlist.append(tx_from_hex(e["data"]))

        block = create_block(tmpl=tmpl, coinbase=coinbase, txlist=txlist)
        add_witness_commitment(block, nonce=0)
        block.solve()
        return block

    @staticmethod
    def strip_trailing_clrc(extra: bytes) -> bytes:
        """
        Miner appends btcc at the end of the extraData stream; remove the last occurrence and everything after it.
        """
        marker = b"btcc"
        idx = extra.rfind(marker)
        if idx == -1:
            return extra
        return extra[:idx]

    def block_has_clreceipt_marker(self, node, block_hash):
        block = node.getblock(block_hash)
        coinbase_txid = block["tx"][0]
        coinbase = node.getrawtransaction(coinbase_txid, True, block_hash)
        for vout in coinbase["vout"]:
            spk_hex = vout["scriptPubKey"]["hex"]
            if "62746363" in spk_hex:
                return True
        return False

    def test_clreceipt_ibd_reject_and_chainlock_reorg(self):
        """
        - Ensure carrier blocks missing 'btcc' are rejected even while IBD=true (cheap deterministic checks).
        - Ensure a node which accepted an invalid-sig btcc block while IBD=true will reorg away once a CLSIG
          forms on the canonical branch at/after the divergence height.
        - Ensure invalid-sig btcc is rejected when IBD=false.
        """
        node0 = self.nodes[0]
        node1 = self.nodes[1]

        # Bring everyone to a known height where next block is carrier height (height % 10 == 6 -> next is 7)
        tip_height = node1.getblockcount()
        blocks_to_6 = (6 - (tip_height % 10)) % 10
        if blocks_to_6:
            self.generate(node1, blocks_to_6)
            self.sync_blocks(self.nodes, timeout=60*5)
        assert node1.getblockcount() % 10 == 6

        common_prev_hash = node1.getbestblockhash()
        common_prev = node1.getblock(common_prev_hash)
        common_prev_time = common_prev["time"]
        target_height = node1.getblockcount() + 1

        # (pre) When IBD=false, an invalid btcc signature must be rejected.
        tmpl1 = node1.getblocktemplate({"rules": ["segwit"]})
        base_extra1 = bytes.fromhex(tmpl1.get("default_witness_commitment_extra", ""))
        expected_receipt_height = target_height - 5
        expected_ancestor_hash = node1.getblockhash(expected_receipt_height)
        receipt_bad_sig = msg_btccsig(height=expected_receipt_height, sysHash=int(expected_ancestor_hash, 16), sig=b"\x01" + b"\x00" * 95, signers=[])
        bad_sig_payload_1 = self.strip_trailing_clrc(base_extra1) + b"btcc" + receipt_bad_sig.serialize()
        bad_block_invalid_sig = self.make_block_from_gbt_with_extradata(node1, bad_sig_payload_1)
        assert_equal(node1.submitblock(hexdata=bad_block_invalid_sig.serialize().hex()), "bad-btcc-sig")

        # Restart node0 with a far-future mocktime so IBD stays latched true
        # (IsInitialBlockDownload() latches false once it exits, so we need a restart to re-enter IBD for testing).
        far_future = common_prev_time + 3 * 24 * 60 * 60
        self.stop_node(0)
        self.start_node(0, extra_args=[f"-mocktime={far_future}", *self.extra_args[0]])
        node0 = self.nodes[0]
        force_finish_mnsync(node0)
        self.connect_nodes(0, 1, wait_for_connect=False)
        self.sync_blocks([node0, node1], timeout=60*5)
        assert node0.getblockchaininfo()["initialblockdownload"]

        # Now isolate node0 to build a competing chain
        self.isolate_node(node0)

        # (A) Verify that a carrier block without the marker is rejected while IBD=true.
        # Start from a valid template extra and remove the trailing btcc segment.
        tmpl0 = node0.getblocktemplate({"rules": ["segwit"]})
        base_extra0 = bytes.fromhex(tmpl0.get("default_witness_commitment_extra", ""))
        missing_payload = self.strip_trailing_clrc(base_extra0)
        bad_block_missing = self.make_block_from_gbt_with_extradata(node0, missing_payload)
        assert_equal(node0.submitblock(hexdata=bad_block_missing.serialize().hex()), "bad-btcc-missing")

        # (B) Build an alternative carrier block that has correct linkage but garbage signature bytes (accepted while IBD=true)
        expected_receipt_height = target_height - 5
        expected_ancestor_hash = node0.getblockhash(expected_receipt_height)
        receipt = msg_btccsig(height=expected_receipt_height, sysHash=int(expected_ancestor_hash, 16), sig=b"\x01" + b"\x00" * 95, signers=[])
        # Build from the valid template extra and replace the btcc receipt with a garbage signature.
        bad_sig_payload = self.strip_trailing_clrc(base_extra0) + b"btcc" + receipt.serialize()
        bad_sig_block = self.make_block_from_gbt_with_extradata(node0, bad_sig_payload)
        assert_equal(node0.submitblock(hexdata=bad_sig_block.serialize().hex()), None)
        assert_equal(node0.getblockcount(), target_height)

        bad_sig_hash = bad_sig_block.hash

        # Mine the canonical carrier block and then enough blocks so that a CLSIG forms for it (CL sign offset 5)
        node1_mining_addr = node1.get_deterministic_priv_key().address
        good_h = self.generate(node1, 1, sync_fun=self.no_op)[-1]
        assert self.block_has_clreceipt_marker(node1, good_h)
        self.generate(node1, 5, sync_fun=self.no_op)
        good_cl = self.mine_until_new_chainlock(node1, node1_mining_addr, sync_nodes=self.nodes[1:])
        self.wait_for_chainlocked_block(node1, good_cl)

        # Also ensure a btcc aggregation is visible via RPC for debugging/tests.
        self.wait_until(
            lambda: node1.getbestbtccheckpoint()["height"] >= expected_receipt_height,
            timeout=120
        )

        # Reconnect node0 and ensure it reorgs away from its bad-sig block after another clsig
        self.reconnect_isolated_node(node0, 1)
        for i in range(1, len(self.nodes)):
            self.connect_nodes(0, i, wait_for_connect=False)
        self.wait_until(lambda: node0.getbestblockhash() == node1.getbestblockhash())

        # Bad branch tip should be marked conflicting
        found = False
        for tip in node0.getchaintips():
            if tip["hash"] == bad_sig_hash:
                assert tip["status"] != "active"
                found = True
                break
        assert found

        # Restore node0 back to the framework mocktime for subsequent test steps
        self.stop_node(0)
        self.start_node(0, extra_args=[f"-mocktime={self.mocktime}", *self.extra_args[0]])
        node0 = self.nodes[0]
        force_finish_mnsync(node0)
        for i in range(1, len(self.nodes)):
            self.connect_nodes(0, i, wait_for_connect=False)
        self.sync_all()

    def create_fake_clsig(self, height_offset):
        # create a fake block height_offset blocks ahead of the tip
        height = self.nodes[0].getblockcount() + height_offset
        fake_block = create_block(0xff, create_coinbase(height))
        # create a fake clsig for that block
        quorum_hash = self.nodes[0].quorum_list(1)["quorums"][0]
        request_id_buf = ser_string(b"clsig") + struct.pack("<I", height)
        request_id_buf += bytes.fromhex(quorum_hash)[::-1]
        request_id = hash256(request_id_buf)[::-1].hex()
        quorum_hash = self.nodes[0].quorum_list(1)["quorums"][0]
        for mn in self.mninfo:
            mn.node.quorum_sign(request_id, fake_block.hash, quorum_hash)
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
