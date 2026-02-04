#!/usr/bin/env python3
# Copyright (c) 2022-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the sendmany RPC command."""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_greater_than_or_equal,
    assert_raises_rpc_error,
)


class SendmanyTest(BitcoinTestFramework):
    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True

    def test_sffo_repeated_address(self):
        addr_1 = self.wallet.getnewaddress()
        addr_2 = self.wallet.getnewaddress()
        addr_3 = self.wallet.getnewaddress()

        self.log.info("Test using duplicate address in SFFO argument")
        assert_raises_rpc_error(-8, "Invalid parameter 'subtract fee from output', duplicated position: 0", self.def_wallet.sendmany, dummy='', amounts={addr_1: 1, addr_2: 1}, subtractfeefrom=[addr_1, addr_1, addr_1])
        self.log.info("Test using address not present in tx.vout in SFFO argument")
        assert_raises_rpc_error(-8, f"Invalid parameter 'subtract fee from output', destination {addr_3} not found in tx outputs", self.def_wallet.sendmany, dummy='', amounts={addr_1: 1, addr_2: 1}, subtractfeefrom=[addr_3])
        self.log.info("Test using negative index in SFFO argument")
        assert_raises_rpc_error(-8, "Invalid parameter 'subtract fee from output', negative position: -5", self.def_wallet.sendmany, dummy='', amounts={addr_1: 1, addr_2: 1}, subtractfeefrom=[-5])
        self.log.info("Test using an out of bounds index in SFFO argument")
        assert_raises_rpc_error(-8, "Invalid parameter 'subtract fee from output', position too large: 5", self.def_wallet.sendmany, dummy='', amounts={addr_1: 1, addr_2: 1}, subtractfeefrom=[5])
        self.log.info("Test using an unexpected type in SFFO argument")
        assert_raises_rpc_error(-8, "Invalid parameter 'subtract fee from output', invalid value type: bool", self.def_wallet.sendmany, dummy='', amounts={addr_1: 1, addr_2: 1}, subtractfeefrom=[False])
        self.log.info("Test duplicates in SFFO argument, mix string destinations with numeric indexes")
        assert_raises_rpc_error(-8, "Invalid parameter 'subtract fee from output', duplicated position: 0", self.def_wallet.sendmany, dummy='', amounts={addr_1: 1, addr_2: 1}, subtractfeefrom=[0, addr_1])
        self.log.info("Test valid mixing of string destinations with numeric indexes in SFFO argument")
        self.def_wallet.sendmany(dummy='', amounts={addr_1: 1, addr_2: 1}, subtractfeefrom=[0, addr_2])

    def test_anti_fee_sniping(self):
        self.log.info('Test that sendmany sets the transaction locktime no lower than any input locktime')
        self.generate(self.nodes[0], 250)
        wallet_rpc = self.nodes[0].get_wallet_rpc(self.default_wallet_name)
        self.nodes[0].createwallet("anti_fee_sniping_test")
        test_wallet = self.nodes[0].get_wallet_rpc("anti_fee_sniping_test")
        # Create 100 parent transactions, all with the same
        # explicit backdated nLockTime. This avoids relying on the random
        # anti fee sniping behavior and provides several inputs to spend from.
        num_parents = 100
        tip_height = self.nodes[0].getblockcount()
        parent_locktime = tip_height - 80
        for _ in range(num_parents):
            wallet_rpc.send(outputs=[{test_wallet.getnewaddress(): 10}], locktime=parent_locktime)
        self.generate(self.nodes[0], 5)
        tip_height = self.nodes[0].getblockcount()
        # Spend parents one by one until we observe a backdated
        # nLockTime on a child transaction. Backdating happens randomly, so
        # retry until the locktime is less than the current block height.
        child_nlocktime = tip_height
        num_tries = 0
        while child_nlocktime == tip_height:
            child_txid = test_wallet.sendmany("", {wallet_rpc.getnewaddress(): 6, wallet_rpc.getnewaddress(): 3})
            child_nlocktime = test_wallet.gettransaction(txid=child_txid, verbose=True)["decoded"]["locktime"]
            num_tries += 1
            if num_tries >= num_parents:
                raise AssertionError("RPC call sendmany() did not produce a backdated nLockTime")
        # Child nlocktime should be >= parent nlocktime
        assert_greater_than_or_equal(child_nlocktime, parent_locktime)
        test_wallet.unloadwallet()

    def run_test(self):
        self.nodes[0].createwallet("activewallet")
        self.wallet = self.nodes[0].get_wallet_rpc("activewallet")
        self.def_wallet = self.nodes[0].get_wallet_rpc(self.default_wallet_name)
        self.generate(self.nodes[0], 101)

        self.test_sffo_repeated_address()
        self.test_anti_fee_sniping()


if __name__ == '__main__':
    SendmanyTest(__file__).main()
