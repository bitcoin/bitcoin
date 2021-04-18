#!/usr/bin/env python3
# Copyright (c) 2017-2019 The Widecoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test deprecation of RPC calls."""
from test_framework.test_framework import WidecoinTestFramework
from test_framework.util import assert_raises_rpc_error, find_vout_for_address

class DeprecatedRpcTest(WidecoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 2
        self.setup_clean_chain = True
        self.extra_args = [[], ['-deprecatedrpc=bumpfee']]

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
            self.log.info("Test bumpfee RPC")
            self.nodes[0].generate(101)
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

            assert_raises_rpc_error(-32, 'Using bumpfee with wallets that have private keys disabled is deprecated. Use psbtbumpfee instead or restart widecoind with -deprecatedrpc=bumpfee. This functionality will be removed in 0.22', noprivs0.bumpfee, txid)
            bumped_psbt = noprivs1.bumpfee(txid)
            assert 'psbt' in bumped_psbt
        else:
            self.log.info("No tested deprecated RPC methods")

if __name__ == '__main__':
    DeprecatedRpcTest().main()
