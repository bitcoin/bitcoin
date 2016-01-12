#!/usr/bin/env python3
# Copyright (c) 2016 The Bitcoin Core developers
# Distributed under the MIT/X11 software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
#

from test_framework.blocktools import create_block, create_coinbase
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import *

from binascii import a2b_hex, b2a_hex

def find_unspent(node, txid, amount):
    for utxo in node.listunspent(0):
        if utxo['txid'] != txid: continue
        if float(utxo['amount']) != amount: continue
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

class PriorityTest(BitcoinTestFramework):

    def __init__(self):
        super().__init__()
        self.num_nodes = 2

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
        return nodes

    def setup_network(self, split=False):
        self.nodes = self.setup_nodes()
        connect_nodes_bi(self.nodes,0,1)
        self.is_network_split = False
        self.sync_all()

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

    def run_test(self):
        node = self.nodes[0]
        miner = self.nodes[1]
        node.generate(50)
        self.sync_all()
        miner.generate(101)
        self.sync_all()

        fee = 0.01
        amt = 11

        txid_a = node.sendtoaddress(node.getnewaddress(), amt)
        txdata_b = node.createrawtransaction([find_unspent(node, txid_a, amt)], {node.getnewaddress(): amt - fee})
        txdata_b = node.signrawtransaction(txdata_b)['hex']
        txmodsize_b = get_modified_size(node, txdata_b)
        txid_b = node.sendrawtransaction(txdata_b)
        self.sync_all()

        self.assert_prio(txid_b, 0, 0)

        # Mine only the sendtoaddress transaction
        tmpl = node.getblocktemplate()
        rawblock = solve_template_hex(tmpl, [create_coinbase(tmpl['height']), a2b_hex(node.getrawtransaction(txid_a))])
        assert_equal(node.submitblock(rawblock), None)
        self.sync_all()

        self.assert_prio(txid_b, 0, amt * 1e8 / txmodsize_b)

        node.generate(2)
        self.sync_all()

        self.assert_prio(txid_b, 0, amt * 1e8 * 3 / txmodsize_b)

        miner.generate(4)
        self.sync_all()

        txdata_c = node.createrawtransaction([find_unspent(node, txid_b, amt - fee)], {node.getnewaddress(): amt - (fee * 2)})
        txdata_c = node.signrawtransaction(txdata_c)['hex']
        txmodsize_c = get_modified_size(node, txdata_c)
        txid_c = node.sendrawtransaction(txdata_c)
        self.sync_all()

        txid_c_starting_prio = (amt - fee) * 1e8 * 4 / txmodsize_c
        self.assert_prio(txid_c, txid_c_starting_prio, txid_c_starting_prio)

        node.generate(1)
        self.sync_all()

        self.assert_prio(txid_c, txid_c_starting_prio, (amt - fee) * 1e8 * 5 / txmodsize_c)

        node.generate(2)
        self.sync_all()

        self.assert_prio(txid_c, txid_c_starting_prio, (amt - fee) * 1e8 * 7 / txmodsize_c)

if __name__ == '__main__':
    PriorityTest().main()
