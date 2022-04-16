#!/usr/bin/env python3
# Copyright (c) 2015-2021 The Dash Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

'''
feature_llmq_rotation.py

Checks LLMQs Quorum Rotation

'''
import time
from test_framework.test_framework import DashTestFramework
from test_framework.util import (
    assert_equal,
    assert_greater_than_or_equal,
    connect_nodes,
    sync_blocks,
    wait_until,
)


def intersection(lst1, lst2):
    lst3 = [value for value in lst1 if value in lst2]
    return lst3


def extract_quorum_members(quorum_info):
    return [d['proTxHash'] for d in quorum_info["members"]]


class LLMQQuorumRotationTest(DashTestFramework):
    def set_test_params(self):
        self.set_dash_test_params(16, 15, fast_dip3_enforcement=True)
        self.set_dash_llmq_test_params(4, 4)

    def run_test(self):

        llmq_type=103
        llmq_type_name="llmq_test_dip0024"

        # Connect all nodes to node1 so that we always have the whole network connected
        # Otherwise only masternode connections will be established between nodes, which won't propagate TXs/blocks
        # Usually node0 is the one that does this, but in this test we isolate it multiple times

        for i in range(len(self.nodes)):
            if i != 1:
                connect_nodes(self.nodes[i], 0)

        self.activate_dip8()

        self.nodes[0].spork("SPORK_17_QUORUM_DKG_ENABLED", 0)
        self.wait_for_sporks_same()

        self.activate_dip0024(expected_activation_height=900)
        self.log.info("Activated DIP0024 at height:" + str(self.nodes[0].getblockcount()))

        #At this point, we need to move forward 3 cycles (3 x 24 blocks) so the first 3 quarters can be created (without DKG sessions)
        #self.log.info("Start at H height:" + str(self.nodes[0].getblockcount()))
        self.move_to_next_cycle()
        self.log.info("Cycle H height:" + str(self.nodes[0].getblockcount()))
        self.move_to_next_cycle()
        self.log.info("Cycle H+C height:" + str(self.nodes[0].getblockcount()))
        self.move_to_next_cycle()
        self.log.info("Cycle H+2C height:" + str(self.nodes[0].getblockcount()))

        (quorum_info_0_0, quorum_info_0_1) = self.mine_cycle_quorum(llmq_type_name=llmq_type_name, llmq_type=llmq_type)
        quorum_members_0_0 = extract_quorum_members(quorum_info_0_0)
        quorum_members_0_1 = extract_quorum_members(quorum_info_0_1)
        assert_equal(len(intersection(quorum_members_0_0, quorum_members_0_1)), 0)
        self.log.info("Quorum #0_0 members: " + str(quorum_members_0_0))
        self.log.info("Quorum #0_1 members: " + str(quorum_members_0_1))

        (quorum_info_1_0, quorum_info_1_1) = self.mine_cycle_quorum(llmq_type_name=llmq_type_name, llmq_type=llmq_type)
        quorum_members_1_0 = extract_quorum_members(quorum_info_1_0)
        quorum_members_1_1 = extract_quorum_members(quorum_info_1_1)
        assert_equal(len(intersection(quorum_members_1_0, quorum_members_1_1)), 0)
        self.log.info("Quorum #1_0 members: " + str(quorum_members_1_0))
        self.log.info("Quorum #1_1 members: " + str(quorum_members_1_1))

        (quorum_info_2_0, quorum_info_2_1) = self.mine_cycle_quorum(llmq_type_name=llmq_type_name, llmq_type=llmq_type)
        quorum_members_2_0 = extract_quorum_members(quorum_info_2_0)
        quorum_members_2_1 = extract_quorum_members(quorum_info_2_1)
        assert_equal(len(intersection(quorum_members_2_0, quorum_members_2_1)), 0)
        self.log.info("Quorum #2_0 members: " + str(quorum_members_2_0))
        self.log.info("Quorum #2_1 members: " + str(quorum_members_2_1))

        mninfos_online = self.mninfo.copy()
        nodes = [self.nodes[0]] + [mn.node for mn in mninfos_online]
        sync_blocks(nodes)
        quorum_list = self.nodes[0].quorum("list", llmq_type)
        quorum_blockhash = self.nodes[0].getbestblockhash()
        fallback_blockhash = self.nodes[0].generate(1)[0]
        self.log.info("h("+str(self.nodes[0].getblockcount())+") quorum_list:"+str(quorum_list))

        assert_greater_than_or_equal(len(intersection(quorum_members_0_0, quorum_members_1_0)), 3)
        assert_greater_than_or_equal(len(intersection(quorum_members_0_1, quorum_members_1_1)), 3)

        assert_greater_than_or_equal(len(intersection(quorum_members_0_0, quorum_members_2_0)), 2)
        assert_greater_than_or_equal(len(intersection(quorum_members_0_1, quorum_members_2_1)), 2)

        assert_greater_than_or_equal(len(intersection(quorum_members_1_0, quorum_members_2_0)), 3)
        assert_greater_than_or_equal(len(intersection(quorum_members_1_1, quorum_members_2_1)), 3)

        self.log.info("mine a quorum to invalidate")
        (quorum_info_3_0, quorum_info_3_1) = self.mine_cycle_quorum(llmq_type_name=llmq_type_name, llmq_type=llmq_type)

        new_quorum_list = self.nodes[0].quorum("list", llmq_type)
        assert_equal(len(new_quorum_list[llmq_type_name]), len(quorum_list[llmq_type_name]) + 2)
        new_quorum_blockhash = self.nodes[0].getbestblockhash()
        self.log.info("h("+str(self.nodes[0].getblockcount())+") new_quorum_blockhash:"+new_quorum_blockhash)
        self.log.info("h("+str(self.nodes[0].getblockcount())+") new_quorum_list:"+str(new_quorum_list))
        assert new_quorum_list != quorum_list

        self.log.info("Invalidate the quorum")
        self.bump_mocktime(5)
        self.nodes[0].spork("SPORK_19_CHAINLOCKS_ENABLED", 4070908800)
        self.wait_for_sporks_same()
        self.nodes[0].invalidateblock(fallback_blockhash)
        assert_equal(self.nodes[0].getbestblockhash(), quorum_blockhash)
        assert_equal(self.nodes[0].quorum("list", llmq_type), quorum_list)

        self.log.info("Reconsider the quorum")
        self.bump_mocktime(5)
        self.nodes[0].spork("SPORK_19_CHAINLOCKS_ENABLED", 0)
        self.wait_for_sporks_same()
        self.nodes[0].reconsiderblock(fallback_blockhash)
        wait_until(lambda: self.nodes[0].getbestblockhash() == new_quorum_blockhash, sleep=1)
        assert_equal(self.nodes[0].quorum("list", llmq_type), new_quorum_list)

    def move_to_next_cycle(self):
        cycle_length = 24
        mninfos_online = self.mninfo.copy()
        nodes = [self.nodes[0]] + [mn.node for mn in mninfos_online]
        cur_block = self.nodes[0].getblockcount()

        # move forward to next DKG
        skip_count = cycle_length - (cur_block % cycle_length)
        if skip_count != 0:
            self.bump_mocktime(1, nodes=nodes)
            self.nodes[0].generate(skip_count)
        sync_blocks(nodes)
        time.sleep(1)
        self.log.info('Moved from block %d to %d' % (cur_block, self.nodes[0].getblockcount()))


if __name__ == '__main__':
    LLMQQuorumRotationTest().main()
