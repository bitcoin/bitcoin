#!/usr/bin/env python3
# Copyright (c) 2018 The Dash Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.mininode import *
from test_framework.test_framework import DashTestFramework
from test_framework.util import *
from time import *

'''
autois-mempool.py

Checks if automatic InstantSend locks stop working when transaction mempool 
is full (more than 0.1 part from max value). 

'''

MAX_MEMPOOL_SIZE = 1 # max node mempool in MBs
MB_SIZE = 1000000 # C++ code use this coefficient to calc MB in mempool
AUTO_IX_MEM_THRESHOLD = 0.1


class AutoISMempoolTest(DashTestFramework):
    def __init__(self):
        super().__init__(8, 5, ["-maxmempool=%d" % MAX_MEMPOOL_SIZE, '-limitdescendantsize=10'], fast_dip3_enforcement=True)
        # set sender,  receiver
        self.receiver_idx = 1
        self.sender_idx = 2

    def get_mempool_usage(self, node):
        info = node.getmempoolinfo()
        return info['usage']

    def fill_mempool(self):
        # send lots of txes to yourself just to fill the mempool
        counter = 0
        sync_period = 10
        dummy_address = self.nodes[0].getnewaddress()
        while self.get_mempool_usage(self.nodes[self.sender_idx]) < MAX_MEMPOOL_SIZE * MB_SIZE * AUTO_IX_MEM_THRESHOLD:
            self.nodes[0].sendtoaddress(dummy_address, 1.0)
            counter += 1
            if counter % sync_period == 0:
                # sync nodes
                self.sync_all()
        self.sync_all()

    def run_test(self):
        # make sure masternodes are synced
        sync_masternodes(self.nodes)

        self.nodes[0].spork("SPORK_17_QUORUM_DKG_ENABLED", 0)
        self.wait_for_sporks_same()
        self.mine_quorum()

        self.log.info("Test old InstantSend")
        self.test_auto();

        # Generate 6 block to avoid retroactive signing overloading Travis
        self.nodes[0].generate(6)
        sync_blocks(self.nodes)

        self.nodes[0].spork("SPORK_20_INSTANTSEND_LLMQ_BASED", 0)
        self.wait_for_sporks_same()

        self.log.info("Test new InstantSend")
        self.test_auto(True);

    def test_auto(self, new_is = False):
        self.activate_autois_bip9(self.nodes[0])
        self.set_autois_spork_state(self.nodes[0], True)

        # check pre-conditions for autoIS
        assert(self.get_autois_bip9_status(self.nodes[0]) == 'active')
        assert(self.get_autois_spork_state(self.nodes[0]))

        # create 3 inputs for txes on sender node and give them enough confirmations
        sender = self.nodes[self.sender_idx]
        receiver = self.nodes[self.receiver_idx]
        sender_address = sender.getnewaddress()
        for i in range(0, 4):
            self.nodes[0].sendtoaddress(sender_address, 2.0)
        for i in range(0, 2):
            set_mocktime(get_mocktime() + 1)
            set_node_times(self.nodes, get_mocktime())
            self.nodes[0].generate(1)
        self.sync_all()

        # autoIS is working
        assert(self.send_simple_tx(sender, receiver))

        # fill mempool with transactions
        self.set_autois_spork_state(self.nodes[0], False)
        self.nodes[0].spork("SPORK_2_INSTANTSEND_ENABLED", 4070908800)
        self.wait_for_sporks_same()
        self.fill_mempool()
        self.set_autois_spork_state(self.nodes[0], True)
        self.nodes[0].spork("SPORK_2_INSTANTSEND_ENABLED", 0)
        self.wait_for_sporks_same()

        # autoIS is not working now
        if not new_is:
            assert(not self.send_simple_tx(sender, receiver))
            # regular IS is still working for old IS but not for new one
            assert(not self.send_regular_instantsend(sender, receiver, False) if new_is else self.send_regular_instantsend(sender, receiver))

        # generate one block to clean up mempool and retry auto and regular IS
        # generate 5 more blocks to avoid retroactive signing (which would overload Travis)
        set_mocktime(get_mocktime() + 1)
        set_node_times(self.nodes, get_mocktime())
        self.nodes[0].spork("SPORK_2_INSTANTSEND_ENABLED", 4070908800)
        self.wait_for_sporks_same()
        self.nodes[0].generate(6)
        self.sync_all()
        set_mocktime(get_mocktime() + 1)
        set_node_times(self.nodes, get_mocktime())
        self.nodes[0].spork("SPORK_2_INSTANTSEND_ENABLED", 0)
        self.wait_for_sporks_same()
        assert(self.send_simple_tx(sender, receiver))
        assert(self.send_regular_instantsend(sender, receiver, not new_is))


if __name__ == '__main__':
    AutoISMempoolTest().main()
