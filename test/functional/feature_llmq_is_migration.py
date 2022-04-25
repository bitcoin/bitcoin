#!/usr/bin/env python3
# Copyright (c) 2020-2021 The Dash Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
import time

from test_framework.messages import CTransaction, FromHex, hash256, ser_compact_size, ser_string
from test_framework.test_framework import DashTestFramework
from test_framework.util import wait_until, connect_nodes

'''
feature_llmq_is_migration.py

Test IS LLMQ migration with DIP0024

'''

class LLMQISMigrationTest(DashTestFramework):
    def set_test_params(self):
        # -whitelist is needed to avoid the trickling logic on node0
        self.set_dash_test_params(16, 15, [["-whitelist=127.0.0.1"], [], [], [], [], [], [], [], [], [], [], [], [], [], [], []], fast_dip3_enforcement=True)
        self.set_dash_llmq_test_params(4, 4)

    def get_request_id(self, tx_hex):
        tx = FromHex(CTransaction(), tx_hex)

        request_id_buf = ser_string(b"islock") + ser_compact_size(len(tx.vin))
        for txin in tx.vin:
            request_id_buf += txin.prevout.serialize()
        return hash256(request_id_buf)[::-1].hex()

    def run_test(self):

        for i in range(len(self.nodes)):
            if i != 1:
                connect_nodes(self.nodes[i], 0)

        self.activate_dip8()

        node = self.nodes[0]
        node.spork("SPORK_17_QUORUM_DKG_ENABLED", 0)
        node.spork("SPORK_2_INSTANTSEND_ENABLED", 0)
        self.wait_for_sporks_same()

        self.mine_quorum()
        self.mine_quorum()

        self.log.info(self.nodes[0].quorum("list"))

        txid1 = node.sendtoaddress(node.getnewaddress(), 1)
        self.wait_for_instantlock(txid1, node)

        request_id = self.get_request_id(self.nodes[0].getrawtransaction(txid1))
        wait_until(lambda: node.quorum("hasrecsig", 104, request_id, txid1))

        rec_sig = node.quorum("getrecsig", 104, request_id, txid1)['sig']
        assert node.verifyislock(request_id, txid1, rec_sig)

        self.activate_dip0024()
        self.log.info("Activated DIP0024 at height:" + str(self.nodes[0].getblockcount()))

        q_list = self.nodes[0].quorum("list")
        self.log.info(q_list)
        assert len(q_list['llmq_test']) == 2
        assert len(q_list['llmq_test_instantsend']) == 2
        assert len(q_list['llmq_test_v17']) == 0
        assert len(q_list['llmq_test_dip0024']) == 0

        txid1 = node.sendtoaddress(node.getnewaddress(), 1)
        self.wait_for_instantlock(txid1, node)


        # at this point, DIP0024 is active, but we have old quorums, and they should still create islocks!

        txid3 = node.sendtoaddress(node.getnewaddress(), 1)
        self.wait_for_instantlock(txid3, node)

        request_id = self.get_request_id(self.nodes[0].getrawtransaction(txid3))
        wait_until(lambda: node.quorum("hasrecsig", 104, request_id, txid3))

        rec_sig = node.quorum("getrecsig", 104, request_id, txid3)['sig']
        assert node.verifyislock(request_id, txid3, rec_sig)


        #At this point, we need to move forward 3 cycles (3 x 24 blocks) so the first 3 quarters can be created (without DKG sessions)
        #self.log.info("Start at H height:" + str(self.nodes[0].getblockcount()))
        self.move_to_next_cycle()
        self.log.info("Cycle H height:" + str(self.nodes[0].getblockcount()))
        self.move_to_next_cycle()
        self.log.info("Cycle H+C height:" + str(self.nodes[0].getblockcount()))
        self.move_to_next_cycle()
        self.log.info("Cycle H+2C height:" + str(self.nodes[0].getblockcount()))

        (quorum_info_0_0, quorum_info_0_1) = self.mine_cycle_quorum()

        q_list = self.nodes[0].quorum("list")
        self.log.info(q_list)
        assert len(q_list['llmq_test']) == 2
        assert 'llmq_test_instantsend' not in q_list
        assert len(q_list['llmq_test_v17']) == 1
        assert len(q_list['llmq_test_dip0024']) == 2

        # Check that the earliest islock is still present
        self.wait_for_instantlock(txid1, node)

        txid2 = node.sendtoaddress(node.getnewaddress(), 1)
        self.wait_for_instantlock(txid2, node)

        request_id2 = self.get_request_id(self.nodes[0].getrawtransaction(txid2))
        wait_until(lambda: node.quorum("hasrecsig", 103, request_id2, txid2))

        rec_sig2 = node.quorum("getrecsig", 103, request_id2, txid2)['sig']
        assert node.verifyislock(request_id2, txid2, rec_sig2)

        # Check that original islock quorum type doesn't sign
        time.sleep(10)
        for n in self.nodes:
            assert not n.quorum("hasrecsig", 104, request_id2, txid2)


if __name__ == '__main__':
    LLMQISMigrationTest().main()
