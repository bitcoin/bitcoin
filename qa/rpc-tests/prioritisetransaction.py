#!/usr/bin/env python2
# Copyright (c) 2014 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework import BitcoinTestFramework
from bitcoinrpc.authproxy import AuthServiceProxy, JSONRPCException
from util import *

def fresh_gbt(anode):
    tmpl = anode.getblocktemplate({"use_cache": False})
    return tmpl

def tmpl_tx(tmpl, txid):
    for tx in tmpl['transactions']:
        if tx['hash'] == txid:
            return tx

class PrioritiseTransactionTest(BitcoinTestFramework):
    '''
    Test prioritisetransaction with getblocktemplate.
    '''

    def run_test(self):
        node = self.nodes[0]

        node.setgenerate(True, 10)

        (txid_a, txhex, fee) = random_transaction(self.nodes, Decimal("1.1"), 1, 0, 20)
        (txid_b, txhex, fee) = random_transaction(self.nodes, Decimal("1.1"), 2, 0, 20)
        sync_blocks(self.nodes)
        sync_mempools(self.nodes)

        # Put both transactions at the same base priority/fee level
        tmpl = fresh_gbt(node)
        prio_a = tmpl_tx(tmpl, txid_a)['priority_adjusted']
        prio_b = tmpl_tx(tmpl, txid_b)['priority_adjusted']
        base_prio = max(prio_a, prio_b) + 1000000
        assert(node.prioritisetransaction(txid_a, base_prio - prio_a, 200000000))
        assert(node.prioritisetransaction(txid_b, base_prio - prio_b, 100000000))

        if len(fresh_gbt(node)['transactions']) != 2:
            raise AssertionError('Unexpected transaction count in template')

        def testadj(txid, prio, fee):
            assert(node.prioritisetransaction(txid, prio, fee))
            tmpl = fresh_gbt(node)
            tx = tmpl_tx(tmpl, txid)
            assert(tx['fee_adjusted'] == 300000000 + fee)
            assert(tx['priority_adjusted'] == base_prio + prio)
            # NOTE: Fees don't actually affect block ordering
            if prio and tmpl['transactions'][0]['hash'] != txid:
                raise AssertionError('Higher priority ignored')
            assert(node.prioritisetransaction(txid, -prio * 2, -fee * 2))
            tmpl = fresh_gbt(node)
            tx = tmpl_tx(tmpl, txid)
            assert(tx['fee_adjusted'] == 300000000 - fee)
            assert(tx['priority_adjusted'] == base_prio - prio)
            if prio and tmpl['transactions'][0]['hash'] == txid:
                raise AssertionError('Lower priority ignored')
            assert(node.prioritisetransaction(txid, prio, fee))

        testadj(txid_a, 1, 0)
        testadj(txid_b, 1, 0)
        testadj(txid_a, 0, 1)
        testadj(txid_b, 0, 1)

if __name__ == '__main__':
    PrioritiseTransactionTest().main()
