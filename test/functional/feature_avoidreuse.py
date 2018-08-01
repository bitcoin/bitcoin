#!/usr/bin/env python3
# Copyright (c) 2017 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the -avoidreuse config option."""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_raises_rpc_error

# TODO: Copied from wallet_groups.py -- should perhaps move into util.py
def assert_approx(v, vexp, vspan=0.00001):
    if v < vexp - vspan:
        raise AssertionError("%s < [%s..%s]" % (str(v), str(vexp - vspan), str(vexp + vspan)))
    if v > vexp + vspan:
        raise AssertionError("%s > [%s..%s]" % (str(v), str(vexp - vspan), str(vexp + vspan)))

class AvoidReuseTest(BitcoinTestFramework):

    def set_test_params(self):
        self.setup_clean_chain = False
        self.num_nodes = 2
        self.extra_args = [[], ['-avoidreuse=1']]

    def reset_balance(self, node, discardaddr):
        '''
        Throw away all owned coins by the node so it gets a balance of 0.
        '''
        balance = node.getbalance(dest_filter='mixed')
        if balance > 0.5:
            node.sendtoaddress(address=discardaddr, amount=balance, subtractfeefromamount=True, dest_filter='mixed')

    def run_test(self):
        '''
        Set up initial chain and run tests defined below
        '''

        self.nodes[0].generate(110)
        self.sync_all()
        self.reset_balance(self.nodes[1], self.nodes[0].getnewaddress())
        self.test_fund_send_fund_send()
        self.reset_balance(self.nodes[1], self.nodes[0].getnewaddress())
        self.test_fund_send_fund_senddirty()

    def test_fund_send_fund_send(self):
        '''
        Test the simple case where [1] generates a new address A, then
        [0] sends 10 BTC to A.
        [1] spends 5 BTC from A. (leaving roughly 4 BTC useable)
        [0] sends 10 BTC to A again.
        [1] tries to spend 10 BTC (fails; dirty).
        [1] tries to spend 4 BTC (succeeds; change address sufficient)
        '''

        fundaddr = self.nodes[1].getnewaddress()
        retaddr = self.nodes[0].getnewaddress()

        self.nodes[0].sendtoaddress(fundaddr, 10)
        self.nodes[0].generate(1)
        self.sync_all()

        self.nodes[1].sendtoaddress(retaddr, 5)
        self.sync_all()
        self.nodes[0].generate(1)

        self.nodes[0].sendtoaddress(fundaddr, 10)
        self.sync_all()
        self.nodes[0].generate(1)

        # node 1 should now have a balance of 5 (no dirty) or 15 (including dirty)
        assert_approx(self.nodes[1].getbalance(), 5, 0.001)
        assert_approx(self.nodes[1].getbalance(dest_filter='mixed'), 15, 0.001)

        assert_raises_rpc_error(-6, "Insufficient funds", self.nodes[1].sendtoaddress, retaddr, 10)

        self.nodes[1].sendtoaddress(retaddr, 4)

    def test_fund_send_fund_senddirty(self):
        '''
        Test the same as above, except send the 10 BTC with the allow_dirty flag
        set. This means the 10 BTC send should succeed, where it fails above.
        '''

        fundaddr = self.nodes[1].getnewaddress()
        retaddr = self.nodes[0].getnewaddress()

        self.nodes[0].sendtoaddress(fundaddr, 10)
        self.nodes[0].generate(1)
        self.sync_all()

        self.nodes[1].sendtoaddress(retaddr, 5)
        self.sync_all()
        self.nodes[0].generate(1)

        self.nodes[0].sendtoaddress(fundaddr, 10)
        self.sync_all()
        self.nodes[0].generate(1)

        self.nodes[1].sendtoaddress(address=retaddr, amount=10, dest_filter='mixed')

if __name__ == '__main__':
    AvoidReuseTest().main()
