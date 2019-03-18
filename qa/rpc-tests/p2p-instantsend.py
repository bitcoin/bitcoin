#!/usr/bin/env python3
# Copyright (c) 2018 The Dash Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.mininode import *
from test_framework.test_framework import DashTestFramework
from test_framework.util import *
from time import *

'''
InstantSendTest -- test InstantSend functionality (prevent doublespend for unconfirmed transactions)
'''

class InstantSendTest(DashTestFramework):
    def __init__(self):
        super().__init__(9, 5, [], fast_dip3_enforcement=True)
        # set sender,  receiver,  isolated nodes
        self.isolated_idx = 1
        self.receiver_idx = 2
        self.sender_idx = 3

    def run_test(self):
        self.nodes[0].spork("SPORK_17_QUORUM_DKG_ENABLED", 0)
        self.wait_for_sporks_same()
        self.mine_quorum()

        self.log.info("Test old InstantSend")
        self.test_doublespend()

        self.nodes[0].spork("SPORK_20_INSTANTSEND_LLMQ_BASED", 0)
        self.wait_for_sporks_same()

        self.log.info("Test new InstantSend")
        self.test_doublespend()

    def test_doublespend(self, new_is = False):
        sender = self.nodes[self.sender_idx]
        receiver = self.nodes[self.receiver_idx]
        isolated = self.nodes[self.isolated_idx]
        # feed the sender with some balance
        sender_addr = sender.getnewaddress()
        self.nodes[0].sendtoaddress(sender_addr, 1)
        # make sender funds mature for InstantSend
        for i in range(0, 2):
            set_mocktime(get_mocktime() + 1)
            set_node_times(self.nodes, get_mocktime())
            self.nodes[0].generate(1)
        self.sync_all()

        # create doublepending transaction,  but don't relay it
        dblspnd_tx = self.create_raw_tx(sender, isolated, 0.5, 1, 100)
        # stop one node to isolate it from network
        isolated.setnetworkactive(False)
        # instantsend to receiver
        receiver_addr = receiver.getnewaddress()
        is_id = sender.instantsendtoaddress(receiver_addr, 0.9)
        self.wait_for_instantlock(is_id, sender)
        # send doublespend transaction to isolated node
        isolated.sendrawtransaction(dblspnd_tx['hex'])
        # generate block on isolated node with doublespend transaction
        set_mocktime(get_mocktime() + 1)
        set_node_times(self.nodes, get_mocktime())
        isolated.generate(1)
        wrong_block = isolated.getbestblockhash()
        # connect isolated block to network
        isolated.setnetworkactive(True)
        for i in range(0, self.isolated_idx):
            connect_nodes(self.nodes[i], self.isolated_idx)
        # check doublespend block is rejected by other nodes
        timeout = 10
        for i in range(0, self.num_nodes):
            if i == self.isolated_idx:
                continue
            res = self.nodes[i].waitforblock(wrong_block, timeout)
            assert (res['hash'] != wrong_block)
            # wait for long time only for first node
            timeout = 1
        # mine more blocks
        # TODO: mine these blocks on an isolated node
        set_mocktime(get_mocktime() + 1)
        set_node_times(self.nodes, get_mocktime())
        self.nodes[0].generate(2)
        self.sync_all()

if __name__ == '__main__':
    InstantSendTest().main()
