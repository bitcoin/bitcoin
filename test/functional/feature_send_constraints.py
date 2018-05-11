#!/usr/bin/env python3
# Copyright (c) 2018 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Tests the constraints argument in send*"""

import random
from decimal import Decimal

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_raises_rpc_error,
)

def sendtoaddress_fun(sender, receiver, amount, constraints):
    sender.sendtoaddress(address=receiver.getnewaddress(), amount=round(amount, 8), constraints=constraints)

def sendmany_fun(sender, receiver, amount, constraints):
    sender.sendmany(dummy="", amounts={receiver.getnewaddress(): round(Decimal(amount)/2, 8), receiver.getnewaddress(): round(Decimal(amount)/2, 8)}, constraints=constraints)

class SendConstraintsTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = False
        self.num_nodes = 2

    def prep(self, amounts):
        """
        Prepare the given amounts in node 1 from node 0, then generate (confirm)
        and sync up.
        """
        self.sync_all()
        [self.nodes[0].sendtoaddress(self.nodes[1].getnewaddress(), i) for i in amounts]
        self.nodes[0].generate(1)
        self.sync_all()

    def iterate(self, fun, label, rpccode):

        # Clean-up
        self.sync_all()
        n1b = self.nodes[1].getbalance()
        if n1b > 0.01:
            self.nodes[1].sendtoaddress(address=self.nodes[0].getnewaddress(), amount=n1b, subtractfeefromamount=True)
            self.sync_all()

        ########################
        # MAX INPUT CONSTRAINT #
        ########################

        self.log.info("%s::test max input constraint" % (label))

        # Arrange 2 utxos in node 1
        self.prep([1.1, 2])

        # Attempt to send 3 coins back to node 0 with max_inputs=1 - fails
        assert_raises_rpc_error(rpccode, "Insufficient funds", fun, self.nodes[1], self.nodes[0], 3, {"max_inputs":1})

        # Attempt the same with max_inputs=2 - succeeds
        fun(self.nodes[1], self.nodes[0], 3, {"max_inputs":2})

        # Arrange 3 utxos in node 1
        self.prep([1.1, 2, 100])

        # Attempt to send 101 coins back to node 0 with max_inputs=1 - fails
        assert_raises_rpc_error(rpccode, "Insufficient funds", fun, self.nodes[1], self.nodes[0], 101, {"max_inputs":1})

        # Attempt to send 3 coins back to node 0 with max_inputs=1 - succeeds
        fun(self.nodes[1], self.nodes[0], 3, {"max_inputs":1})

        # node 1 should now have 1.1, 2, ~97
        # Attempt to send 100 coins with max_inputs=1..2 - fails
        assert_raises_rpc_error(rpccode, "Insufficient funds", fun, self.nodes[1], self.nodes[0], 100, {"max_inputs":1})
        assert_raises_rpc_error(rpccode, "Insufficient funds", fun, self.nodes[1], self.nodes[0], 100, {"max_inputs":2})

        # Attempt to send 100 coins with max_inputs=3 - succeeds
        fun(self.nodes[1], self.nodes[0], 100, {"max_inputs":3})

        ##############################
        # SELECTED INPUTS CONSTRAINT #
        ##############################

        self.log.info("%s::test selected (whitelisted) input constraint (one input)" % (label))

        # Send some amounts to node 1
        self.prep([i + 1 for i in range(20)])

        # Pick each and send it with arbitrary fee to node 0
        utxos = self.nodes[1].listunspent()
        utxolen = len(utxos)
        while utxolen > 0:
            idx = random.choice(range(utxolen))
            u = utxos[idx]
            del utxos[idx]
            utxolen -= 1
            txid = u["txid"]
            vout = u["vout"]
            amount = u["amount"]
            if amount > 0.001:
                withfees = amount - Decimal("0.00010000")
                fun(self.nodes[1], self.nodes[0], withfees, {"inputs":[{"txid":txid,"vout":vout}]})

        self.log.info("%s::test selected (whitelisted) input constraint (two inputs)" % (label))

        # Send some amounts to node 1 again
        self.prep([i + 1 for i in range(20)])

        # Pick two at a time and send with arbitrary fee to node 0
        utxos = self.nodes[1].listunspent()
        utxolen = len(utxos)
        while utxolen > 1:
            idx = random.choice(range(utxolen))
            u = utxos[idx]
            del utxos[idx]
            utxolen -= 1
            idx = random.choice(range(utxolen))
            u2 = utxos[idx]
            del utxos[idx]
            utxolen -= 1
            txid = u["txid"]
            vout = u["vout"]
            amount = u["amount"]
            txid2 = u2["txid"]
            vout2 = u2["vout"]
            amount2 = u2["amount"]
            if amount + amount2 > 0.001:
                withfees = amount + amount2 - Decimal("0.00010000")
                fun(self.nodes[1], self.nodes[0], withfees, {"inputs":[{"txid":txid,"vout":vout}, {"txid":txid2,"vout":vout2}]})

    def run_test(self):
        self.nodes[0].generate(110)
        self.iterate(sendtoaddress_fun, "sendtoaddress", -4)
        self.iterate(sendmany_fun, "sendmany", -6)

if __name__ == '__main__':
    SendConstraintsTest().main()
