#!/usr/bin/env python3
# Copyright (c) 2015-2022 The Dash Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

'''
feature_llmq_hpmn.py

Checks HPMNs

'''
from _decimal import Decimal
import random

from test_framework.script import hash160
from test_framework.test_framework import DashTestFramework
from test_framework.util import (
    assert_equal, p2p_port
)


def extract_quorum_members(quorum_info):
    return [d['proTxHash'] for d in quorum_info["members"]]

class LLMQHPMNTest(DashTestFramework):
    def set_test_params(self):
        self.set_dash_test_params(5, 4, fast_dip3_enforcement=True, hpmn_count=7)
        self.set_dash_llmq_test_params(4, 4)

    def run_test(self):
        # Connect all nodes to node1 so that we always have the whole network connected
        # Otherwise only masternode connections will be established between nodes, which won't propagate TXs/blocks
        # Usually node0 is the one that does this, but in this test we isolate it multiple times

        for i in range(len(self.nodes)):
            if i != 0:
                self.connect_nodes(i, 0)

        self.activate_dip8()

        self.nodes[0].sporkupdate("SPORK_17_QUORUM_DKG_ENABLED", 0)
        self.wait_for_sporks_same()

        self.mine_quorum(llmq_type_name='llmq_test', llmq_type=100)

        self.log.info("Test that HPMN registration is rejected before v19")
        self.test_hpmn_is_rejected_before_v19()

        self.test_masternode_count(expected_mns_count=4, expected_hpmns_count=0)

        self.activate_v19(expected_activation_height=900)
        self.log.info("Activated v19 at height:" + str(self.nodes[0].getblockcount()))

        self.move_to_next_cycle()
        self.log.info("Cycle H height:" + str(self.nodes[0].getblockcount()))
        self.move_to_next_cycle()
        self.log.info("Cycle H+C height:" + str(self.nodes[0].getblockcount()))
        self.move_to_next_cycle()
        self.log.info("Cycle H+2C height:" + str(self.nodes[0].getblockcount()))

        (quorum_info_i_0, quorum_info_i_1) = self.mine_cycle_quorum(llmq_type_name='llmq_test_dip0024', llmq_type=103)

        hpmn_protxhash_list = list()
        for i in range(5):
            hpmn_info = self.dynamically_add_masternode(hpmn=True)
            hpmn_protxhash_list.append(hpmn_info.proTxHash)
            self.nodes[0].generate(8)
            self.sync_blocks(self.nodes)
            self.test_masternode_count(expected_mns_count=4, expected_hpmns_count=i+1)
            self.test_hpmn_update_service(hpmn_info)

        self.log.info("Test llmq_platform are formed only with HPMNs")
        for i in range(3):
            quorum_i_hash = self.mine_quorum(llmq_type_name='llmq_test_platform', llmq_type=106)
            self.test_quorum_members_are_high_performance(quorum_i_hash, llmq_type=106)

        self.log.info("Test that HPMNs are present in MN list")
        self.test_hpmn_protx_are_in_mnlist(hpmn_protxhash_list)

        self.log.info("Test that HPMNs are paid 4x blocks in a row")
        self.test_hpmmn_payements(window_analysis=256)

        self.log.info(self.nodes[0].masternodelist())

        return

    def test_hpmmn_payements(self, window_analysis):
        current_hpmn = None
        consecutive_paymments = 0
        for i in range(0, window_analysis):
            payee = self.get_mn_payee_for_block(self.nodes[0].getbestblockhash())
            if payee is not None and payee.hpmn:
                if current_hpmn is not None and payee.proTxHash == current_hpmn.proTxHash:
                    # same HPMN
                    assert consecutive_paymments > 0
                    consecutive_paymments += 1
                    consecutive_paymments_rpc = self.nodes[0].protx('info', current_hpmn.proTxHash)['state']['consecutivePayments']
                    assert_equal(consecutive_paymments, consecutive_paymments_rpc)
                else:
                    # new HPMN
                    if current_hpmn is not None:
                        # make sure the old one was paid 4 times in a row
                        assert_equal(consecutive_paymments, 4)
                        consecutive_paymments_rpc = self.nodes[0].protx('info', current_hpmn.proTxHash)['state']['consecutivePayments']
                        # old HPMN should have its nConsecutivePayments reset to 0
                        assert_equal(consecutive_paymments_rpc, 0)
                    current_hpmn = payee
                    consecutive_paymments = 1
                    consecutive_paymments_rpc = self.nodes[0].protx('info', current_hpmn.proTxHash)['state']['consecutivePayments']
                    assert_equal(consecutive_paymments, consecutive_paymments_rpc)
            else:
                # not a HPMN
                if current_hpmn is not None:
                    # make sure the old one was paid 4 times in a row
                    assert_equal(consecutive_paymments, 4)
                    consecutive_paymments_rpc = self.nodes[0].protx('info', current_hpmn.proTxHash)['state']['consecutivePayments']
                    # old HPMN should have its nConsecutivePayments reset to 0
                    assert_equal(consecutive_paymments_rpc, 0)
                current_hpmn = None
                consecutive_paymments = 0

            self.nodes[0].generate(1)
            if i % 8 == 0:
                self.sync_blocks()

    def get_mn_payee_for_block(self, block_hash):
        mn_payee_info = self.nodes[0].masternode("payments", block_hash)[0]
        mn_payee_protx = mn_payee_info['masternodes'][0]['proTxHash']

        mninfos_online = self.mninfo.copy()
        for mn_info in mninfos_online:
            if mn_info.proTxHash == mn_payee_protx:
                return mn_info
        return None

    def test_quorum_members_are_high_performance(self, quorum_hash, llmq_type):
        quorum_info = self.nodes[0].quorum("info", llmq_type, quorum_hash)
        quorum_members = extract_quorum_members(quorum_info)
        mninfos_online = self.mninfo.copy()
        for qm in quorum_members:
            found = False
            for mn in mninfos_online:
                if mn.proTxHash == qm:
                    assert_equal(mn.hpmn, True)
                    found = True
                    break
            assert_equal(found, True)

    def test_hpmn_protx_are_in_mnlist(self, hpmn_protx_list):
        mn_list = self.nodes[0].masternodelist()
        for hpmn_protx in hpmn_protx_list:
            found = False
            for mn in mn_list:
                if mn_list.get(mn)['proTxHash'] == hpmn_protx:
                    found = True
                    assert_equal(mn_list.get(mn)['type'], "HighPerformance")
            assert_equal(found, True)

    def test_hpmn_is_rejected_before_v19(self):
        bls = self.nodes[0].bls('generate')
        collateral_address = self.nodes[0].getnewaddress()
        funds_address = self.nodes[0].getnewaddress()
        owner_address = self.nodes[0].getnewaddress()
        voting_address = self.nodes[0].getnewaddress()
        reward_address = self.nodes[0].getnewaddress()

        collateral_amount = 4000
        collateral_txid = self.nodes[0].sendtoaddress(collateral_address, collateral_amount)
        # send to same address to reserve some funds for fees
        self.nodes[0].sendtoaddress(funds_address, 1)
        collateral_vout = 0
        self.nodes[0].generate(1)
        self.sync_all(self.nodes)

        rawtx = self.nodes[0].getrawtransaction(collateral_txid, 1)
        for txout in rawtx['vout']:
            if txout['value'] == Decimal(collateral_amount):
                collateral_vout = txout['n']
                break
        assert collateral_vout is not None

        ipAndPort = '127.0.0.1:%d' % p2p_port(len(self.nodes))
        operatorReward = len(self.nodes)

        self.nodes[0].generate(1)

        protx_success = False
        try:
            self.nodes[0].protx('register_hpmn', collateral_txid, collateral_vout, ipAndPort, owner_address, bls['public'], voting_address, operatorReward, reward_address, funds_address, True)
            protx_success = True
        except:
            self.log.info("protx_hpmn rejected")
        assert_equal(protx_success, False)

    def test_masternode_count(self, expected_mns_count, expected_hpmns_count):
        mn_count = self.nodes[0].masternode('count')
        assert_equal(mn_count['total'], expected_mns_count + expected_hpmns_count)
        detailed_count = mn_count['detailed']
        assert_equal(detailed_count['regular']['total'], expected_mns_count)
        assert_equal(detailed_count['hpmn']['total'], expected_hpmns_count)

    def test_hpmn_update_service(self, hpmn_info):
        funds_address = self.nodes[0].getnewaddress()
        operator_reward_address = self.nodes[0].getnewaddress()

        # For the sake of the test, generate random nodeid, p2p and http platform values
        rnd = random.randint(1000, 65000)
        platform_node_id = hash160(b'%d' % rnd).hex()
        platform_p2p_port = '%d' % (rnd + 1)
        platform_http_port = '%d' % (rnd + 2)

        self.nodes[0].sendtoaddress(funds_address, 1)
        self.nodes[0].generate(1)
        self.sync_all(self.nodes)

        self.nodes[0].protx('update_service_hpmn', hpmn_info.proTxHash, hpmn_info.addr, hpmn_info.keyOperator, platform_node_id, platform_p2p_port, platform_http_port, operator_reward_address, funds_address)
        self.nodes[0].generate(1)
        self.sync_all(self.nodes)
        self.log.info("Updated HPMN %s: platformNodeID=%s, platformP2PPort=%s, platformHTTPPort=%s" % (hpmn_info.proTxHash, platform_node_id, platform_p2p_port, platform_http_port))

if __name__ == '__main__':
    LLMQHPMNTest().main()
