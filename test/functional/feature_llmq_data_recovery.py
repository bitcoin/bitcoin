#!/usr/bin/env python3
# Copyright (c) 2021-2024 The Dash Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

import time
from typing import List, Optional
from test_framework.p2p import logger
from test_framework.test_framework import (
    DashTestFramework,
    MasternodeInfo,
)
from test_framework.util import force_finish_mnsync

'''
feature_llmq_data_recovery.py

Tests automated recovery of DKG data and the related command line parameters:
 -llmq-data-recovery
 -llmq-qvvec-sync
'''

# LLMQ types available in regtest
llmq_test = 100
llmq_test_v17 = 102
llmq_type_strings = {llmq_test: 'llmq_test', llmq_test_v17: 'llmq_test_v17'}


class QuorumDataRecoveryTest(DashTestFramework):
    def set_test_params(self):
        extra_args = [["-vbparams=testdummy:0:999999999999:0:10:8:6:5:-1"] for _ in range(7)]
        self.set_dash_test_params(7, 6, extra_args=extra_args)
        self.set_dash_llmq_test_params(4, 3)

    def restart_mn(self, mn: MasternodeInfo, reindex=False, qvvec_sync=None, qdata_recovery_enabled=True):
        args = self.extra_args[mn.nodeIdx] + ['-masternodeblsprivkey=%s' % mn.keyOperator,
                                              '-llmq-data-recovery=%d' % qdata_recovery_enabled]
        if qvvec_sync is None:
            qvvec_sync = []
        for llmq_sync in qvvec_sync:
            args.append('-llmq-qvvec-sync=%s:%d' % (llmq_type_strings[llmq_sync[0]], llmq_sync[1]))
        if reindex:
            args.append('-reindex')
            self.restart_node(mn.nodeIdx, args)
        else:
            self.restart_node(mn.nodeIdx, args)

    def wait_restarted_mn(self, mn: MasternodeInfo, reindex=False, block_count=None):
        if reindex:
            self.wait_until(lambda: mn.get_node(self).getblockcount() >= block_count)

        force_finish_mnsync(mn.get_node(self))
        self.connect_nodes(mn.nodeIdx, 0)

    def restart_mns(self, mns=None, exclude=None, reindex=False, qvvec_sync=None, qdata_recovery_enabled=True):
        if exclude is None:
            exclude = []
        if qvvec_sync is None:
            qvvec_sync = []

        block_count = self.nodes[0].getblockcount()
        for mn in self.mninfo if mns is None else mns: # type: MasternodeInfo
            if mn not in exclude:
                self.restart_mn(mn, reindex, qvvec_sync, qdata_recovery_enabled)

        for mn in self.mninfo if mns is None else mns: # type: MasternodeInfo
            if mn not in exclude:
                self.wait_restarted_mn(mn, reindex, block_count)

        if qdata_recovery_enabled:
            # trigger recovery threads and wait for them to start
            self.generate(self.nodes[0], 1, sync_fun=self.no_op)

            self.bump_mocktime(self.quorum_data_thread_request_timeout_seconds + 1)
            time.sleep(1)

        self.sync_blocks()
        self.wait_for_sporks_same()

    def test_mns(self, quorum_type_in, quorum_hash_in, valid_mns=None, all_mns=None, test_secret=True, expect_secret=True,
                 recover=False, timeout=120):
        if valid_mns is None:
            valid_mns = []
        if all_mns is None:
            all_mns = []
        for mn in all_mns:
            if mn not in valid_mns:
                assert not self.test_mn_quorum_data(mn, quorum_type_in, quorum_hash_in, test_secret, False)
        self.wait_for_quorum_data(valid_mns, quorum_type_in, quorum_hash_in, test_secret, expect_secret, recover, timeout)

    def get_mn(self, protx_hash) -> Optional[MasternodeInfo]:
        for mn in self.mninfo: # type: MasternodeInfo
            if mn.proTxHash == protx_hash:
                return mn
        return None

    def get_member_mns(self, quorum_type, quorum_hash) -> List[Optional[MasternodeInfo]]:
        members = self.nodes[0].quorum("info", quorum_type, quorum_hash)["members"]
        mns = []
        for member in members:
            if member["valid"]:
                mns.append(self.get_mn(member["proTxHash"]))
        return mns

    def get_subset_only_in_left(self, quorum_members_left, quorum_members_right) -> List[MasternodeInfo]:
        quorum_members_subset = quorum_members_left.copy()
        for mn in list(set(quorum_members_left) & set(quorum_members_right)):
            quorum_members_subset.remove(mn)
        return quorum_members_subset

    def test_llmq_qvvec_sync(self, llmq_sync_entries):
        self.log.info("Test with %d -llmq-qvvec-sync option(s)" % len(llmq_sync_entries))
        for llmq_sync in llmq_sync_entries:
            llmq_type = llmq_sync[0]
            llmq_sync_mode = llmq_sync[1]
            self.log.info("Validate -llmq-qvvec-sync=%s:%d" % (llmq_type_strings[llmq_type], llmq_sync_mode))
            # First restart with recovery thread triggering disabled
            self.restart_mns(qdata_recovery_enabled=False)
            # If mode=0 i.e. "Sync always" all nodes should request the qvvec from new quorums
            if llmq_sync_mode == 0:
                quorum_hash = self.mine_quorum()
                member_mns = self.get_member_mns(llmq_type, quorum_hash)
                # So far the only the quorum members of the quorum should have the quorum verification vector
                self.test_mns(llmq_type, quorum_hash, valid_mns=member_mns, all_mns=self.mninfo,
                              test_secret=False, recover=False)
                # Now restart with recovery enabled
                self.restart_mns(qvvec_sync=llmq_sync_entries)
                # All other nodes should now request the qvvec from the quorum
                self.test_mns(llmq_type, quorum_hash, valid_mns=self.mninfo, test_secret=False, recover=True)
            # If mode=1 i.e. "Sync only if type member" not all nodes should request the qvvec from quorum 1 and 2
            elif llmq_sync_mode == 1:
                # Create quorum_1 and a quorum_2 so that we have subsets (members_only_in_1, members_only_in_2) where
                # each only contains nodes that are members of quorum_1 but not quorum_2 and vice versa
                quorum_hash_1 = None
                quorum_hash_2 = None
                members_only_in_1 = []
                members_only_in_2 = []
                member_mns_2 = []
                while len(members_only_in_1) == 0 or len(members_only_in_2) == 0:
                    quorum_hash_1 = quorum_hash_2
                    quorum_hash_2 = self.mine_quorum()
                    member_mns_1 = member_mns_2
                    member_mns_2 = self.get_member_mns(llmq_type, quorum_hash_2)
                    members_only_in_1 = self.get_subset_only_in_left(member_mns_1, member_mns_2)
                    members_only_in_2 = self.get_subset_only_in_left(member_mns_2, member_mns_1)
                # So far the nodes of quorum_1 shouldn't have the quorum verification vector of quorum_2 and vice versa
                self.test_mns(llmq_type, quorum_hash_2, all_mns=members_only_in_1, expect_secret=False)
                self.test_mns(llmq_type, quorum_hash_1, all_mns=members_only_in_2, expect_secret=False)
                # Now restart with recovery enabled
                self.restart_mns(qvvec_sync=llmq_sync_entries)
                # Members which are only in quorum 2 should request the qvvec from quorum 1 from the members of quorum 1
                self.test_mns(llmq_type, quorum_hash_1, valid_mns=members_only_in_2, expect_secret=False, recover=True)
                # Members which are only in quorum 1 should request the qvvec from quorum 2 from the members of quorum 2
                self.test_mns(llmq_type, quorum_hash_2, valid_mns=members_only_in_1, expect_secret=False, recover=True)


    def run_test(self):

        node = self.nodes[0]
        node.sporkupdate("SPORK_17_QUORUM_DKG_ENABLED", 0)
        node.sporkupdate("SPORK_21_QUORUM_ALL_CONNECTED", 0)
        self.wait_for_sporks_same()

        logger.info("Test automated DGK data recovery")
        # This two nodes will remain the only ones with valid DKG data
        last_resort_test: MasternodeInfo = None
        last_resort_v17: MasternodeInfo = None
        while True:
            # Mine the quorums used for the recovery test
            quorum_hash_recover = self.mine_quorum()
            # Get all their member masternodes
            member_mns_recover_test = self.get_member_mns(llmq_test, quorum_hash_recover)
            member_mns_recover_v17 = self.get_member_mns(llmq_test_v17, quorum_hash_recover)
            # All members should initially be valid
            self.test_mns(llmq_test, quorum_hash_recover, valid_mns=member_mns_recover_test)
            self.test_mns(llmq_test_v17, quorum_hash_recover, valid_mns=member_mns_recover_v17)
            try:
                # As last resorts find one node which is in llmq_test but not in llmq_test_v17 and one other vice versa
                last_resort_test = self.get_subset_only_in_left(member_mns_recover_test, member_mns_recover_v17)[0]
                last_resort_v17 = self.get_subset_only_in_left(member_mns_recover_v17, member_mns_recover_test)[0]
                break
            except IndexError:
                continue
        assert last_resort_test != last_resort_v17
        # Reindex all other nodes the to drop their DKG data, first run with recovery disabled to make sure disabling
        # works as expected
        recover_members = member_mns_recover_test + member_mns_recover_v17
        exclude_members = [last_resort_test, last_resort_v17]
        # Reindex all masternodes but exclude the last_resort for both testing quorums
        self.restart_mns(exclude=exclude_members, reindex=True, qdata_recovery_enabled=False)
        # Validate all but one are invalid members now
        self.test_mns(llmq_test, quorum_hash_recover, valid_mns=[last_resort_test], all_mns=member_mns_recover_test)
        self.test_mns(llmq_test_v17, quorum_hash_recover, valid_mns=[last_resort_v17], all_mns=member_mns_recover_v17)
        # If recovery would be enabled it would trigger after the mocktime bump / mined block
        self.bump_mocktime(self.quorum_data_request_expiration_timeout + 1)
        self.generate(node, 1, sync_fun=self.no_op)
        time.sleep(10)
        # Make sure they are still invalid
        self.test_mns(llmq_test, quorum_hash_recover, valid_mns=[last_resort_test], all_mns=member_mns_recover_test)
        self.test_mns(llmq_test_v17, quorum_hash_recover, valid_mns=[last_resort_v17], all_mns=member_mns_recover_v17)
        # Mining a block should not result in a chainlock now because the responsible quorum shouldn't have enough
        # valid members.
        self.wait_for_chainlocked_block(node, self.generate(node, 1, sync_fun=self.no_op)[0], False, 5)
        # Now restart with recovery enabled
        self.restart_mns(mns=recover_members, exclude=exclude_members, reindex=True, qdata_recovery_enabled=True)
        # Validate that all invalid members recover. Note: recover=True leads to mocktime bumps and mining while waiting
        # which trigger CQuorumManger::TriggerQuorumDataRecoveryThreads()
        self.test_mns(llmq_test, quorum_hash_recover, valid_mns=member_mns_recover_test, recover=True)
        self.test_mns(llmq_test_v17, quorum_hash_recover, valid_mns=member_mns_recover_v17, recover=True)
        # Mining a block should result in a chainlock now because the quorum should be healed
        self.wait_for_chainlocked_block_all_nodes(node.getbestblockhash())
        logger.info("Test -llmq-qvvec-sync command line parameter")
        # Run with one type separated and then both possible (for regtest) together, both calls generate new quorums
        # and are restarting the nodes with the other parameters
        self.test_llmq_qvvec_sync([(llmq_test, 0)])
        self.test_llmq_qvvec_sync([(llmq_test_v17, 1)])
        self.test_llmq_qvvec_sync([(llmq_test, 0), (llmq_test_v17, 1)])
        logger.info("Test invalid command line parameter values")
        node.stop_node()
        node.wait_until_stopped()
        # Test -llmq-qvvec-sync entry format
        node.assert_start_raises_init_error(["-llmq-qvvec-sync="],
                                            "Error: Invalid format in -llmq-qvvec-sync:")
        node.assert_start_raises_init_error(["-llmq-qvvec-sync=0"],
                                            "Error: Invalid format in -llmq-qvvec-sync: 0")
        node.assert_start_raises_init_error(["-llmq-qvvec-sync=0:"],
                                            "Error: Invalid format in -llmq-qvvec-sync: 0:")
        node.assert_start_raises_init_error(["-llmq-qvvec-sync=:0"],
                                            "Error: Invalid format in -llmq-qvvec-sync: :0")
        node.assert_start_raises_init_error(["-llmq-qvvec-sync=0:0:0"],
                                            "Error: Invalid format in -llmq-qvvec-sync: 0:0:0")
        node.assert_start_raises_init_error(["-llmq-qvvec-sync=0::"],
                                            "Error: Invalid format in -llmq-qvvec-sync: 0::")
        node.assert_start_raises_init_error(["-llmq-qvvec-sync=::0"],
                                            "Error: Invalid format in -llmq-qvvec-sync: ::0")
        node.assert_start_raises_init_error(["-llmq-qvvec-sync=:0:"],
                                            "Error: Invalid format in -llmq-qvvec-sync: :0:")
        # Test llmqType
        node.assert_start_raises_init_error(["-llmq-qvvec-sync=0:0"],
                                            "Error: Invalid llmqType in -llmq-qvvec-sync: 0:0")
        node.assert_start_raises_init_error(["-llmq-qvvec-sync=llmq-test:0"],
                                            "Error: Invalid llmqType in -llmq-qvvec-sync: llmq-test:0")
        node.assert_start_raises_init_error(["-llmq-qvvec-sync=100:0", "-llmq-qvvec-sync=0"],
                                            "Error: Invalid llmqType in -llmq-qvvec-sync: 100:0")
        node.assert_start_raises_init_error(["-llmq-qvvec-sync=llmq_test:0", "-llmq-qvvec-sync=llmq_test:0"],
                                            "Error: Duplicated llmqType in -llmq-qvvec-sync: llmq_test:0")
        # Test mode
        node.assert_start_raises_init_error(["-llmq-qvvec-sync=llmq_test:-1"],
                                            "Error: Invalid mode in -llmq-qvvec-sync: llmq_test:-1")
        node.assert_start_raises_init_error(["-llmq-qvvec-sync=llmq_test:2"],
                                            "Error: Invalid mode in -llmq-qvvec-sync: llmq_test:2")


if __name__ == '__main__':
    QuorumDataRecoveryTest().main()
