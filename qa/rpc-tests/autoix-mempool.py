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
        return info['bip9_softforks']['autoix']['status']

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
                sync_masternodes(self.nodes)

        while self.get_autoix_bip9_status() == 'started':
            set_mocktime(get_mocktime() + 1)
            set_node_times(self.nodes, get_mocktime())
            self.nodes[0].generate(1)
            counter += 1
            if counter % sync_period == 0:
                # sync nodes
                self.sync_all()
                sync_masternodes(self.nodes)

        while self.get_autoix_bip9_status() == 'locked_in':
            set_mocktime(get_mocktime() + 1)
            set_node_times(self.nodes, get_mocktime())
            self.nodes[0].generate(1)
            counter += 1
            if counter % sync_period == 0:
                # sync nodes
                self.sync_all()
                sync_masternodes(self.nodes)

        # sync nodes
        self.sync_all()
        sync_masternodes(self.nodes)

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

    def get_mempool_size(self, node):
        info = node.getmempoolinfo()
        return info['usage']

    def fill_mempool(self):
        node = self.nodes[0]
        rec_address = node.getnewaddress()
        while self.get_mempool_size(node) < MAX_MEMPOOL_SIZE * MB_SIZE * AUTO_IX_MEM_THRESHOLD + 10000:
            node.sendtoaddress(rec_address, 1.0)
            sleep(0.1)
        self.sync_all()

    def run_test(self):
        self.enforce_masternode_payments()  # required for bip9 activation
        self.activate_autoix_bip9()
        self.set_autoix_spork_state(True)

        # check pre-conditions for autoIX
        assert(self.get_autoix_bip9_status() == 'active')
        assert(self.get_autoix_spork_state())

        # autoIX is working
        assert(self.send_simple_tx(self.nodes[0], self.nodes[self.receiver_idx]))

        # send funds for InstantSend  after filling mempool and give them 6 confirmations
        rec_address = self.nodes[self.receiver_idx].getnewaddress()
        self.nodes[0].sendtoaddress(rec_address, 500.0)
        self.nodes[0].sendtoaddress(rec_address, 500.0)
        self.sync_all()
        for i in range(0, 2):
            self.nodes[self.receiver_idx].generate(1)
        self.sync_all()

        # fill mempool with transactions
        self.fill_mempool()

        # autoIX is not working now
        assert(not self.send_simple_tx(self.nodes[self.receiver_idx], self.nodes[0]))
        # regular IX is still working
        assert(self.send_regular_IX(self.nodes[self.receiver_idx], self.nodes[0]))


if __name__ == '__main__':
    AutoIXMempoolTest().main()
