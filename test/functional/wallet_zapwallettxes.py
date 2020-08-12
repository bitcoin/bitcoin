#!/usr/bin/env python3
# Copyright (c) 2014-2018 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the zapwallettxes functionality.

- make two wallest
- create two transactions on zaptx wallet - one is confirmed and one is unconfirmed.
- run zapwallettxes, reload the wallet, and verify that both transactions are available
- run zapwallettxes, restart node 0 without persistmempool, and verify that the confirmed
  transactions are still available, but that the unconfirmed transaction has
  been zapped.
"""

import subprocess
import textwrap

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_raises_rpc_error,
)

class ZapWalletTXesTest (BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.extra_args = [["-keypool=1"]]
        self.num_nodes = 1

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()
        self.skip_if_no_wallet_tool()

    def bitcoin_wallet_process(self, args):
        binary = self.config["environment"]["BUILDDIR"] + '/src/bitcoin-wallet' + self.config["environment"]["EXEEXT"]
        args = ['-datadir={}'.format(self.nodes[0].datadir), '-chain=%s' % self.chain] + args
        return subprocess.Popen([binary] + args, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, universal_newlines=True)

    def assert_tool_output(self, output, args):
        p = self.bitcoin_wallet_process(args)
        stdout, stderr = p.communicate()
        assert_equal(stderr, '')
        assert_equal(stdout, output)
        assert_equal(p.poll(), 0)

    def assert_zapped(self, wallet, keep_meta=True):
        args = ["-wallet={}".format(wallet)]
        if not keep_meta:
            args.append("-keepmeta=0")
        if not self.nodes[0].is_node_stopped():
            self.nodes[0].unloadwallet(wallet)
        out = textwrap.dedent('''\
            Wallet info
            ===========
            Encrypted: no
            HD (hd seed available): yes
            Keypool Size: 2
            Transactions: 3
            Address Book: 2
        ''')
        self.assert_tool_output(out, args + ["info"])
        self.assert_tool_output("", args +["zapwallettxes"])
        out = textwrap.dedent('''\
            Wallet info
            ===========
            Encrypted: no
            HD (hd seed available): yes
            Keypool Size: 2
            Transactions: 0
            Address Book: 2
        ''')
        self.assert_tool_output(out, args + ['info'])
        if not self.nodes[0].is_node_stopped():
            self.nodes[0].loadwallet(wallet)

    def run_test(self):
        self.log.info("Mining blocks...")
        self.nodes[0].generate(101)
        self.sync_all()

        # Make a wallet that we will do the zapping on
        self.nodes[0].createwallet(wallet_name="zaptx")
        zaptx = self.nodes[0].get_wallet_rpc("zaptx")
        default = self.nodes[0].get_wallet_rpc("")

        default.sendtoaddress(zaptx.getnewaddress(), 10)
        self.nodes[0].generate(1)
        self.sync_all()

        # This transaction will be confirmed
        txid1 = zaptx.sendtoaddress(address=default.getnewaddress(), amount=10, comment="A comment", subtractfeefromamount=True)

        self.nodes[0].generate(1)
        self.sync_all()

        # This transaction will not be confirmed
        txid2 = default.sendtoaddress(zaptx.getnewaddress(), 20)
        zaptx.keypoolrefill()

        # Confirmed and unconfirmed transactions are the only transactions in the wallet.
        assert_equal(zaptx.gettransaction(txid1)['txid'], txid1)
        assert_equal(zaptx.gettransaction(txid2)['txid'], txid2)
        assert_equal(len(zaptx.listtransactions()), 3)

        # Zap normally. The wallet should be rescanned (both blockchain and mempool) on loading
        self.assert_zapped("zaptx")
        assert_equal(zaptx.gettransaction(txid1)['txid'], txid1)
        assert_equal(zaptx.gettransaction(txid2)['txid'], txid2)

        # The comment should be persisted
        assert_equal(zaptx.gettransaction(txid1)["comment"], "A comment")

        # Zap without keepmeta
        self.assert_zapped("zaptx", False)
        assert_equal(zaptx.gettransaction(txid1)['txid'], txid1)
        assert_equal(zaptx.gettransaction(txid2)['txid'], txid2)
        assert_equal("comment" not in zaptx.gettransaction(txid1), True)

        # Zap normally. Restart the node with -persismempool=0
        self.stop_node(0)
        self.assert_zapped("zaptx")
        self.start_node(0, ["-persistmempool=0", "-wallet=zaptx"])
        zaptx = self.nodes[0].get_wallet_rpc("zaptx")

        # tx1 is still be available because it was confirmed
        assert_equal(zaptx.gettransaction(txid1)['txid'], txid1)

        # This will raise an exception because the unconfirmed transaction has been zapped
        assert_raises_rpc_error(-5, 'Invalid or non-wallet transaction id', zaptx.gettransaction, txid2)

        self.log.info("Make sure -zapwallettxes gives an error")
        self.nodes[0].assert_start_raises_init_error(["-zapwallettxes"], "Error: -zapwallettxes has been replaced with the zapwallettxes command in the bitcoin-wallet tool. Please use that instead.")

if __name__ == '__main__':
    ZapWalletTXesTest().main()
