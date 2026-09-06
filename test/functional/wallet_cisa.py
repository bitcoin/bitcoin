#!/usr/bin/env python3
# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test cisa() descriptors in the wallet: address generation and receiving."""

from test_framework.descriptors import descsum_create
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal


class WalletCisaTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        node = self.nodes[0]
        node.createwallet("cisa")
        wallet = node.get_wallet_rpc("cisa")
        self.generatetoaddress(node, 101, wallet.getnewaddress())

        tprv = "tprv8ZgxMBicQKsPd7Uf69XL1XwhmjHopUGep8GuEiJDZmbQz6o58LninorQAfcKZWARbtRtfnLcJ5MQ2AtHcQJCCRUcMRvmDUjyEmNUWwx8UbK"
        desc = descsum_create(f"cisa({tprv}/86h/1h/0h/<0;1>/*)")
        assert_equal(wallet.importdescriptors([{"desc": desc, "active": True, "timestamp": "now"}])[0]["success"], True)

        # An active cisa() descriptor provides the bech32m addresses
        addr = wallet.getnewaddress("", "bech32m")
        info = wallet.getaddressinfo(addr)
        assert_equal(info["witness_version"], 2)
        assert_equal(info["ismine"], True)
        assert info["desc"].startswith("cisa([")
        assert_equal(wallet.getaddressinfo(wallet.getrawchangeaddress("bech32m"))["witness_version"], 2)

        txid = wallet.sendtoaddress(addr, 1)
        self.generate(node, 1)
        utxo, = [u for u in wallet.listunspent() if u["address"] == addr]
        assert_equal(utxo["txid"], txid)
        assert_equal(utxo["desc"], info["desc"])


if __name__ == "__main__":
    WalletCisaTest(__file__).main()
