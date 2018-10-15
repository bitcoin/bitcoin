#!/usr/bin/env python3
# Copyright (c) 2018 The Dash Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.mininode import *
from test_framework.test_framework import BitcoinTestFramework
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

MASTERNODE_COLLATERAL = 1000


class MasternodeInfo:
    def __init__(self, key, collateral_id, collateral_out):
        self.key = key
        self.collateral_id = collateral_id
        self.collateral_out = collateral_out


class AutoInstantSendTest(BitcoinTestFramework):
    def __init__(self):
        super().__init__()
        self.mn_count = 10
        self.num_nodes = self.mn_count + 4
        self.mninfo = []
        self.setup_clean_chain = True
        self.is_network_split = False
        # set sender,  receiver,  isolated nodes
        self.isolated_idx = self.num_nodes - 1
        self.receiver_idx = self.num_nodes - 2
        self.sender_idx = self.num_nodes - 3

    def create_simple_node(self):
        idx = len(self.nodes)
        self.nodes.append(start_node(idx, self.options.tmpdir,
                                     ["-debug"]))
        for i in range(0, idx):
            connect_nodes(self.nodes[i], idx)

    def get_mnconf_file(self):
        return os.path.join(self.options.tmpdir, "node0/regtest/masternode.conf")

    def prepare_masternodes(self):
        for idx in range(0, self.mn_count):
            key = self.nodes[0].masternode("genkey")
            address = self.nodes[0].getnewaddress()
            txid = self.nodes[0].sendtoaddress(address, MASTERNODE_COLLATERAL)
            txrow = self.nodes[0].getrawtransaction(txid, True)
            collateral_vout = 0
            for vout_idx in range(0, len(txrow["vout"])):
                vout = txrow["vout"][vout_idx]
                if vout["value"] == MASTERNODE_COLLATERAL:
                    collateral_vout = vout_idx
            self.nodes[0].lockunspent(False,
                                      [{"txid": txid, "vout": collateral_vout}])
            self.mninfo.append(MasternodeInfo(key, txid, collateral_vout))

    def write_mn_config(self):
        conf = self.get_mnconf_file()
        f = open(conf, 'a')
        for idx in range(0, self.mn_count):
            f.write("mn%d 127.0.0.1:%d %s %s %d\n" % (idx + 1, p2p_port(idx + 1),
                                                      self.mninfo[idx].key,
                                                      self.mninfo[idx].collateral_id,
                                                      self.mninfo[idx].collateral_out))
        f.close()

    def create_masternodes(self):
        for idx in range(0, self.mn_count):
            self.nodes.append(start_node(idx + 1, self.options.tmpdir,
                                         ['-debug=masternode', '-externalip=127.0.0.1',
                                          '-masternode=1',
                                          '-masternodeprivkey=%s' % self.mninfo[idx].key
                                          ]))
            for i in range(0, idx + 1):
                connect_nodes(self.nodes[i], idx + 1)

    def sentinel(self):
        for i in range(1, self.mn_count + 1):
            self.nodes[i].sentinelping("1.1.0")

    def setup_network(self):
        self.nodes = []
        # create faucet node for collateral and transactions
        self.nodes.append(start_node(0, self.options.tmpdir, ["-debug"]))
        required_balance = MASTERNODE_COLLATERAL * self.mn_count + 1
        while self.nodes[0].getbalance() < required_balance:
            set_mocktime(get_mocktime() + 1)
            set_node_times(self.nodes, get_mocktime())
            self.nodes[0].generate(1)
        # create masternodes
        self.prepare_masternodes()
        self.write_mn_config()
        stop_node(self.nodes[0], 0)
        self.nodes[0] = start_node(0, self.options.tmpdir,
                                   ["-debug", "-sporkkey=cP4EKFyJsHT39LDqgdcB43Y3YXjNyjb5Fuas1GQSeAtjnZWmZEQK"])
        self.create_masternodes()
        # create connected simple nodes
        for i in range(0, self.num_nodes - self.mn_count - 1):
            self.create_simple_node()
        # feed the sender with some balance
        sender_addr = self.nodes[self.sender_idx].getnewaddress()
        self.nodes[0].sendtoaddress(sender_addr, 1)
        # make sender funds mature for InstantSend
        for i in range(0, 2):
            set_mocktime(get_mocktime() + 1)
            set_node_times(self.nodes, get_mocktime())
            self.nodes[0].generate(1)
        # sync nodes
        self.sync_all()
        set_mocktime(get_mocktime() + 1)
        set_node_times(self.nodes, get_mocktime())
        sync_masternodes(self.nodes)
        for i in range(1, self.mn_count + 1):
            res = self.nodes[0].masternode("start-alias", "mn%d" % i)
            assert(res["result"] == 'successful')
        sync_masternodes(self.nodes)
        #self.sentinel()
        mn_info = self.nodes[0].masternodelist("status")
        assert(len(mn_info) == self.mn_count)
        for status in mn_info.values():
            assert(status == 'ENABLED')

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

    def enforce_masternode_payments(self):
        self.nodes[0].spork('SPORK_8_MASTERNODE_PAYMENT_ENFORCEMENT', 0)

    def create_raw_trx(self, node_from, node_to, amount, min_inputs, max_inputs):
        assert(min_inputs <= max_inputs)
        # fill inputs
        inputs=[]
        balances = node_from.listunspent()
        in_amount = 0.0
        last_amount = 0.0
        for tx in balances:
            if len(inputs) < min_inputs:
                input = {}
                input["txid"] = tx['txid']
                input['vout'] = tx['vout']
                in_amount += float(tx['amount'])
                inputs.append(input)
            elif in_amount > amount:
                break
            elif len(inputs) < max_inputs:
                input = {}
                input["txid"] = tx['txid']
                input['vout'] = tx['vout']
                in_amount += float(tx['amount'])
                inputs.append(input)
            else:
                input = {}
                input["txid"] = tx['txid']
                input['vout'] = tx['vout']
                in_amount -= last_amount
                in_amount += float(tx['amount'])
                inputs[-1] = input
            last_amount = float(tx['amount'])

        assert(len(inputs) > 0)
        assert(in_amount > amount)
        # fill outputs
        receiver_address = node_to.getnewaddress()
        change_address = node_from.getnewaddress()
        fee = 0.001
        outputs={}
        outputs[receiver_address] = amount
        outputs[change_address] = in_amount - amount - fee
        rawtx = node_from.createrawtransaction(inputs, outputs)
        return node_from.signrawtransaction(rawtx)

    def check_IX_lock(self, txid):
        # wait for instantsend locks
        start = time()
        locked = False
        while True:
            is_trx = self.nodes[0].gettransaction(txid)
            if is_trx['instantlock']:
                locked = True
                break
            if time() > start + 10:
                break
            sleep(0.1)
        return locked

    # sends regular IX with high fee and may inputs (not-simple transaction)
    def send_regular_IX(self):
        receiver_addr = self.nodes[self.receiver_idx].getnewaddress()
        txid = self.nodes[0].instantsendtoaddress(receiver_addr, 1.0)
        MIN_FEE = satoshi_round(-0.0001)
        fee = self.nodes[0].gettransaction(txid)['fee']
        expected_fee = MIN_FEE * len(self.nodes[0].getrawtransaction(txid, True)['vin'])
        assert_equal(fee, expected_fee)
        return self.check_IX_lock(txid)


    # sends simple trx, it should become IX if autolocks are allowed
    def send_simple_tx(self):
        raw_tx = self.create_raw_trx(self.nodes[0], self.nodes[self.receiver_idx], 1.0, 1, 4)
        txid = self.nodes[0].sendrawtransaction(raw_tx['hex'])
        self.sync_all()
        return self.check_IX_lock(txid)

    # sends complex trx, it should never become IX
    def send_complex_tx(self):
        raw_tx = self.create_raw_trx(self.nodes[0], self.nodes[self.receiver_idx], 1.0, 5, 100)
        txid = self.nodes[0].sendrawtransaction(raw_tx['hex'])
        self.sync_all()
        return self.check_IX_lock(txid)

    def run_test(self):
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
