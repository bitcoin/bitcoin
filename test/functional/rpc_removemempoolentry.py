#!/usr/bin/env python3
# Copyright (c) 2014-2019 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the removemempoolentry RPC."""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal, 
    assert_raises_rpc_error, 
    assert_greater_than
)


class MempoolRemoveTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1
        self.extra_args = [["-minrelaytxfee=0"]]

    def setup_network(self):
        self.setup_nodes()

    def run_test(self):
        self.log.info("Mine some blocks so there are coins to spend")
        beneficiary1 = self.nodes[0].getnewaddress()
        self.nodes[0].generatetoaddress(101, beneficiary1)

        res = self.nodes[0].listunspent(100, 99999999, [beneficiary1])
        outpoint = res[0]

        self.log.info("Generate and broadcast first transaction")
        beneficiary2 = self.nodes[0].getnewaddress()
        res = self.nodes[0].createrawtransaction([{'txid': outpoint['txid'], 
                                                'vout': outpoint['vout']}], 
                                                [{beneficiary2: outpoint['amount']}])
        res = self.nodes[0].signrawtransactionwithwallet(res)
        orig_txid = self.nodes[0].sendrawtransaction(res['hex'])

        

        self.log.info("Generate second transaction, double-spending the first")
        beneficiary3 = self.nodes[0].getnewaddress()
        res = self.nodes[0].createrawtransaction([{'txid': outpoint['txid'], 
                                                'vout': outpoint['vout']}], 
                                                [{beneficiary3: outpoint['amount']}])
        res = self.nodes[0].signrawtransactionwithwallet(res)

        self.log.info("Check second transaction cannot be added to mempool")
        assert_raises_rpc_error(-26, 'txn-mempool-conflict (code 18)', self.nodes[0].sendrawtransaction, res['hex'])

        self.log.info("Remove first transaction from mempool")
        assert_equal(self.nodes[0].removemempoolentry(orig_txid), True)
        assert_raises_rpc_error(-5, 'Transaction not in mempool', self.nodes[0].removemempoolentry, orig_txid)

        self.log.info("Send double-spend transaction")
        ds_txid = self.nodes[0].sendrawtransaction(res['hex'])

        self.log.info("Check double-spend transaction is in the mempool")
        assert_raises_rpc_error(-5, 'Transaction not in mempool', self.nodes[0].getmempoolentry, orig_txid)
        assert_greater_than(len(self.nodes[0].getmempoolentry(ds_txid)), 0)

if __name__ == '__main__':
    MempoolRemoveTest().main()
