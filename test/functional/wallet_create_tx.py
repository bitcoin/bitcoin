#!/usr/bin/env python3
# Copyright (c) 2018-2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.messages import (
    tx_from_hex,
)
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_raises_rpc_error,
)
from test_framework.blocktools import (
    TIME_GENESIS_BLOCK,
)


class CreateTxWalletTest(BitcoinTestFramework):
    def add_options(self, parser):
        self.add_wallet_options(parser)

    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        self.log.info('Create some old blocks')
        self.nodes[0].setmocktime(TIME_GENESIS_BLOCK)
        self.generate(self.nodes[0], 200)
        self.nodes[0].setmocktime(0)

        self.test_anti_fee_sniping()
        self.test_tx_size_too_large()
        self.test_create_too_long_mempool_chain()
        self.test_version3()

    def test_anti_fee_sniping(self):
        self.log.info('Check that we have some (old) blocks and that anti-fee-sniping is disabled')
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
        outputs = {self.nodes[0].getnewaddress(address_type='bech32'): 0.000025 for _ in range(400)}
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

    def test_create_too_long_mempool_chain(self):
        self.log.info('Check too-long mempool chain error')
        df_wallet = self.nodes[0].get_wallet_rpc(self.default_wallet_name)

        self.nodes[0].createwallet("too_long")
        test_wallet = self.nodes[0].get_wallet_rpc("too_long")

        tx_data = df_wallet.send(outputs=[{test_wallet.getnewaddress(): 25}], options={"change_position": 0})
        txid = tx_data['txid']
        vout = 1

        self.nodes[0].syncwithvalidationinterfacequeue()
        options = {"change_position": 0, "add_inputs": False}
        for i in range(1, 25):
            options['inputs'] = [{'txid': txid, 'vout': vout}]
            tx_data = test_wallet.send(outputs=[{test_wallet.getnewaddress(): 25 - i}], options=options)
            txid = tx_data['txid']

        # Sending one more chained transaction will fail
        options = {"minconf": 0, "include_unsafe": True, 'add_inputs': True}
        assert_raises_rpc_error(-4, "Unconfirmed UTXOs are available, but spending them creates a chain of transactions that will be rejected by the mempool",
                                test_wallet.send, outputs=[{test_wallet.getnewaddress(): 0.3}], options=options)

        test_wallet.unloadwallet()

    def test_version3(self):
        self.log.info('Check wallet does not create transactions with version=3 yet')
        wallet_rpc = self.nodes[0].get_wallet_rpc(self.default_wallet_name)

        self.nodes[0].createwallet("version3")
        wallet_v3 = self.nodes[0].get_wallet_rpc("version3")

        tx_data = wallet_rpc.send(outputs=[{wallet_v3.getnewaddress(): 25}], options={"change_position": 0})
        wallet_tx_data = wallet_rpc.gettransaction(tx_data["txid"])
        tx_current_version = tx_from_hex(wallet_tx_data["hex"])

        # While version=3 transactions are standard, the CURRENT_VERSION is 2.
        # This test can be removed if CURRENT_VERSION is changed, and replaced with tests that the
        # wallet handles TRUC rules properly.
        assert_equal(tx_current_version.version, 2)
        wallet_v3.unloadwallet()


if __name__ == '__main__':
    CreateTxWalletTest(__file__).main()
