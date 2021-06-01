#!/usr/bin/env python3
# Copyright (c) 2014-2019 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test mempool limiting together/eviction with the wallet."""

from decimal import Decimal

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal, assert_greater_than, assert_raises_rpc_error, gen_return_txouts
from test_framework.wallet import MiniWallet
from test_framework.messages import CTransaction, FromHex

class MempoolLimitTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1
        self.extra_args = [[
            "-acceptnonstdtxn=1",
            "-maxmempool=5",
            "-spendzeroconfchange=0",
        ]]
        self.supports_cli = False

    def run_test(self):
        txouts = gen_return_txouts()
        node = self.nodes[0]
        miniwallet = MiniWallet(node)
        relayfee = node.getnetworkinfo()['relayfee']

        self.log.info('Check that mempoolminfee is minrelytxfee')
        assert_equal(node.getmempoolinfo()['minrelaytxfee'], Decimal('0.00001000'))
        assert_equal(node.getmempoolinfo()['mempoolminfee'], Decimal('0.00001000'))

        txids=[]
        miniwallet.generate(92)
        node.generate(100)

        self.log.info('Create a mempool tx that will be evicted')
        txid = miniwallet.send_self_transfer(fee_rate = relayfee, from_node = node)['txid']
        mempool_min_fee = node.getmempoolinfo()['mempoolminfee']
        base_fee = relayfee*1000
        for i in range (3):
            txids.append([])
            txids[i] = self.create_large_transactions(node, txouts, miniwallet, 30, (i+1)*base_fee, mempool_min_fee)

        self.log.info('The tx should be evicted by now')
        assert_greater_than(len([txid for batch in txids for txid in batch]), len(node.getrawmempool()))
        assert txid not in node.getrawmempool() # check txid in the raw mempool

        self.log.info('Check that mempoolminfee is larger than minrelytxfee')
        assert_equal(node.getmempoolinfo()['minrelaytxfee'], Decimal('0.00001000'))
        assert_greater_than(node.getmempoolinfo()['mempoolminfee'], Decimal('0.00001000'))

        self.log.info('Create a mempool tx that will not pass mempoolminfee')
        assert_raises_rpc_error(-26, "mempool min fee not met", miniwallet.send_self_transfer, from_node=node, fee_rate=relayfee)

    def create_large_transactions(self, node, txouts, miniwallet, num, fee, mempool_min_fee):
        large_txids = []
        for j in range(num):
            hex = miniwallet.send_self_transfer(from_node=node, fee_rate=fee, submit_tx=False)['hex']
            tx = FromHex(CTransaction(), hex)
            tx.vout.extend(txouts)
            tx_hex = tx.serialize().hex()
            txid = node.sendrawtransaction(tx_hex, 0)
            large_txids.append(txid)
        return large_txids

if __name__ == '__main__':
    MempoolLimitTest().main()
