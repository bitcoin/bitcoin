#!/usr/bin/env python3

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_raises_rpc_error,
)


class SilentPaymentsReceivingTest(BitcoinTestFramework):
    def add_options(self, parser):
        self.add_wallet_options(parser, legacy=False)

    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()
        self.skip_if_no_sqlite()

    def test_createwallet(self):
        self.log.info("Check createwallet silent payments option")

        self.nodes[0].createwallet(wallet_name="sp", silent_payment=True)
        wallet = self.nodes[0].get_wallet_rpc("sp")
        addr = wallet.getnewaddress(address_type="silent-payment")
        assert addr.startswith("sp")
        addr_again = wallet.getnewaddress(address_type="silent-payment")
        assert_equal(addr, addr_again)

        self.nodes[0].createwallet(wallet_name="non_sp", silent_payment=False)
        wallet = self.nodes[0].get_wallet_rpc("non_sp")
        assert_raises_rpc_error(-12, "Error: No silent-payment addresses available", wallet.getnewaddress, address_type="silent-payment")

        if self.is_bdb_compiled():
            assert_raises_rpc_error(-4, "Wallet with silent payments must also be a descriptor wallet", self.nodes[0].createwallet, wallet_name="legacy_sp", descriptors=False, silent_payment=True)

            self.nodes[0].createwallet(wallet_name="legacy_sp", descriptors=False)
            wallet = self.nodes[0].get_wallet_rpc("legacy_sp")
            assert_raises_rpc_error(-12, "Error: No silent-payment addresses available", wallet.getnewaddress, address_type="silent-payment")

    def test_basic(self):
        self.log.info("Basic receive and send")

        self.nodes[0].createwallet(wallet_name="basic", silent_payment=True)
        wallet = self.nodes[0].get_wallet_rpc("basic")

        addr = wallet.getnewaddress(address_type="silent-payment")
        txid = self.def_wallet.sendtoaddress(addr, 10)
        self.generate(self.nodes[0], 1)

        assert_equal(wallet.getbalance(), 10)
        wallet.gettransaction(txid)

        wallet.sendall([self.def_wallet.getnewaddress()])
        self.generate(self.nodes[0], 1)

        assert_equal(wallet.getbalance(), 0)

    def run_test(self):
        self.def_wallet = self.nodes[0].get_wallet_rpc(self.default_wallet_name)
        self.generate(self.nodes[0], 101)

        self.test_createwallet()
        self.test_basic()


if __name__ == '__main__':
    SilentPaymentsReceivingTest().main()
