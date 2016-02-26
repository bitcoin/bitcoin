#!/usr/bin/env python3
# Copyright (c) 2016 The Bitcoin Core developers
# Distributed under the MIT/X11 software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
#

from test_framework.blocktools import create_block, create_coinbase
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import *

from binascii import a2b_hex, b2a_hex
from decimal import Decimal

def find_unspent(node, txid, amount):
    for utxo in node.listunspent(0):
        if utxo['txid'] != txid: continue
        if utxo['amount'] != amount: continue
        return {'txid': utxo['txid'], 'vout': utxo['vout']}

def solve_template_hex(tmpl, txlist):
    block = create_block(tmpl=tmpl, txlist=txlist)
    block.solve()
    b = block.serialize()
    x = b2a_hex(b).decode('ascii')
    return x

def get_modified_size(node, txdata):
    decoded = node.decoderawtransaction(txdata)
    size = len(txdata) // 2
    for inp in decoded['vin']:
        offset = 41 + min(len(inp['scriptSig']['hex']) // 2, 110)
        if offset <= size:
            size -= offset
    return size

def assert_approximate(a, b):
    assert_equal(int(a), int(b))

BTC = Decimal('100000000')

class PriorityTest(BitcoinTestFramework):

    def __init__(self):
        super().__init__()
        self.num_nodes = 4
        self.testmsg_num = 0

    def add_options(self, parser):
        parser.add_option("--gbt", dest="test_gbt", default=False, action="store_true",
                          help="Test priorities used by GBT")

    def setup_nodes(self):
        ppopt = []
        if self.options.test_gbt:
            ppopt.append('-printpriority')

        nodes = []
        nodes.append(start_node(0, self.options.tmpdir, ['-blockmaxsize=0']))
        nodes.append(start_node(1, self.options.tmpdir, ['-blockprioritysize=1000000', '-blockmaxsize=1000000'] + ppopt))
        nodes.append(start_node(2, self.options.tmpdir, ['-blockmaxsize=0']))
        nodes.append(start_node(3, self.options.tmpdir, ['-blockmaxsize=0']))
        return nodes

    def assert_prio(self, txid, starting, current):
        node = self.nodes[1]

        if self.options.test_gbt:
            tmpl = node.getblocktemplate({})
            tmplentry = None
            for tx in tmpl['transactions']:
                if tx['hash'] == txid:
                    tmplentry = tx
                    break
            # GBT does not expose starting priority, so we don't check that
            assert_approximate(tmplentry['priority'], current)
        else:
            mempoolentry = node.getrawmempool(True)[txid]
            assert_approximate(mempoolentry['startingpriority'], starting)
            assert_approximate(mempoolentry['currentpriority'], current)

    def testmsg(self, msg):
        self.testmsg_num += 1
        print('Test %d: %s' % (self.testmsg_num, msg))

    def run_test(self):
        node = self.nodes[0]
        miner = self.nodes[1]

        node.generate(50)
        self.sync_all()
        miner.generate(101)
        self.sync_all()

        fee = Decimal('0.01')
        amt = Decimal('11')

        txid_a = node.sendtoaddress(node.getnewaddress(), amt)
        txdata_b = node.createrawtransaction([find_unspent(node, txid_a, amt)], {node.getnewaddress(): amt - fee})
        txdata_b = node.signrawtransaction(txdata_b)['hex']
        txmodsize_b = get_modified_size(node, txdata_b)
        txid_b = node.sendrawtransaction(txdata_b)
        self.sync_all()

        self.testmsg('priority starts at 0 with all unconfirmed inputs')
        self.assert_prio(txid_b, 0, 0)

        self.testmsg('priority increases correctly when that input is mined')

        # Mine only the sendtoaddress transaction
        tmpl = node.getblocktemplate()
        rawblock = solve_template_hex(tmpl, [create_coinbase(tmpl['height']), a2b_hex(node.getrawtransaction(txid_a))])
        assert_equal(node.submitblock(rawblock), None)
        self.sync_all()

        self.assert_prio(txid_b, 0, amt * BTC / txmodsize_b)

        self.testmsg('priority continues to increase the deeper the block confirming its inputs gets buried')

        node.generate(2)
        self.sync_all()

        self.assert_prio(txid_b, 0, amt * BTC * 3 / txmodsize_b)

        self.testmsg('with a confirmed input, the initial priority is calculated correctly')

        miner.generate(4)
        self.sync_all()

        amt_c = (amt - fee) / 2
        amt_c2 = amt_c - (fee * 2)  # could be just amt_c-fee, but then it'd be undiscernable later on
        txdata_c = node.createrawtransaction([find_unspent(node, txid_b, amt - fee)], {node.getnewaddress(): amt_c, node.getnewaddress(): amt_c2})
        txdata_c = node.signrawtransaction(txdata_c)['hex']
        txmodsize_c = get_modified_size(node, txdata_c)
        txid_c = node.sendrawtransaction(txdata_c)
        self.sync_all()

        txid_c_starting_prio = (amt - fee) * BTC * 4 / txmodsize_c
        self.assert_prio(txid_c, txid_c_starting_prio, txid_c_starting_prio)

        self.testmsg('with an input confirmed prior to the transaction, the priority gets incremented correctly as it gets buried deeper')

        node.generate(1)
        self.sync_all()

        self.assert_prio(txid_c, txid_c_starting_prio, (amt - fee) * BTC * 5 / txmodsize_c)

        self.testmsg('with an input confirmed prior to the transaction, the priority gets incremented correctly as it gets buried deeper and deeper')

        node.generate(2)
        self.sync_all()

        self.assert_prio(txid_c, txid_c_starting_prio, (amt - fee) * BTC * 7 / txmodsize_c)

        print('(preparing for reorg test)')

        miner.generate(1)
        self.sync_all()

        self.split_network()
        node = self.nodes[0]
        miner = self.nodes[1]
        competing_miner = self.nodes[2]

        txdata_d = node.createrawtransaction([find_unspent(node, txid_c, amt_c)], {node.getnewaddress(): amt_c - fee})
        txdata_d = node.signrawtransaction(txdata_d)['hex']
        txmodsize_d = get_modified_size(node, txdata_d)
        txid_d = node.sendrawtransaction(txdata_d)
        self.sync_all()

        miner.generate(1)
        self.sync_all()

        txdata_e = node.createrawtransaction([find_unspent(node, txid_d, amt_c - fee), find_unspent(node, txid_c, amt_c2)], {node.getnewaddress(): (amt_c - fee) + amt_c2 - fee})
        txdata_e = node.signrawtransaction(txdata_e)['hex']
        txmodsize_e = get_modified_size(node, txdata_e)
        txid_e = node.sendrawtransaction(txdata_e)
        self.sync_all()

        txid_e_starting_prio = (((amt_c - fee) * BTC) + (amt_c2 * BTC * 2)) / txmodsize_e
        self.assert_prio(txid_e, txid_e_starting_prio, txid_e_starting_prio)  # Sanity check 1

        competing_miner.generate(5)
        self.sync_all()

        self.assert_prio(txid_e, txid_e_starting_prio, txid_e_starting_prio)  # Sanity check 2

        self.testmsg('priority is updated correctly when input-confirming block is reorganised out')

        # NOTE: We cannot use join_network because it restarts the nodes (thus losing the mempool)
        connect_nodes_bi(self.nodes, 1, 2)
        self.is_network_split = False
        sync_blocks(self.nodes)

        txid_e_reorg_prio = (amt_c2 * BTC * 6) / txmodsize_e
        self.assert_prio(txid_e, txid_e_starting_prio, txid_e_reorg_prio)

if __name__ == '__main__':
    PriorityTest().main()
