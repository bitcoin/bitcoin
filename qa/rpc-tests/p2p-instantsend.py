#!/usr/bin/env python3
# Copyright (c) 2018 The Dash Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.mininode import *
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import *
from time import *

'''
InstantSendTest -- test InstantSend functionality (prevent doublespend for unconfirmed transactions)
'''

MASTERNODE_COLLATERAL = 1000


class MasternodeInfo:
    def __init__(self, key, collateral_id, collateral_out):
        self.key = key
        self.collateral_id = collateral_id
        self.collateral_out = collateral_out


class InstantSendTest(BitcoinTestFramework):
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
            self.nodes[0].generate(1)
        # create masternodes
        self.prepare_masternodes()
        self.write_mn_config()
        stop_node(self.nodes[0], 0)
        self.nodes[0] = start_node(0, self.options.tmpdir, ["-debug"])
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
            self.nodes[0].generate(1)
        # sync nodes
        self.sync_all()
        set_mocktime(get_mocktime() + 1)
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

    def create_raw_trx(self, node_from, node_to, amount):
        # fill inputs
        inputs=[]
        balances = node_from.listunspent()
        for tx in balances:
            if tx['amount'] > amount:
                input = {}
                input["txid"] = tx['txid']
                input['vout'] = tx['vout']
                in_amount = float(tx['amount'])
                inputs.append(input)
                break
        assert(len(inputs) > 0)
        # fill outputs
        receiver_address = node_to.getnewaddress()
        change_address = node_from.getnewaddress()
        fee = 0.001
        outputs={}
        outputs[receiver_address] = amount
        outputs[change_address] = in_amount - amount - fee
        rawtx = node_from.createrawtransaction(inputs, outputs)
        return node_from.signrawtransaction(rawtx)

    def run_test(self):
        # create doublepending transaction,  but don't relay it
        dblspnd_tx = self.create_raw_trx(self.nodes[self.sender_idx],
                                       self.nodes[self.isolated_idx],
                                       0.5)
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
        assert(locked)
        # start last node
        self.nodes[self.isolated_idx] = start_node(self.isolated_idx,
                                                   self.options.tmpdir,
                                                   ["-debug"])
        # send doublespend transaction to isolated node
        self.nodes[self.isolated_idx].sendrawtransaction(dblspnd_tx['hex'])
        # generate block on isolated node with doublespend transaction
        set_mocktime(get_mocktime() + 1)
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
