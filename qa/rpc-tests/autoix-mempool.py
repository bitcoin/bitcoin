#!/usr/bin/env python3
# Copyright (c) 2018 The Dash Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.mininode import *
from test_framework.test_framework import DashTestFramework
from test_framework.util import *
from time import *

'''
autoix-mempool.py

Checks if automatic InstantSend locks stop working when transaction mempool 
is full (more than 0.1 part from max value). 

'''

MAX_MEMPOOL_SIZE = 5 # max node mempool in MBs
MB_SIZE = 1000000 # C++ code use this coefficient to calc MB in mempool
AUTO_IX_MEM_THRESHOLD = 0.1


class AutoIXMempoolTest(DashTestFramework):
    def __init__(self):
        super().__init__(13, 10, ["-maxmempool=%d" % MAX_MEMPOOL_SIZE])
        # set sender,  receiver
        self.receiver_idx = self.num_nodes - 2
        self.sender_idx = self.num_nodes - 3

    def get_autoix_bip9_status(self):
        info = self.nodes[0].getblockchaininfo()
        # we reuse the dip3 deployment
        return info['bip9_softforks']['dip0003']['status']

    def activate_autoix_bip9(self):
        # sync nodes periodically
        # if we sync them too often, activation takes too many time
        # if we sync them too rarely, nodes failed to update its state and
        # bip9 status is not updated
        # so, in this code nodes are synced once per 20 blocks
        counter = 0
        sync_period = 10

        while self.get_autoix_bip9_status() == 'defined':
            set_mocktime(get_mocktime() + 1)
            set_node_times(self.nodes, get_mocktime())
            self.nodes[0].generate(1)
            counter += 1
            if counter % sync_period == 0:
                # sync nodes
                self.sync_all()

        while self.get_autoix_bip9_status() == 'started':
            set_mocktime(get_mocktime() + 1)
            set_node_times(self.nodes, get_mocktime())
            self.nodes[0].generate(1)
            counter += 1
            if counter % sync_period == 0:
                # sync nodes
                self.sync_all()

        while self.get_autoix_bip9_status() == 'locked_in':
            set_mocktime(get_mocktime() + 1)
            set_node_times(self.nodes, get_mocktime())
            self.nodes[0].generate(1)
            counter += 1
            if counter % sync_period == 0:
                # sync nodes
                self.sync_all()

        # sync nodes
        self.sync_all()

        assert(self.get_autoix_bip9_status() == 'active')

    def get_autoix_spork_state(self):
        info = self.nodes[0].spork('active')
        return info['SPORK_16_INSTANTSEND_AUTOLOCKS']

    def set_autoix_spork_state(self, state):
        if state:
            value = 0
        else:
            value = 4070908800
        self.nodes[0].spork('SPORK_16_INSTANTSEND_AUTOLOCKS', value)

    # sends regular IX with high fee and may inputs (not-simple transaction)
    def send_regular_IX(self, sender, receiver):
        receiver_addr = receiver.getnewaddress()
        txid = sender.instantsendtoaddress(receiver_addr, 1.0)
        return self.wait_for_instantlock(txid, sender)

    # sends simple trx, it should become IX if autolocks are allowed
    def send_simple_tx(self, sender, receiver):
        raw_tx = self.create_raw_trx(sender, receiver, 1.0, 1, 4)
        txid = self.nodes[0].sendrawtransaction(raw_tx['hex'])
        self.sync_all()
        return self.wait_for_instantlock(txid, sender)

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
        self.enforce_masternode_payments()  # required for bip9 activation
        self.activate_autoix_bip9()
        self.set_autoix_spork_state(True)

        # check pre-conditions for autoIX
        assert(self.get_autoix_bip9_status() == 'active')
        assert(self.get_autoix_spork_state())

        # create 3 inputs for txes on sender node and give them 6 confirmations
        sender = self.nodes[self.sender_idx]
        receiver = self.nodes[self.receiver_idx]
        sender_address = sender.getnewaddress()
        for i in range(0, 3):
            self.nodes[0].sendtoaddress(sender_address, 2.0)
        for i in range(0, 6):
            set_mocktime(get_mocktime() + 1)
            set_node_times(self.nodes, get_mocktime())
            self.nodes[0].generate(1)
        self.sync_all()

        # autoIX is working
        assert(self.send_simple_tx(sender, receiver))

        # fill mempool with transactions
        self.fill_mempool()

        # autoIX is not working now
        assert(not self.send_simple_tx(sender, receiver))
        # regular IX is still working
        assert(self.send_regular_IX(sender, receiver))


if __name__ == '__main__':
    AutoIXMempoolTest().main()
