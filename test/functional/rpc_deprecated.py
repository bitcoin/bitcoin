#!/usr/bin/env python3
# Copyright (c) 2017-2019 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test deprecation of RPC calls."""
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_raises_rpc_error, find_vout_for_address

class DeprecatedRpcTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 2
        self.setup_clean_chain = True
        self.extra_args = [['-deprecatedrpc=feeRate'], ['-deprecatedrpc=bumpfee']]

    def run_test(self):
        # This test should be used to verify correct behaviour of deprecated
        # RPC methods with and without the -deprecatedrpc flags. For example:
        #
        # In set_test_params:
        # self.extra_args = [[], ["-deprecatedrpc=generate"]]
        #
        # In run_test:
        # self.log.info("Test generate RPC")
        # assert_raises_rpc_error(-32, 'The wallet generate rpc method is deprecated', self.nodes[0].rpc.generate, 1)
        # self.nodes[1].generate(1)

        if self.is_wallet_compiled():
            self.nodes[0].generate(101)
            self.test_feeRate_deprecation()
            self.test_bumpfee_privkey_deprecation()
        else:
            self.log.info("No tested deprecated RPC methods")

    def test_feeRate_deprecation(self):
        self.log.info("Test feeRate deprecation in RPCs fundrawtransaction and walletcreatefundedpsbt")
        inputs = []
        outputs = {self.nodes[0].getnewaddress() : 1}
        rawtx = self.nodes[1].createrawtransaction(inputs, outputs)

        # Node 0 with -deprecatedrpc=feeRate can still use deprecated feeRate option.
        self.nodes[0].fundrawtransaction(rawtx, {"feeRate": 0.0001})
        self.nodes[0].walletcreatefundedpsbt(inputs, outputs, 0, {"feeRate": 0.0001})

        # Node 1 cannot use deprecated feeRate option.
        msg = "The feeRate (BTC/kvB) option is deprecated and will be removed in 0.22. Use fee_rate (sat/vB) instead or restart bitcoind with -deprecatedrpc=feeRate."
        assert_raises_rpc_error(-32, msg, self.nodes[1].fundrawtransaction, rawtx, {"feeRate": 0.0001})
        assert_raises_rpc_error(-32, msg, self.nodes[1].walletcreatefundedpsbt, inputs, outputs, 0, {"feeRate": 0.1})

    def test_bumpfee_privkey_deprecation(self):
        self.log.info("Test bumpfee RPC")
        self.nodes[0].createwallet(wallet_name='nopriv', disable_private_keys=True)
        noprivs0 = self.nodes[0].get_wallet_rpc('nopriv')
        w0 = self.nodes[0].get_wallet_rpc(self.default_wallet_name)
        self.nodes[1].createwallet(wallet_name='nopriv', disable_private_keys=True)
        noprivs1 = self.nodes[1].get_wallet_rpc('nopriv')

        address = w0.getnewaddress()
        desc = w0.getaddressinfo(address)['desc']
        change_addr = w0.getrawchangeaddress()
        change_desc = w0.getaddressinfo(change_addr)['desc']
        txid = w0.sendtoaddress(address=address, amount=10)
        vout = find_vout_for_address(w0, txid, address)
        self.nodes[0].generate(1)
        rawtx = w0.createrawtransaction([{'txid': txid, 'vout': vout}], {w0.getnewaddress(): 5}, 0, True)
        rawtx = w0.fundrawtransaction(rawtx, {'changeAddress': change_addr})
        signed_tx = w0.signrawtransactionwithwallet(rawtx['hex'])['hex']

        noprivs0.importmulti([{'desc': desc, 'timestamp': 0}, {'desc': change_desc, 'timestamp': 0, 'internal': True}])
        noprivs1.importmulti([{'desc': desc, 'timestamp': 0}, {'desc': change_desc, 'timestamp': 0, 'internal': True}])

        txid = w0.sendrawtransaction(signed_tx)
        self.sync_all()

        assert_raises_rpc_error(-32, 'Using bumpfee with wallets that have private keys disabled is deprecated. Use psbtbumpfee instead or restart bitcoind with -deprecatedrpc=bumpfee. This functionality will be removed in 0.22', noprivs0.bumpfee, txid)
        bumped_psbt = noprivs1.bumpfee(txid)
        assert 'psbt' in bumped_psbt


if __name__ == '__main__':
    DeprecatedRpcTest().main()
