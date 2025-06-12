#!/usr/bin/env python3
# Copyright (c) 2015-2024 The Dash Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

'''
feature_llmq_singlenode.py

Checks creating LLMQs signle node quorum creation and signing
This functional test is similar to feature_llmq_signing.py but difference are big enough to make implementation common.

'''

import time

from test_framework.authproxy import JSONRPCException
from test_framework.test_framework import (
    DashTestFramework,
    MasternodeInfo,
)
from test_framework.util import (
    assert_raises_rpc_error,
    assert_greater_than,
    wait_until_helper,
)


id = "0000000000000000000000000000000000000000000000000000000000000001"
msgHash = "0000000000000000000000000000000000000000000000000000000000000002"
msgHashConflict = "0000000000000000000000000000000000000000000000000000000000000003"


q_type=100
class LLMQSigningTest(DashTestFramework):
    def set_test_params(self):
        self.set_dash_test_params(1, 0, [["-llmqtestinstantsenddip0024=llmq_test_instantsend", "-peertimeout=300000000"]],
                evo_count=2)
        self.set_dash_llmq_test_params(1, 1)


    def mine_single_node_quorum(self):
        node = self.nodes[0]
        quorums = node.quorum('list')['llmq_test']

        skip_count = 24 - (self.nodes[0].getblockcount() % 24)
        if skip_count != 0:
            self.bump_mocktime(1)
            self.generate(self.nodes[0], skip_count)
        time.sleep(1)
        self.generate(self.nodes[0], 30)
        new_quorums_list = node.quorum('list')['llmq_test']

        self.log.info(f"Test Quorums at height={node.getblockcount()} : {new_quorums_list}")
        assert new_quorums_list != quorums

    def check_sigs(self, hasrecsigs, isconflicting1, isconflicting2):
        has_sig = False
        conflicting_1 = False
        conflicting_2 = False

        for mn in self.mninfo: # type: MasternodeInfo
            if mn.get_node(self).quorum("hasrecsig", q_type, id, msgHash):
                has_sig = True
            if mn.get_node(self).quorum("isconflicting", q_type, id, msgHash):
                conflicting_1 = True
            if mn.get_node(self).quorum("isconflicting", q_type, id, msgHashConflict):
                conflicting_2 = True
        if has_sig != hasrecsigs:
            return False
        if conflicting_1 != isconflicting1:
            return False
        if conflicting_2 != isconflicting2:
            return False

        return True

    def wait_for_sigs(self, hasrecsigs, isconflicting1, isconflicting2, timeout):
        self.wait_until(lambda: self.check_sigs(hasrecsigs, isconflicting1, isconflicting2), timeout = timeout)

    def assert_sigs_nochange(self, hasrecsigs, isconflicting1, isconflicting2, timeout):
        assert not wait_until_helper(lambda: not self.check_sigs(hasrecsigs, isconflicting1, isconflicting2), timeout = timeout, do_assert = False)


    def log_connections(self):
        connections = []
        for idx in range(len(self.nodes)):
            connections.append(self.nodes[idx].getconnectioncount())
        self.log.info(f"nodes connection count: {connections}")

    def run_test(self):
        self.nodes[0].sporkupdate("SPORK_17_QUORUM_DKG_ENABLED", 0)
        self.wait_for_sporks_same()

        self.dynamically_add_masternode(evo=False)
        self.dynamically_add_masternode(evo=True)
        self.connect_nodes(1, 2)

        self.mine_single_node_quorum()

        self.log_connections()
        assert_greater_than(len(self.nodes[0].quorum('list')['llmq_test']), 0)
        self.log.info("We have quorum waiting for ChainLock")
        self.wait_for_chainlocked_block(self.nodes[0], self.nodes[0].getbestblockhash())

        self.log.info("Send funds and wait InstantSend lock")
        txid = self.nodes[0].sendtoaddress(self.nodes[0].getnewaddress(), 1)
        self.wait_for_instantlock(txid, self.nodes[0])

        self.log.info("Test various options to sign messages with nodes")
        recsig_time = self.mocktime

        # Initial state
        self.wait_for_sigs(False, False, False, 1)

        # Sign first share without any optional parameter, should not result in recovered sig
        # Sign second share and test optional quorumHash parameter, should not result in recovered sig
        # 1. Providing an invalid quorum hash should fail and cause no changes for sigs
        assert not self.mninfo[1].get_node(self).quorum("sign", q_type, id, msgHash, msgHash)
        self.assert_sigs_nochange(False, False, False, 3)
        # 2. Providing a valid quorum hash should succeed and cause no changes for sigss
        quorumHash = self.mninfo[1].get_node(self).quorum("selectquorum", q_type, id)["quorumHash"]

        qnode = self.mninfo[0].get_node(self)
        try:
            qnode.quorum("sign", q_type, id, msgHash, quorumHash, False)
        except JSONRPCException as e:
            if e.error['code'] == -8: # failed to create sigShare
                qnode = self.mninfo[1].get_node(self)
                qnode.quorum("sign", q_type, id, msgHash, quorumHash, False)
            else:
                raise e

        self.mninfo[0].get_node(self).quorum("sign", q_type, id, msgHash)
        self.mninfo[1].get_node(self).quorum("sign", q_type, id, msgHash)

        self.wait_for_sigs(True, False, True, 15)
        has0 = self.nodes[0].quorum("hasrecsig", q_type, id, msgHash)
        has1 = self.nodes[1].quorum("hasrecsig", q_type, id, msgHash)
        has2 = self.nodes[2].quorum("hasrecsig", q_type, id, msgHash)
        assert (has0 or has1 or has2)

        self.log.info("Test `quorum verify` rpc")
        node = self.mninfo[0].get_node(self)
        recsig = qnode.quorum("getrecsig", q_type, id, msgHash)
        self.log.info("Find quorum automatically")
        height = node.getblockcount()
        height_bad = node.getblockheader(recsig["quorumHash"])["height"]
        hash_bad = node.getblockhash(0)
        assert node.quorum("verify", q_type, id, msgHash, recsig["sig"])
        assert node.quorum("verify", q_type, id, msgHash, recsig["sig"], "", height)
        assert not node.quorum("verify", q_type, id, msgHashConflict, recsig["sig"])
        assert not node.quorum("verify", q_type, id, msgHash, recsig["sig"], "", height_bad)
        self.log.info("Use specific quorum")
        assert node.quorum("verify", q_type, id, msgHash, recsig["sig"], recsig["quorumHash"])
        assert not node.quorum("verify", q_type, id, msgHashConflict, recsig["sig"], recsig["quorumHash"])
        assert_raises_rpc_error(-8, "quorum not found", node.quorum, "verify", q_type, id, msgHash, recsig["sig"], hash_bad)

        self.log.info("Mine one more quorum, so that we have 2 active ones, nothing should change")
        self.mine_single_node_quorum()
        self.assert_sigs_nochange(True, False, True, 3)


        self.log.info("Mine 2 more quorums, so that the one used for the the recovered sig should become inactive, nothing should change")
        self.mine_single_node_quorum()
        self.mine_single_node_quorum()
        self.assert_sigs_nochange(True, False, True, 3)

        self.log_connections()
        self.log.info("Fast forward until 0.5 days before cleanup is expected, recovered sig should still be valid")
        self.bump_mocktime(recsig_time + int(60 * 60 * 24 * 6.5) - self.mocktime, update_schedulers=False)
        self.log.info("Cleanup starts every 5 seconds")
        self.wait_for_sigs(True, False, True, 15)
        self.log.info("Fast forward 1 day, recovered sig should not be valid anymore")
        self.bump_mocktime(int(60 * 60 * 24 * 1), update_schedulers=False)
        self.log.info("Cleanup starts every 5 seconds")
        self.wait_for_sigs(False, False, False, 15)

        self.log_connections()
        self.log.info("Test chainlocks and InstantSend with new quorums")
        block_hash = self.nodes[0].getbestblockhash()
        self.log.info(f"Chainlock on block: {block_hash} is expecting")
        self.wait_for_best_chainlock(self.nodes[0], block_hash)
        txid = self.nodes[0].sendtoaddress(self.nodes[0].getnewaddress(), 1)
        self.log.info(f"InstantSend lock on tx: {txid} is expecting")
        self.wait_for_instantlock(txid, self.nodes[0])


if __name__ == '__main__':
    LLMQSigningTest().main()
