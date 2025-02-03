#!/usr/bin/env python3
# Copyright (c) 2019-2024 The Dash Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

import random

from test_framework.test_framework import BitcoinTestFramework
from test_framework.messages import (
    COIN,
    MAX_MONEY,
    uint256_to_string,
)
from test_framework.util import (
    assert_equal,
    assert_is_hex_string,
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
        self.test_salt_presence(w1)
        self.test_coinjoin_start_stop(w1)
        self.test_setcoinjoinamount(w1)
        self.test_setcoinjoinrounds(w1)
        self.test_coinjoinsalt(w1)
        w1.unloadwallet()

        node.createwallet(wallet_name='w2', blank=True, disable_private_keys=True)
        w2 = node.get_wallet_rpc('w2')
        self.test_coinjoinsalt_disabled(w2)
        w2.unloadwallet()

    def test_salt_presence(self, node):
        self.log.info('Salt should be automatically generated in new wallet')
        # Will raise exception if no salt generated
        assert_is_hex_string(node.coinjoinsalt('get'))

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

    def test_coinjoinsalt(self, node):
        self.log.info('"coinjoinsalt generate" should fail if salt already present')
        assert_raises_rpc_error(-32600, 'Wallet "w1" already has set CoinJoin salt!', node.coinjoinsalt, 'generate')

        self.log.info('"coinjoinsalt" subcommands should succeed if no balance and not mixing')
        # 'coinjoinsalt generate' should return a new salt if overwrite enabled
        s1 = node.coinjoinsalt('get')
        assert_equal(node.coinjoinsalt('generate', True), True)
        s2 = node.coinjoinsalt('get')
        assert s1 != s2

        # 'coinjoinsalt get' should fetch newly generated value (i.e. new salt should persist)
        node.unloadwallet('w1')
        node.loadwallet('w1')
        node = self.nodes[0].get_wallet_rpc('w1')
        assert_equal(s2, node.coinjoinsalt('get'))

        # 'coinjoinsalt set' should work with random hashes
        s1 = uint256_to_string(random.getrandbits(256))
        node.coinjoinsalt('set', s1)
        assert_equal(s1, node.coinjoinsalt('get'))
        assert s1 != s2

        # 'coinjoinsalt set' shouldn't work with nonsense values
        s2 = format(0, '064x')
        assert_raises_rpc_error(-8, "Invalid CoinJoin salt value", node.coinjoinsalt, 'set', s2, True)
        s2 = s2[0:63] + 'h'
        assert_raises_rpc_error(-8, "salt must be hexadecimal string (not '%s')" % s2, node.coinjoinsalt, 'set', s2, True)

        self.log.info('"coinjoinsalt generate" and "coinjoinsalt set" should fail if mixing')
        # Start mix session
        node.coinjoin('start')
        assert_equal(node.getcoinjoininfo()['running'], True)

        # 'coinjoinsalt generate' and 'coinjoinsalt set' should fail when mixing
        assert_raises_rpc_error(-4, 'Wallet "w1" is currently mixing, cannot change salt!', node.coinjoinsalt, 'generate', True)
        assert_raises_rpc_error(-4, 'Wallet "w1" is currently mixing, cannot change salt!', node.coinjoinsalt, 'set', s1, True)

        # 'coinjoinsalt get' should still work
        assert_equal(node.coinjoinsalt('get'), s1)

        # Stop mix session
        node.coinjoin('stop')
        assert_equal(node.getcoinjoininfo()['running'], False)

        # 'coinjoinsalt generate' and 'coinjoinsalt set' should start working again
        assert_equal(node.coinjoinsalt('generate', True), True)
        assert_equal(node.coinjoinsalt('set', s1, True), True)

    def test_coinjoinsalt_disabled(self, node):
        self.log.info('"coinjoinsalt" subcommands should fail if private keys disabled')
        for subcommand in ['generate', 'get']:
            assert_raises_rpc_error(-32600, 'Wallet "w2" has private keys disabled, cannot perform CoinJoin!', node.coinjoinsalt, subcommand)
        s1 = uint256_to_string(random.getrandbits(256))
        assert_raises_rpc_error(-32600, 'Wallet "w2" has private keys disabled, cannot perform CoinJoin!', node.coinjoinsalt, 'set', s1)

if __name__ == '__main__':
    CoinJoinTest().main()
