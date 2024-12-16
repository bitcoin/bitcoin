#!/usr/bin/env python3
# Copyright (c) 2019-2021 The Dash Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.test_framework import BitcoinTestFramework
from test_framework.messages import (
    COIN,
    MAX_MONEY,
)
from test_framework.util import (
    assert_equal,
    assert_raises_rpc_error,
)

# See coinjoin/options.h
COINJOIN_ROUNDS_DEFAULT = 4
COINJOIN_ROUNDS_MAX = 16
COINJOIN_ROUNDS_MIN = 2
COINJOIN_TARGET_MAX = int(MAX_MONEY / COIN)
COINJOIN_TARGET_MIN = 2

class CoinJoinTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def setup_nodes(self):
        self.add_nodes(self.num_nodes)
        self.start_nodes()

    def run_test(self):
        node = self.nodes[0]

        node.createwallet(wallet_name='w1', blank=True, disable_private_keys=False)
        w1 = node.get_wallet_rpc('w1')
        self.test_coinjoin_start_stop(w1)
        self.test_setcoinjoinamount(w1)
        self.test_setcoinjoinrounds(w1)

    def test_coinjoin_start_stop(self, node):
        self.log.info('"coinjoin" subcommands should update mixing status')
        # Start mix session and ensure it's reported
        node.coinjoin('start')
        cj_info = node.getcoinjoininfo()
        assert_equal(cj_info['enabled'], True)
        assert_equal(cj_info['running'], True)
        # Repeated start should yield error
        assert_raises_rpc_error(-32603, 'Mixing has been started already.', node.coinjoin, 'start')

        # Stop mix session and ensure it's reported
        node.coinjoin('stop')
        cj_info = node.getcoinjoininfo()
        assert_equal(cj_info['enabled'], True)
        assert_equal(cj_info['running'], False)
        # Repeated stop should yield error
        assert_raises_rpc_error(-32603, 'No mix session to stop', node.coinjoin, 'stop')

        # Reset mix session
        assert_equal(node.coinjoin('reset'), "Mixing was reset")

    def test_setcoinjoinamount(self, node):
        self.log.info('"setcoinjoinamount" should update mixing target')
        # Test normal and large values
        for value in [COINJOIN_TARGET_MIN, 50, 1200000, COINJOIN_TARGET_MAX]:
            node.setcoinjoinamount(value)
            assert_equal(node.getcoinjoininfo()['max_amount'], value)
        # Test values below minimum and above maximum
        for value in [COINJOIN_TARGET_MIN - 1, COINJOIN_TARGET_MAX + 1]:
            assert_raises_rpc_error(-8, "Invalid amount of DASH as mixing goal amount", node.setcoinjoinamount, value)

    def test_setcoinjoinrounds(self, node):
        self.log.info('"setcoinjoinrounds" should update mixing rounds')
        # Test acceptable values
        for value in [COINJOIN_ROUNDS_MIN, COINJOIN_ROUNDS_DEFAULT, COINJOIN_ROUNDS_MAX]:
            node.setcoinjoinrounds(value)
            assert_equal(node.getcoinjoininfo()['max_rounds'], value)
        # Test values below minimum and above maximum
        for value in [COINJOIN_ROUNDS_MIN - 1, COINJOIN_ROUNDS_MAX + 1]:
            assert_raises_rpc_error(-8, "Invalid number of rounds", node.setcoinjoinrounds, value)

if __name__ == '__main__':
    CoinJoinTest().main()
