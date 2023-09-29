#!/usr/bin/env python3
# Copyright (c) 2016 The Bitcoin Core developers
# Distributed under the MIT/X11 software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
#

from test_framework.blocktools import create_block
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal

from binascii import b2a_hex
from decimal import Decimal

def find_unspent(node, txid, amount):
    for utxo in node.listunspent(0):
        if utxo['txid'] != txid:
            continue
        if utxo['amount'] != amount:
            continue
        return {'txid': utxo['txid'], 'vout': utxo['vout']}

def solve_template_hex(tmpl, txlist):
    block = create_block(tmpl=tmpl, txlist=txlist)
    block.solve()
    b = block.serialize()
    x = b2a_hex(b).decode('ascii')
    return x

def get_modified_size(node, txdata):
    decoded = node.decoderawtransaction(txdata)
    size = decoded['vsize']
    for inp in decoded['vin']:
        offset = 41 + min(len(inp['scriptSig']['hex']) // 2, 110)
        if offset <= size:
            size -= offset
    return size

def assert_approximate(a, b):
    assert_equal(int(a), int(b))

BTC = Decimal('100000000')

class PriorityTest(BitcoinTestFramework):
    def add_options(self, parser):
        self.add_wallet_options(parser)

    def set_test_params(self):
        self.num_nodes = 3
        self.testmsg_num = 0

    def setup_nodes(self):
        self.extra_args = [
            ['-blockmaxsize=0'],
            ['-blockprioritysize=1000000', '-blockmaxsize=1000000', '-printpriority'],
            ['-blockmaxsize=0'],
        ]

        super().setup_nodes()

    def assert_prio(self, txid, starting, current):
        node = self.nodes[1]

        tmpl = node.getblocktemplate({'rules':('segwit',)})
        tmplentry = None
        for tx in tmpl['transactions']:
            if tx['txid'] == txid:
                tmplentry = tx
                break
        # GBT does not expose starting priority, so we don't check that
        assert_approximate(tmplentry['priority'], current)

        mempoolentry = node.getrawmempool(True)[txid]
        assert_approximate(mempoolentry['startingpriority'], starting)
        assert_approximate(mempoolentry['currentpriority'], current)

    def testmsg(self, msg):
        self.testmsg_num += 1
        self.log.info('Test %d: %s' % (self.testmsg_num, msg))

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        node = self.nodes[0]
        miner = self.nodes[1]

        self.generate(node, 50)
        self.generate(miner, 101)

        fee = Decimal('0.0001')
        amt = Decimal('11')

        txid_a = node.sendtoaddress(node.getnewaddress(), amt)
        txdata_b = node.createrawtransaction([find_unspent(node, txid_a, amt)], {node.getnewaddress(): amt - fee})
        txdata_b = node.signrawtransactionwithwallet(txdata_b)['hex']
        txmodsize_b = get_modified_size(node, txdata_b)
        txid_b = node.sendrawtransaction(txdata_b)
        self.sync_all()

        self.testmsg('priority starts at 0 with all unconfirmed inputs')
        self.assert_prio(txid_b, 0, 0)

        self.testmsg('priority increases correctly when that input is mined')

        # Mine only the sendtoaddress transaction
        tmpl = node.getblocktemplate({'rules':('segwit',)})
        rawblock = solve_template_hex(tmpl, [node.getrawtransaction(txid_a)])
        assert_equal(node.submitblock(rawblock), None)
        self.sync_all()

        self.assert_prio(txid_b, 0, amt * BTC / txmodsize_b)

        self.testmsg('priority continues to increase the deeper the block confirming its inputs gets buried')

        self.generate(node, 2)

        self.assert_prio(txid_b, 0, amt * BTC * 3 / txmodsize_b)

        self.testmsg('with a confirmed input, the initial priority is calculated correctly')

        self.generate(miner, 4)

        amt_c = (amt - fee) / 2
        amt_c2 = amt_c - fee
        txdata_c = node.createrawtransaction([find_unspent(node, txid_b, amt - fee)], {node.getnewaddress(): amt_c, node.getnewaddress(): amt_c2})
        txdata_c = node.signrawtransactionwithwallet(txdata_c)['hex']
        txmodsize_c = get_modified_size(node, txdata_c)
        txid_c = node.sendrawtransaction(txdata_c)
        self.sync_all()

        txid_c_starting_prio = (amt - fee) * BTC * 4 / txmodsize_c
        self.assert_prio(txid_c, txid_c_starting_prio, txid_c_starting_prio)

        self.testmsg('with an input confirmed prior to the transaction, the priority gets incremented correctly as it gets buried deeper')

        self.generate(node, 1)

        self.assert_prio(txid_c, txid_c_starting_prio, (amt - fee) * BTC * 5 / txmodsize_c)

        self.testmsg('with an input confirmed prior to the transaction, the priority gets incremented correctly as it gets buried deeper and deeper')

        self.generate(node, 2)

        self.assert_prio(txid_c, txid_c_starting_prio, (amt - fee) * BTC * 7 / txmodsize_c)

        self.log.info('(preparing for reorg test)')

        self.generate(miner, 1)

        self.split_network()
        node = self.nodes[0]
        miner = self.nodes[1]
        competing_miner = self.nodes[2]

        txdata_d = node.createrawtransaction([find_unspent(node, txid_c, amt_c)], {node.getnewaddress(): amt_c - fee})
        txdata_d = node.signrawtransactionwithwallet(txdata_d)['hex']
        get_modified_size(node, txdata_d)
        txid_d = node.sendrawtransaction(txdata_d)
        self.sync_all(self.nodes[:2])
        self.sync_all(self.nodes[2:])

        self.generate(miner, 1, sync_fun=self.no_op)
        self.sync_all(self.nodes[:2])
        self.sync_all(self.nodes[2:])

        txdata_e = node.createrawtransaction([find_unspent(node, txid_d, amt_c - fee), find_unspent(node, txid_c, amt_c2)], {node.getnewaddress(): (amt_c - fee) + amt_c2 - fee})
        txdata_e = node.signrawtransactionwithwallet(txdata_e)['hex']
        txmodsize_e = get_modified_size(node, txdata_e)
        txid_e = node.sendrawtransaction(txdata_e)
        self.sync_all(self.nodes[:2])
        self.sync_all(self.nodes[2:])

        txid_e_starting_prio = (((amt_c - fee) * BTC) + (amt_c2 * BTC * 2)) / txmodsize_e
        self.assert_prio(txid_e, txid_e_starting_prio, txid_e_starting_prio)  # Sanity check 1

        self.generate(competing_miner, 5, sync_fun=self.no_op)
        self.sync_all(self.nodes[:2])
        self.sync_all(self.nodes[2:])

        self.assert_prio(txid_e, txid_e_starting_prio, txid_e_starting_prio)  # Sanity check 2

        self.testmsg('priority is updated correctly when input-confirming block is reorganised out')

        self.connect_nodes(1, 2)
        self.sync_blocks()

        txid_e_reorg_prio = (amt_c2 * BTC * 6) / txmodsize_e
        self.assert_prio(txid_e, txid_e_starting_prio, txid_e_reorg_prio)

if __name__ == '__main__':
    PriorityTest(__file__).main()
