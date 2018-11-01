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
        super().__init__(14, 10, [])
        # set sender,  receiver,  isolated nodes
        self.isolated_idx = self.num_nodes - 1
        self.receiver_idx = self.num_nodes - 2
        self.sender_idx = self.num_nodes - 3

    def run_test(self):
        # feed the sender with some balance
        sender_addr = self.nodes[self.sender_idx].getnewaddress()
        self.nodes[0].sendtoaddress(sender_addr, 1)
        # make sender funds mature for InstantSend
        for i in range(0, 2):
            set_mocktime(get_mocktime() + 1)
            set_node_times(self.nodes, get_mocktime())
            self.nodes[0].generate(1)
        # create doublepending transaction,  but don't relay it
        dblspnd_tx = self.create_raw_trx(self.nodes[self.sender_idx],
                                       self.nodes[self.isolated_idx],
                                       0.5, 1, 100)
        # stop one node to isolate it from network
        stop_node(self.nodes[self.isolated_idx], self.isolated_idx)
        # instantsend to receiver
        receiver_addr = self.nodes[self.receiver_idx].getnewaddress()
        is_id = self.nodes[self.sender_idx].instantsendtoaddress(receiver_addr, 0.9)
        # wait for instantsend locks
        start = time()
        locked = False
        while True:
            is_trx = self.nodes[self.sender_idx].gettransaction(is_id)
            if is_trx['instantlock']:
                locked = True
                break
            if time() > start + 10:
                break
            sleep(0.1)
        assert(locked)
        # start last node
        self.nodes[self.isolated_idx] = start_node(self.isolated_idx,
                                                   self.options.tmpdir,
                                                   ["-debug"])
        # send doublespend transaction to isolated node
        self.nodes[self.isolated_idx].sendrawtransaction(dblspnd_tx['hex'])
        # generate block on isolated node with doublespend transaction
        set_mocktime(get_mocktime() + 1)
        set_node_times(self.nodes, get_mocktime())
        self.nodes[self.isolated_idx].generate(1)
        wrong_block = self.nodes[self.isolated_idx].getbestblockhash()
        # connect isolated block to network
        for i in range(0, self.isolated_idx):
            connect_nodes(self.nodes[i], self.isolated_idx)
        # check doublespend block is rejected by other nodes
        timeout = 10
        for i in range(0, self.isolated_idx):
            res = self.nodes[i].waitforblock(wrong_block, timeout)
            assert (res['hash'] != wrong_block)
            # wait for long time only for first node
            timeout = 1


if __name__ == '__main__':
    InstantSendTest().main()
