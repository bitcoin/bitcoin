#!/usr/bin/env python3
# Copyright (c) 2017-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test deprecation of RPC calls."""
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_raises_rpc_error


class DeprecatedRpcTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True
        self.extra_args = [[]]
        self.uses_wallet = None

    def run_test(self):
        # This test should be used to verify the errors of the currently
        # deprecated RPC methods (without the -deprecatedrpc flag) until
        # such RPCs are fully removed. For example:
        #
        # self.log.info("Test generate RPC")
        # assert_raises_rpc_error(-32, 'The wallet generate rpc method is deprecated', self.nodes[0].generate, 1)
        #
        # Please ensure that for all the RPC methods tested here, there is
        # at least one other functional test that still tests the RPCs
        # functionality using the respective -deprecatedrpc flag.

        # Please don't delete nor modify the comments and the boilerplate setup
        self.log.info("Tests for deprecated RPC methods (if any)")
        if self.is_wallet_compiled():
            self.log.info("Tests for deprecated wallet-related RPC methods (if any)")
            self.nodes[0].createwallet("wtest")
            wallet = self.nodes[0].get_wallet_rpc("wtest")  # Keep this setup, even if unused

            self.log.info("Test watchonly option deprecation")
            address = wallet.getnewaddress()
            rawtx = self.nodes[0].createrawtransaction([], [{address: 1}])
            def named_null(val):
                # TestNodeCLI omits None. Pass "null" to bitcoin-cli instead.
                return "null" if self.options.usecli and val is None else val

            deprecated_rpcs_watchonly = (
                lambda val: wallet.fundrawtransaction(rawtx, {"includeWatching": val}),
                lambda val: wallet.walletcreatefundedpsbt([], [{address: 1}], options={"includeWatching": val}),
                lambda val: wallet.send([{address: 1}], include_watching=named_null(val)),
                lambda val: wallet.sendall([address], include_watching=named_null(val)),
                lambda val: wallet.simulaterawtransaction([rawtx], include_watchonly=named_null(val)),
            )
            for val in (True, False, None):  # also check deprecation for None
                for rpc in deprecated_rpcs_watchonly:
                    assert_raises_rpc_error(-32, "deprecated no-op option", rpc, val)

            self.log.info("Test confTarget option deprecation")
            for rpc in (wallet.bumpfee, wallet.psbtbumpfee):
                assert_raises_rpc_error(-32, "start the RPC server with the `-deprecatedrpc=confTarget`", rpc, "0" * 64, {"confTarget": 1})

            self.log.info("Test encryptwallet deprecation")
            assert_raises_rpc_error(
                -32, "Start bitcoind with the `-deprecatedrpc=encryptwallet`",
                wallet.encryptwallet,
                "passphrase"
            )


if __name__ == '__main__':
    DeprecatedRpcTest(__file__).main()
