#!/usr/bin/env python3
# Copyright (c) 2018-2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.messages import COIN
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_greater_than_or_equal,
    assert_raises_rpc_error,
)


class CreateTxWalletTest(BitcoinTestFramework):
    def add_options(self, parser):
        self.add_wallet_options(parser)

    def set_test_params(self):
        self.num_nodes = 1

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        self.test_anti_fee_sniping()
        self.test_tx_size_too_large()
        self.test_compact_size_counts()

    def test_anti_fee_sniping(self):
        self.log.info('Check that we have some (old) blocks and that anti-fee-sniping is disabled')
        self.bump_mocktime(8 * 60 * 60 + 1, update_schedulers=False)
        assert_equal(self.nodes[0].getblockchaininfo()['blocks'], 200)
        txid = self.nodes[0].sendtoaddress(self.nodes[0].getnewaddress(), 1)
        tx = self.nodes[0].gettransaction(txid=txid, verbose=True)['decoded']
        assert_equal(tx['locktime'], 0)

        self.log.info('Check that anti-fee-sniping is enabled when we mine a recent block')
        self.generate(self.nodes[0], 1)
        txid = self.nodes[0].sendtoaddress(self.nodes[0].getnewaddress(), 1)
        tx = self.nodes[0].gettransaction(txid=txid, verbose=True)['decoded']
        assert 0 < tx['locktime'] <= 201

    def test_tx_size_too_large(self):
        # More than 10kB of outputs, so that we hit -maxtxfee with a high feerate
        outputs = {self.nodes[0].getnewaddress(): 0.000025 for _ in range(400)}
        raw_tx = self.nodes[0].createrawtransaction(inputs=[], outputs=outputs)

        for fee_setting in ['-minrelaytxfee=0.01', '-mintxfee=0.01', '-paytxfee=0.01']:
            self.log.info('Check maxtxfee in combination with {}'.format(fee_setting))
            self.restart_node(0, extra_args=[fee_setting])
            assert_raises_rpc_error(
                -6,
                "Fee exceeds maximum configured by user (e.g. -maxtxfee, maxfeerate)",
                lambda: self.nodes[0].sendmany(dummy="", amounts=outputs),
            )
            assert_raises_rpc_error(
                -4,
                "Fee exceeds maximum configured by user (e.g. -maxtxfee, maxfeerate)",
                lambda: self.nodes[0].fundrawtransaction(hexstring=raw_tx),
            )

        self.log.info('Check maxtxfee in combination with settxfee')
        self.restart_node(0)
        self.nodes[0].settxfee(0.01)
        assert_raises_rpc_error(
            -6,
            "Fee exceeds maximum configured by user (e.g. -maxtxfee, maxfeerate)",
            lambda: self.nodes[0].sendmany(dummy="", amounts=outputs),
        )
        assert_raises_rpc_error(
            -4,
            "Fee exceeds maximum configured by user (e.g. -maxtxfee, maxfeerate)",
            lambda: self.nodes[0].fundrawtransaction(hexstring=raw_tx),
        )
        self.nodes[0].settxfee(0)

    def test_compact_size_counts(self):
        self.log.info('Check that the input and output counts are fully paid for once they need a 3 byte CompactSize')
        node = self.nodes[0]
        node.createwallet(wallet_name='compact_size', descriptors=self.options.descriptors)
        wallet = node.get_wallet_rpc('compact_size')
        default_wallet = node.get_wallet_rpc(self.default_wallet_name)

        def check_fee(txid, num_inputs, num_outputs):
            tx = wallet.gettransaction(txid=txid, verbose=True)
            vin, size = tx['decoded']['vin'], tx['decoded']['size']
            assert_greater_than_or_equal(len(vin), num_inputs)
            assert_equal(len(tx['decoded']['vout']), num_outputs)
            # The default feerate is 1 duff/byte. The fee is estimated with maximum size signatures, so it
            # may exceed the size by a couple of bytes for each signature that came out shorter.
            fee = -tx['fee'] * COIN
            assert_greater_than_or_equal(fee, size)
            assert_greater_than_or_equal(size + 2 * len(vin), fee)

        # Spend at least 253 inputs and create change
        num_utxos = 260
        default_wallet.sendmany(dummy="", amounts={wallet.getnewaddress(): 0.01 for _ in range(num_utxos)})
        self.generate(node, 1)
        txid = wallet.sendtoaddress(address=default_wallet.getnewaddress(), amount=2.55)
        check_fee(txid, num_inputs=253, num_outputs=2)
        self.generate(node, 1)

        # Pay 252 recipients, so that adding change makes it 253 outputs
        default_wallet.sendtoaddress(address=wallet.getnewaddress(), amount=1)
        self.generate(node, 1)
        txid = wallet.sendmany(dummy="", amounts={default_wallet.getnewaddress(): 0.001 for _ in range(252)})
        check_fee(txid, num_inputs=1, num_outputs=253)
        self.generate(node, 1)
        wallet.unloadwallet()


if __name__ == '__main__':
    CreateTxWalletTest().main()
