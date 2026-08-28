#!/usr/bin/env python3
# Copyright (c) 2017-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test external signer.

Verify that a bitcoind node can use an external signer command
See also rpc_signer.py for tests without wallet context.
"""
import os
import sys

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_greater_than,
    assert_raises_rpc_error,
)


class WalletSignerTest(BitcoinTestFramework):
    def mock_signer_path(self):
        path = os.path.join(os.path.dirname(os.path.realpath(__file__)), 'mocks', 'signer.py')
        return sys.executable + " " + path

    def mock_no_connected_signer_path(self):
        path = os.path.join(os.path.dirname(os.path.realpath(__file__)), 'mocks', 'no_signer.py')
        return sys.executable + " " + path

    def mock_invalid_signer_path(self):
        path = os.path.join(os.path.dirname(os.path.realpath(__file__)), 'mocks', 'invalid_signer.py')
        return sys.executable + " " + path

    def mock_multi_signers_path(self):
        path = os.path.join(os.path.dirname(os.path.realpath(__file__)), 'mocks', 'multi_signers.py')
        return sys.executable + " " + path

    def set_test_params(self):
        self.num_nodes = 3

        self.extra_args = [
            [],
            [f"-signer={self.mock_signer_path()}", '-keypool=10'],
            # Node for the signer mock's wallet, kept offline so the mock
            # can't cheat by e.g. inspecting the UTXO set
            ["-maxconnections=0"],
        ]

    def setup_network(self):
        self.setup_nodes()
        # Leave the signer mock's node disconnected
        self.connect_nodes(0, 1)

    def sync_except_mock(self):
        """Sync all nodes except the signer mock's, which never receives blocks."""
        self.sync_all(self.nodes[0:2])

    def skip_test_if_missing_module(self):
        self.skip_if_no_external_signer()
        self.skip_if_no_wallet()

    def set_mock_result(self, node, res):
        with open(os.path.join(node.cwd, "mock_result"), "w") as f:
            f.write(res)

    def clear_mock_result(self, node):
        os.remove(os.path.join(node.cwd, "mock_result"))

    def init_mock_node(self):
        """Hand the signer mock its dedicated offline node, on which it
        creates the wallet it signs with."""
        signer_node = self.nodes[2]
        assert_equal(signer_node.getconnectioncount(), 0)
        with open(os.path.join(self.nodes[1].cwd, "mock_rpc_url"), "w") as f:
            f.write(signer_node.url)

    def run_test(self):
        self.init_mock_node()
        self.test_valid_signer()
        self.test_disconnected_signer()
        self.restart_node(1, [f"-signer={self.mock_invalid_signer_path()}", "-keypool=10"])
        self.test_invalid_signer()
        self.restart_node(1, [f"-signer={self.mock_multi_signers_path()}", "-keypool=10"])
        self.test_multiple_signers()

    def test_valid_signer(self):
        self.log.debug(f"-signer={self.mock_signer_path()}")

        # Create new wallets for an external signer.
        # disable_private_keys and descriptors must be true:
        assert_raises_rpc_error(-4, "Private keys must be disabled when using an external signer", self.nodes[1].createwallet, wallet_name='not_hww', disable_private_keys=False, external_signer=True)
        self.nodes[1].createwallet(wallet_name='hww', disable_private_keys=True, external_signer=True)
        hww = self.nodes[1].get_wallet_rpc('hww')
        assert_equal(hww.getwalletinfo()["external_signer"], True)

        # Flag can't be set afterwards (could be added later for non-blank descriptor based watch-only wallets)
        self.nodes[1].createwallet(wallet_name='not_hww', disable_private_keys=True, external_signer=False)
        not_hww = self.nodes[1].get_wallet_rpc('not_hww')
        assert_equal(not_hww.getwalletinfo()["external_signer"], False)
        assert_raises_rpc_error(-8, "Wallet flag is immutable: external_signer", not_hww.setwalletflag, "external_signer", True)


        self.set_mock_result(self.nodes[1], '0 {"invalid json"}')
        assert_raises_rpc_error(-1, 'Unable to parse JSON',
            self.nodes[1].createwallet, wallet_name='hww2', disable_private_keys=True, external_signer=True
        )
        self.clear_mock_result(self.nodes[1])

        assert_equal(hww.getwalletinfo()["keypoolsize"], 40)

        address1 = hww.getnewaddress(address_type="bech32")
        assert_equal(address1, "bcrt1qm90ugl4d48jv8n6e5t9ln6t9zlpm5th68x4f8g")
        address_info = hww.getaddressinfo(address1)
        assert_equal(address_info['solvable'], True)
        assert_equal(address_info['ismine'], True)
        assert_equal(address_info['hdkeypath'], "m/84h/1h/0h/0/0")

        address2 = hww.getnewaddress(address_type="p2sh-segwit")
        assert_equal(address2, "2N2gQKzjUe47gM8p1JZxaAkTcoHPXV6YyVp")
        address_info = hww.getaddressinfo(address2)
        assert_equal(address_info['solvable'], True)
        assert_equal(address_info['ismine'], True)
        assert_equal(address_info['hdkeypath'], "m/49h/1h/0h/0/0")

        address3 = hww.getnewaddress(address_type="legacy")
        assert_equal(address3, "n1LKejAadN6hg2FrBXoU1KrwX4uK16mco9")
        address_info = hww.getaddressinfo(address3)
        assert_equal(address_info['solvable'], True)
        assert_equal(address_info['ismine'], True)
        assert_equal(address_info['hdkeypath'], "m/44h/1h/0h/0/0")

        address4 = hww.getnewaddress(address_type="bech32m")
        assert_equal(address4, "bcrt1phw4cgpt6cd30kz9k4wkpwm872cdvhss29jga2xpmftelhqll62ms4e9sqj")
        address_info = hww.getaddressinfo(address4)
        assert_equal(address_info['solvable'], True)
        assert_equal(address_info['ismine'], True)
        assert_equal(address_info['hdkeypath'], "m/86h/1h/0h/0/0")

        self.log.info('Test walletdisplayaddress')
        for address in [address1, address2, address3]:
            result = hww.walletdisplayaddress(address)
            assert_equal(result, {"address": address})

        assert_raises_rpc_error(
            -4,
            "Error: sendtoaddress and sendmany are not supported for wallets with external signers; use send instead",
            hww.sendtoaddress,
            self.nodes[0].getnewaddress(),
            0.01,
        )
        assert_raises_rpc_error(
            -4,
            "Error: sendtoaddress and sendmany are not supported for wallets with external signers; use send instead",
            hww.sendmany,
            "",
            {self.nodes[0].getnewaddress(): 0.01},
        )

        # Handle error thrown by script
        self.set_mock_result(self.nodes[1], "2")
        assert_raises_rpc_error(-1, 'RunCommandParseJSON error',
            hww.walletdisplayaddress, address1
        )
        self.clear_mock_result(self.nodes[1])

        # Returned address MUST match:
        address_fail = hww.getnewaddress(address_type="bech32")
        assert_equal(address_fail, "bcrt1ql7zg7ukh3dwr25ex2zn9jse926f27xy2jz58tm")
        assert_raises_rpc_error(-1, 'Signer echoed unexpected address wrong_address',
            hww.walletdisplayaddress, address_fail
        )

        self.log.info('Fund hww wallet')
        for address in [address1, address2, address3, address4]:
            self.nodes[0].sendtoaddress(address, 1)
        self.generate(self.nodes[0], 1, sync_fun=self.sync_except_mock)
        assert_equal(hww.getwalletinfo()["txcount"], 4)

        dest = self.nodes[0].getnewaddress(address_type='bech32')

        self.log.info('Test send using hww1')

        # Spend all four address types at once. Don't broadcast the transaction
        # yet so the RPC returns the raw hex.
        res = hww.send(outputs={dest:3.5}, add_to_wallet=False)
        assert res["complete"]
        assert_equal(len(hww.decoderawtransaction(res["hex"])["vin"]), 4)
        assert hww.testmempoolaccept([res["hex"]])[0]["allowed"]

        self.log.info('Test sendall using hww1')

        res = hww.sendall(recipients=[{dest:3.5}, hww.getrawchangeaddress()], add_to_wallet=False)
        assert res["complete"]
        assert hww.testmempoolaccept([res["hex"]])[0]["allowed"]
        # Broadcast transaction so we can bump the fee
        hww.sendrawtransaction(res["hex"])

        self.log.info('Test bumpfee using hww1')

        orig_tx_id = res["txid"]
        res = hww.bumpfee(orig_tx_id)
        assert_greater_than(res["fee"], res["origfee"])
        assert_equal(res["errors"], [])


    def test_disconnected_signer(self):
        self.log.info('Test disconnected external signer')

        # First create a wallet with the signer connected
        self.nodes[1].createwallet(wallet_name='hww_disconnect', disable_private_keys=True, external_signer=True)
        hww = self.nodes[1].get_wallet_rpc('hww_disconnect')
        assert_equal(hww.getwalletinfo()["external_signer"], True)

        # Fund wallet
        self.nodes[0].sendtoaddress(hww.getnewaddress(address_type="bech32m"), 1)
        self.generate(self.nodes[0], 1, sync_fun=self.sync_except_mock)

        # Restart node with no signer connected
        self.log.debug(f"-signer={self.mock_no_connected_signer_path()}")
        self.restart_node(1, [f"-signer={self.mock_no_connected_signer_path()}", "-keypool=10"])
        self.nodes[1].loadwallet('hww_disconnect')
        hww = self.nodes[1].get_wallet_rpc('hww_disconnect')

        # Try to spend
        dest = hww.getrawchangeaddress()
        assert_raises_rpc_error(-25, "External signer not found", hww.send, outputs=[{dest:0.5}])

    def test_invalid_signer(self):
        self.log.debug(f"-signer={self.mock_invalid_signer_path()}")
        self.log.info('Test invalid external signer')
        assert_raises_rpc_error(-1, "Invalid descriptor", self.nodes[1].createwallet, wallet_name='hww_invalid', disable_private_keys=True, external_signer=True)

    def test_multiple_signers(self):
        self.log.debug(f"-signer={self.mock_multi_signers_path()}")
        self.log.info('Test multiple external signers')

        assert_raises_rpc_error(-1, "More than one external signer found", self.nodes[1].createwallet, wallet_name='multi_hww', disable_private_keys=True, external_signer=True)

if __name__ == '__main__':
    WalletSignerTest(__file__).main()
