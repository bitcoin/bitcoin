#!/usr/bin/env python3
# Copyright (c) 2017-2018 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test external signer.

Verify that a bitcoind node can use an external signer command
"""
import os
import platform

from decimal import Decimal

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_raises_rpc_error
)


class SignerTest(BitcoinTestFramework):
    def mock_signer_path(self):
        path = os.path.join(os.path.dirname(os.path.realpath(__file__)), 'mocks', 'signer.py')
        if platform.system() == "Windows":
            return "python " + path
        else:
            return path

    def set_test_params(self):
        self.num_nodes = 5

        self.extra_args = [
            [],
            ['-signer=%s' % self.mock_signer_path() , '-addresstype=bech32', '-keypool=10'],
            ['-signer=%s' % self.mock_signer_path(), '-addresstype=p2sh-segwit', '-keypool=10'],
            ['-signer=%s' % self.mock_signer_path(), '-addresstype=legacy', '-keypool=10'],
            ['-signer=%s' % "fake.py"],
        ]

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def set_mock_result(self, node, res):
        f = open(os.path.join(node.cwd, "mock_result"), "w", encoding="utf8")
        f.write(res)
        f.close()

    def clear_mock_result(self, node):
        os.remove(os.path.join(node.cwd, "mock_result"))

    def run_test(self):
        self.skip_if_no_runcommand(self.nodes[0])
        self.log.info('-signer=%s' % self.mock_signer_path())
        assert_equal(self.nodes[0].getbalance(), 1250)
        assert_equal(self.nodes[1].getbalance(), 1250)
        assert_raises_rpc_error(-4, 'Error: restart bitcoind with -signer=<cmd>',
            self.nodes[0].enumeratesigners
        )

        # Handle script missing:
        assert_raises_rpc_error(-1, 'execve failed: No such file or directory',
            self.nodes[4].enumeratesigners
        )

        # Handle error thrown by script
        self.set_mock_result(self.nodes[1], "2")
        assert_raises_rpc_error(-1, 'Unable to parse JSON',
            self.nodes[1].enumeratesigners
        )
        self.clear_mock_result(self.nodes[1])

        # Create new wallets with private keys disabled:
        self.nodes[1].createwallet('hww1', True)
        hww1 = self.nodes[1].get_wallet_rpc('hww1')
        self.nodes[2].createwallet('hww2', True)
        hww2 = self.nodes[2].get_wallet_rpc('hww2')
        self.nodes[3].createwallet('hww3', True)
        hww3 = self.nodes[3].get_wallet_rpc('hww3')

        result = hww1.enumeratesigners()
        assert_equal(len(result['signers']), 2)
        hww2.enumeratesigners()
        # Delay enumerate on third wallet to test error handling
        # hww3.enumeratesigners()

        self.log.info('Test signerfetchkeys with bech32, p2sh-segwit and legacy')

        result = hww1.signerfetchkeys(0, "00000001")
        assert_equal(result, [{'success': True}, {'success': True}])
        assert_equal(hww1.getwalletinfo()["keypoolsize"], 10)

        # Handle error thrown by script
        self.set_mock_result(self.nodes[1], "2")
        assert_raises_rpc_error(-1, 'Unable to parse JSON',
            hww1.signerfetchkeys, 0, "00000001"
        )
        self.clear_mock_result(self.nodes[1])

        address1 = hww1.getnewaddress()
        assert_equal(address1, "bcrt1qm90ugl4d48jv8n6e5t9ln6t9zlpm5th68x4f8g")
        address_info = hww1.getaddressinfo(address1)
        assert_equal(address_info['solvable'], True)
        assert_equal(address_info['ismine'], False)
        assert_equal(address_info['hdkeypath'], "m/84'/1'/0'/0/0")

        assert_raises_rpc_error(-4, "First call enumeratesigners", hww3.signerfetchkeys)
        hww3.enumeratesigners()

        result = hww2.signerfetchkeys(0, "00000001")
        assert_equal(result, [{'success': True}, {'success': True}])
        assert_equal(hww2.getwalletinfo()["keypoolsize"], 10)

        address2 = hww2.getnewaddress()
        assert_equal(address2, "2N2gQKzjUe47gM8p1JZxaAkTcoHPXV6YyVp")
        address_info = hww2.getaddressinfo(address2)
        assert_equal(address_info['solvable'], True)
        assert_equal(address_info['ismine'], False)
        assert_equal(address_info['hdkeypath'], "m/49'/1'/0'/0/0")

        # signerfetchkeys range argument:
        assert_equal(hww2.getwalletinfo()["keypoolsize"], 9)
        for _ in range(9):
            hww2.getnewaddress()
        result = hww2.signerfetchkeys(0, "00000001", [10,19])
        assert_equal(hww2.getwalletinfo()["keypoolsize"], 10)

        address_info = hww2.getaddressinfo(hww2.getnewaddress())
        assert_equal(address_info['hdkeypath'], "m/49'/1'/0'/0/10")

        assert_raises_rpc_error(-4, "Multiple signers found, please specify which to use", hww3.signerfetchkeys)
        hww3.signerdissociate("00000002")
        hww3.signerfetchkeys()

        assert_equal(result, [{'success': True}, {'success': True}])
        assert_equal(hww3.getwalletinfo()["keypoolsize"], 10)

        address3 = hww3.getnewaddress("00000001")
        assert_equal(address3, "n1LKejAadN6hg2FrBXoU1KrwX4uK16mco9")
        address_info = hww3.getaddressinfo(address3)
        assert_equal(address_info['solvable'], True)
        assert_equal(address_info['ismine'], False)
        assert_equal(address_info['hdkeypath'], "m/44'/1'/0'/0/0")

        self.log.info('Test signerdisplayaddress')
        hww1.signerdisplayaddress(address1, "00000001")
        hww3.signerdisplayaddress(address3)

        # Handle error thrown by script
        self.set_mock_result(self.nodes[3], "2")
        assert_raises_rpc_error(-1, 'Unable to parse JSON',
            hww3.signerdisplayaddress, address3
        )
        self.clear_mock_result(self.nodes[3])

        self.log.info('Prepare mock PSBT')
        self.nodes[0].sendtoaddress(address1, 1)
        self.nodes[0].generate(1)
        self.sync_all()

        # Create mock PSBT for testing signerprocesspsbt and signersend.

        # Load private key into wallet to generate a signed PSBT for the mock
        self.nodes[1].createwallet(wallet_name="mock", disable_private_keys=False, blank=True)
        mock_wallet = self.nodes[1].get_wallet_rpc("mock")
        assert mock_wallet.getwalletinfo()['private_keys_enabled']

        result = mock_wallet.importmulti([{
            "desc": "wpkh([00000001/84'/1'/0']tprv8ZgxMBicQKsPd7Uf69XL1XwhmjHopUGep8GuEiJDZmbQz6o58LninorQAfcKZWARbtRtfnLcJ5MQ2AtHcQJCCRUcMRvmDUjyEmNUWwx8UbK/0/*)#rweraev0",
            "timestamp": "now",
            "range": [0,1],
            "internal": False,
            "keypool": False
        },
        {
            "desc": "wpkh([00000001/84'/1'/0']tprv8ZgxMBicQKsPd7Uf69XL1XwhmjHopUGep8GuEiJDZmbQz6o58LninorQAfcKZWARbtRtfnLcJ5MQ2AtHcQJCCRUcMRvmDUjyEmNUWwx8UbK/1/0)#0fe3a06x",
            "timestamp": "now",
            "internal": True,
            "keypool": False
        }])
        assert_equal(result[0], {'success': True})
        assert_equal(result[1], {'success': True})
        assert_equal(mock_wallet.getwalletinfo()["txcount"], 1)
        assert_equal(mock_wallet.getwalletinfo()["keypoolsize"], 0)
        change_address = mock_wallet.deriveaddresses("wpkh([00000001/84'/1'/0']tprv8ZgxMBicQKsPd7Uf69XL1XwhmjHopUGep8GuEiJDZmbQz6o58LninorQAfcKZWARbtRtfnLcJ5MQ2AtHcQJCCRUcMRvmDUjyEmNUWwx8UbK/1/0)#0fe3a06x")[0]
        dest = self.nodes[0].getnewaddress(address_type='bech32')
        mock_psbt = mock_wallet.walletcreatefundedpsbt([], {dest:0.5}, 0, {"includeWatching": True, "changeAddress": change_address}, True)['psbt']
        mock_psbt_signed = mock_wallet.walletprocesspsbt(psbt=mock_psbt, sign=True, sighashtype="ALL", bip32derivs=True)
        mock_psbt_final = mock_wallet.finalizepsbt(mock_psbt_signed["psbt"])
        mock_tx = mock_psbt_final["hex"]
        assert(mock_wallet.testmempoolaccept([mock_tx])[0]["allowed"])

        # Create two new wallets and populate with specific public keys, in order
        # to work with the mock signed PSBT.
        self.nodes[1].createwallet(wallet_name="hww4", disable_private_keys=True)
        hww4 = self.nodes[1].get_wallet_rpc("hww4")

        self.nodes[1].createwallet(wallet_name="hww5", disable_private_keys=True)
        hww5 = self.nodes[1].get_wallet_rpc("hww5")

        importmulti = [{
            "desc": "wpkh([00000001/84'/1'/0']tpubD6NzVbkrYhZ4WaWSyoBvQwbpLkojyoTZPRsgXELWz3Popb3qkjcJyJUGLnL4qHHoQvao8ESaAstxYSnhyswJ76uZPStJRJCTKvosUCJZL5B/0/*)#x30uthjs",
            "timestamp": "now",
            "range": [0, 1],
            "internal": False,
            "keypool": True,
            "watchonly": True
        },
        {
            "desc": "wpkh([00000001/84'/1'/0']tpubD6NzVbkrYhZ4WaWSyoBvQwbpLkojyoTZPRsgXELWz3Popb3qkjcJyJUGLnL4qHHoQvao8ESaAstxYSnhyswJ76uZPStJRJCTKvosUCJZL5B/1/0)#2k0wtpye",
            "timestamp": "now",
            "internal": True,
            "keypool": True,
            "watchonly": True
        }]

        result = hww4.importmulti(importmulti)
        assert_equal(result[0], {'success': True})
        assert_equal(result[1], {'success': True})
        assert_equal(hww4.getwalletinfo()["txcount"], 1)
        assert_equal(hww4.getwalletinfo()["keypoolsize"], 1)

        result = hww5.importmulti(importmulti)
        assert_equal(result[0], {'success': True})
        assert_equal(result[1], {'success': True})
        assert_equal(hww5.getwalletinfo()["txcount"], 1)
        assert_equal(hww5.getwalletinfo()["keypoolsize"], 1)

        assert(hww4.testmempoolaccept([mock_tx])[0]["allowed"])

        f = open(os.path.join(self.nodes[1].cwd, "mock_psbt"), "w")
        f.write(mock_psbt_signed["psbt"])
        f.close()

        self.log.info('Test signerprocesspsbt PSBT')

        hww4.enumeratesigners()
        psbt_orig = hww4.walletcreatefundedpsbt([], {dest:0.5}, 0, {"includeWatching": True}, True)['psbt']
        psbt_processed = hww4.signerprocesspsbt(psbt_orig, "00000001")
        assert_equal(psbt_processed['complete'], True)
        psbt_final = hww4.finalizepsbt(psbt_processed["psbt"])
        tx = psbt_final["hex"]
        assert_equal(tx, mock_tx)
        assert(hww4.testmempoolaccept([tx])[0]["allowed"])

        self.log.info('Test signersend')

        res = hww5.signersend([], {dest:0.5}, 0, None, "00000001")
        assert_equal(res['complete'], True)
        txid = res["txid"]

        self.sync_all()
        self.nodes[0].generate(1)
        node0_tx = self.nodes[0].gettransaction(txid)
        assert_equal(node0_tx['hex'], tx)
        assert_equal(self.nodes[0].getreceivedbyaddress(dest), Decimal("0.5"))

        # Handle error thrown by script
        self.set_mock_result(self.nodes[4], "2")
        assert_raises_rpc_error(-1, 'Unable to parse JSON',
            hww4.signerprocesspsbt, psbt_orig, "00000001"
        )
        self.clear_mock_result(self.nodes[4])

if __name__ == '__main__':
    SignerTest().main()
