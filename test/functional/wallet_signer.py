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
from decimal import Decimal

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
        self.num_nodes = 2

        self.extra_args = [
            [],
            [f"-signer={self.mock_signer_path()}", '-keypool=10'],
        ]

    def skip_test_if_missing_module(self):
        self.skip_if_no_external_signer()
        self.skip_if_no_wallet()

    def set_mock_result(self, node, res):
        with open(os.path.join(node.cwd, "mock_result"), "w") as f:
            f.write(res)

    def clear_mock_result(self, node):
        os.remove(os.path.join(node.cwd, "mock_result"))

    def run_test(self):
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

        self.log.info('Prepare mock PSBT')
        self.nodes[0].sendtoaddress(address4, 1)
        self.generate(self.nodes[0], 1)

        # Load private key into wallet to generate a signed PSBT for the mock
        self.nodes[1].createwallet(wallet_name="mock", disable_private_keys=False, blank=True)
        mock_wallet = self.nodes[1].get_wallet_rpc("mock")
        assert mock_wallet.getwalletinfo()['private_keys_enabled']

        result = mock_wallet.importdescriptors([{
            "desc": "tr([00000001/86h/1h/0']tprv8ZgxMBicQKsPd7Uf69XL1XwhmjHopUGep8GuEiJDZmbQz6o58LninorQAfcKZWARbtRtfnLcJ5MQ2AtHcQJCCRUcMRvmDUjyEmNUWwx8UbK/0/*)#7ew68cn8",
            "timestamp": 0,
            "range": [0,1],
            "internal": False,
            "active": True
        },
        {
            "desc": "tr([00000001/86h/1h/0']tprv8ZgxMBicQKsPd7Uf69XL1XwhmjHopUGep8GuEiJDZmbQz6o58LninorQAfcKZWARbtRtfnLcJ5MQ2AtHcQJCCRUcMRvmDUjyEmNUWwx8UbK/1/*)#0dtm6drl",
            "timestamp": 0,
            "range": [0, 0],
            "internal": True,
            "active": True
        }])
        assert_equal(result[0], {'success': True})
        assert_equal(result[1], {'success': True})
        assert_equal(mock_wallet.getwalletinfo()["txcount"], 1)
        dest = self.nodes[0].getnewaddress(address_type='bech32')

        # ExternalSigner::SignTransaction rejects a reply whose outputs differ
        # from the PSBT it was given, so the pre-signed mock_psbt must be the
        # signed form of the exact PSBT hww builds. Have hww build the PSBT,
        # then have mock_wallet (which holds the matching private keys) sign it.
        # Pin change_position and change_address so hww.send rebuilds an
        # identical PSBT when the signer is called.
        change_addr = hww.getrawchangeaddress()
        mock_psbt = hww.walletcreatefundedpsbt(
            [], {dest: 0.5}, 0,
            {'replaceable': True, 'change_position': 1, 'change_address': change_addr},
            True,
        )['psbt']
        mock_psbt_signed = mock_wallet.walletprocesspsbt(psbt=mock_psbt, sign=True, sighashtype="ALL", bip32derivs=True)
        mock_tx = mock_psbt_signed["hex"]
        assert mock_wallet.testmempoolaccept([mock_tx])[0]["allowed"]

        assert_equal(hww.getwalletinfo()["txcount"], 1)

        assert hww.testmempoolaccept([mock_tx])[0]["allowed"]

        with open(os.path.join(self.nodes[1].cwd, "mock_psbt"), "w") as f:
            f.write(mock_psbt_signed["psbt"])

        self.log.info('Test send using hww1')

        # Don't broadcast transaction yet so the RPC returns the raw hex
        res = hww.send(
            outputs={dest: 0.5},
            options={'add_to_wallet': False, 'change_position': 1, 'change_address': change_addr},
        )
        assert res["complete"]
        assert_equal(res["hex"], mock_tx)

        self.log.info('Test sendall using hww1')

        sendall_change = hww.getrawchangeaddress()
        utxos = hww.listunspent()
        assert_equal(len(utxos), 1)
        utxo = utxos[0]
        sendall_psbt = hww.walletcreatefundedpsbt(
            [{"txid": utxo["txid"], "vout": utxo["vout"]}],
            [{dest: 0.5}, {sendall_change: utxo["amount"] - Decimal("0.5")}],
            0,
            {'add_inputs': False, 'subtractFeeFromOutputs': [1], 'replaceable': True},
            True,
        )['psbt']
        sendall_psbt_signed = mock_wallet.walletprocesspsbt(psbt=sendall_psbt, sign=True, sighashtype="ALL", bip32derivs=True)
        sendall_tx = sendall_psbt_signed["hex"]

        with open(os.path.join(self.nodes[1].cwd, "mock_psbt"), "w") as f:
            f.write(sendall_psbt_signed["psbt"])

        res = hww.sendall(recipients=[{dest: 0.5}, sendall_change], add_to_wallet=False)
        assert res["complete"]
        assert_equal(res["hex"], sendall_tx)
        # Broadcast transaction so we can bump the fee
        hww.sendrawtransaction(res["hex"])

        self.log.info('Prepare fee bumped mock PSBT')

        # Build the bumped PSBT on hww (not mock_wallet) so it matches what
        # the external signer will receive; mock_wallet only adds signatures.
        orig_tx_id = res["txid"]
        hww_psbt_bumped = hww.psbtbumpfee(orig_tx_id)["psbt"]
        mock_psbt_bumped_signed = mock_wallet.walletprocesspsbt(psbt=hww_psbt_bumped, sign=True, sighashtype="ALL", bip32derivs=True)

        with open(os.path.join(self.nodes[1].cwd, "mock_psbt"), "w") as f:
            f.write(mock_psbt_bumped_signed["psbt"])

        self.log.info('Test bumpfee using hww1')

        # hww.bumpfee rebuilds the bumped tx internally with a random
        # change_position (CreateTransaction is called with change_pos=nullopt
        # in feebumper.cpp), so its outputs would not line up with the
        # pre-signed mock_psbt. Send the exact pre-built PSBT through
        # walletprocesspsbt, which routes to the external signer.
        signed_bumped = hww.walletprocesspsbt(psbt=hww_psbt_bumped)
        assert signed_bumped["complete"]
        final_bumped = hww.finalizepsbt(signed_bumped["psbt"])
        assert final_bumped["complete"]
        assert hww.testmempoolaccept([final_bumped["hex"]])[0]["allowed"]
        bumped_decoded = hww.decoderawtransaction(final_bumped["hex"])
        orig_decoded = hww.gettransaction(orig_tx_id, True, True)["decoded"]
        bumped_out_total = sum(vout["value"] for vout in bumped_decoded["vout"])
        orig_out_total = sum(vout["value"] for vout in orig_decoded["vout"])
        # 1 BTC input on both sides, so a smaller output total means a higher fee.
        assert_greater_than(orig_out_total, bumped_out_total)


    def test_disconnected_signer(self):
        self.log.info('Test disconnected external signer')

        # First create a wallet with the signer connected
        self.nodes[1].createwallet(wallet_name='hww_disconnect', disable_private_keys=True, external_signer=True)
        hww = self.nodes[1].get_wallet_rpc('hww_disconnect')
        assert_equal(hww.getwalletinfo()["external_signer"], True)

        # Fund wallet
        self.nodes[0].sendtoaddress(hww.getnewaddress(address_type="bech32m"), 1)
        self.generate(self.nodes[0], 1)

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
