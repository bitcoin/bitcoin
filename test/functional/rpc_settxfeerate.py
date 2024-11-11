#!/usr/bin/env python3
# Copyright (c) 2024 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the wallet changes the feerate configured properly when settxfee and settxfeerate is set."""

from decimal import Decimal
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_raises_rpc_error
)


class RPCSetTxFeeTest(BitcoinTestFramework):
    def add_options(self, parser):
        self.add_wallet_options(parser)

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def set_test_params(self):
        self.num_nodes = 2
        self.extra_args = [["-deprecatedrpc=settxfee"], []]
        self.supports_cli = False

    def run_test(self):
        node = self.nodes[0]

        self.log.debug("Test that settxfee is a hidden RPC")
        # It is hidden from general help, but its detailed help may be called directly.
        assert "settxfee " not in node.help() # An empty space is left after settxfee as if not would return True because of settxfeerate
        assert "settxfeerate" in node.help()
        assert "unknown command: settxfeerate" not in node.help("settxfeerate")
        assert "unknown command: settxfee" not in node.help("settxfee")

        self.log.debug("Test that settxfee set the feerate in B/kvB")
        assert_equal(node.settxfee(0.001), True)
        assert_equal(str(node.getwalletinfo()['paytxfee']), "0.00100000")

        self.log.debug("Test that settxfeerate set the feerate in v/vB")
        assert_equal(node.settxfeerate(6), True)
        assert_equal(str(node.getwalletinfo()['paytxfee']), "0.00006000")

        self.log.debug("Test that settxfee limit (0.1 B/kvB) can not be surpassed")
        assert_raises_rpc_error(-8, "txfee cannot be more than wallet max tx fee (0.10000000 BTC/kvB)", node.settxfee, 1)

        self.log.debug("Test that settxfeerate limit (10000 sat/vB) can not be surpassed")
        assert_raises_rpc_error(-8, "txfee cannot be more than wallet max tx fee (10000.000 sat/vB)", node.settxfeerate, 10001)

        self.log.debug("Test that settxfee minimum (0.00001 B/kvB) can not be ignored")
        assert_raises_rpc_error(-8, "txfee cannot be less than min relay tx fee (0.00001000 BTC/kvB)", node.settxfee, 0.000009)

        self.log.debug("Test that settxfeerate minimum (1 sat/vB) can not be ignored")
        assert_raises_rpc_error(-8, "txfee cannot be less than min relay tx fee (1.000 sat/vB)", node.settxfeerate, 0.99)

        self.log.debug("Test that the feerate set using settxfee is used when creating transactions")
        expected_feerate = 0.001
        node.settxfee(expected_feerate)
        txid = node.sendtoaddress(node.getnewaddress(), 10)
        fee = Decimal(node.gettransaction(txid)['fee'] * -1)
        vsize = Decimal(node.getrawtransaction(txid, 1)['vsize']/1000)
        feerate = Decimal(fee/vsize)
        feerate = feerate.quantize(Decimal("0.00000001"))
        expected_feerate = Decimal(expected_feerate).quantize(Decimal("0.00000001"))
        assert_equal(feerate, expected_feerate)

        self.log.debug("Test that the feerate set using settxfeerate is used when creating transactions")
        expected_feerate = 6
        node.settxfeerate(expected_feerate)
        txid = node.sendtoaddress(node.getnewaddress(), 10)
        fee = Decimal(node.gettransaction(txid)['fee'] * -100000000)
        vsize = Decimal(node.getrawtransaction(txid, 1)['vsize'])
        feerate = Decimal(fee/vsize)
        feerate = feerate.quantize(Decimal("0.00000001"))
        expected_feerate = Decimal(expected_feerate).quantize(Decimal("0.00000001"))

        self.log.debug("Test that settxfee is deprecated")
        assert_raises_rpc_error(-32, "Using settxfee is deprecated. Use settxfeerate instead or restart bitcoind with -deprecatedrpc=settxfee. This functionality will be removed in x.xx", self.nodes[1].settxfee, 0.05)


if __name__ == '__main__':
    RPCSetTxFeeTest(__file__).main()
