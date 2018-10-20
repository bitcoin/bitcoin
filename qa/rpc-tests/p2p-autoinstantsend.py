#!/usr/bin/env python3
# Copyright (c) 2018 The Dash Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.mininode import *
from test_framework.test_framework import DashTestFramework
from test_framework.util import *
from time import *

'''
p2p-autoinstantsend.py

Test automatic InstantSend locks functionality.

Checks that simple transactions automatically become InstantSend locked, 
complex transactions don't become IS-locked and this functionality is
activated only if it is BIP9-activated and SPORK_16_INSTANTSEND_AUTOLOCKS is 
active.

Also checks that this functionality doesn't influence regular InstantSend
transactions with high fee. 
'''

class AutoInstantSendTest(DashTestFramework):
    def __init__(self):
        super().__init__(14, 10, [])
        # set sender,  receiver,  isolated nodes
        self.isolated_idx = self.num_nodes - 1
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
        set_mocktime(get_mocktime() + 1)
        set_node_times(self.nodes, get_mocktime())
        if state:
            value = 0
        else:
            value = 4070908800
        self.nodes[0].spork('SPORK_16_INSTANTSEND_AUTOLOCKS', value)

    # sends regular IX with high fee and may inputs (not-simple transaction)
    def send_regular_IX(self):
        receiver_addr = self.nodes[self.receiver_idx].getnewaddress()
        txid = self.nodes[0].instantsendtoaddress(receiver_addr, 1.0)
        MIN_FEE = satoshi_round(-0.0001)
        fee = self.nodes[0].gettransaction(txid)['fee']
        expected_fee = MIN_FEE * len(self.nodes[0].getrawtransaction(txid, True)['vin'])
        assert_equal(fee, expected_fee)
        return self.wait_for_instantlock(txid, self.nodes[0])

    # sends simple trx, it should become IX if autolocks are allowed
    def send_simple_tx(self):
        raw_tx = self.create_raw_trx(self.nodes[0], self.nodes[self.receiver_idx], 1.0, 1, 4)
        txid = self.nodes[0].sendrawtransaction(raw_tx['hex'])
        self.sync_all()
        return self.wait_for_instantlock(txid, self.nodes[0])

    # sends complex trx, it should never become IX
    def send_complex_tx(self):
        raw_tx = self.create_raw_trx(self.nodes[0], self.nodes[self.receiver_idx], 1.0, 5, 100)
        txid = self.nodes[0].sendrawtransaction(raw_tx['hex'])
        self.sync_all()
        return self.wait_for_instantlock(txid, self.nodes[0])

    def run_test(self):
        # feed the sender with some balance
        sender_addr = self.nodes[self.sender_idx].getnewaddress()
        self.nodes[0].sendtoaddress(sender_addr, 1)
        # make sender funds mature for InstantSend
        for i in range(0, 2):
            set_mocktime(get_mocktime() + 1)
            set_node_times(self.nodes, get_mocktime())
            self.nodes[0].generate(1)
            
        self.enforce_masternode_payments()  # required for bip9 activation
        assert(self.get_autoix_bip9_status() == 'defined')
        assert(not self.get_autoix_spork_state())

        assert(self.send_regular_IX())
        assert(not self.send_simple_tx())
        assert(not self.send_complex_tx())

        self.activate_autoix_bip9()
        self.set_autoix_spork_state(True)

        assert(self.get_autoix_bip9_status() == 'active')
        assert(self.get_autoix_spork_state())

        assert(self.send_regular_IX())
        assert(self.send_simple_tx())
        assert(not self.send_complex_tx())

        self.set_autoix_spork_state(False)
        assert(not self.get_autoix_spork_state())

        assert(self.send_regular_IX())
        assert(not self.send_simple_tx())
        assert(not self.send_complex_tx())


if __name__ == '__main__':
    AutoInstantSendTest().main()
