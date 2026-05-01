#!/usr/bin/env python3
# Copyright (c) 2026-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test MuSig2 descriptors in an external-signer wallet."""

from test_framework.descriptors import descsum_create
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal


# The device side of the MuSig2 setup.
DEVICE_ORIGIN = "[00000001/84h/1h/0h]"
DEVICE_XPUB = "tpubDCxzhZZE31g2EqSv1UajMAw5Hd62htydz9r2XBkrccHgBh8uw3n62zr6Zjmj64tfTk8Tjxo6VctjUMAh5DXWTErfQPC6RmQhTdtNnXuTXTQ"

# Hardcode the local MuSig participant's account xprv so the test can import a
# descriptor with one hot-wallet key and one external-signer key.
LOCAL_ORIGIN = "[ec63add0/84h/1h/0h]"
LOCAL_XPRV = "tprv8gauGKtmnH4cxv22ZtmEKqDDAeqnm2b4srPWuFsugMuVc79KDEHRTWgKGdAhACqjZQytU1o9gcc91TSW8L1s18PgFUHAJ8p8iY1GwaUEn9u"

# The hot wallet in this test does not contain an HD key. After
# bitcoin/bitcoin#29136 it could, and after bitcoin/bitcoin#32784 we could
# derive the 84h/1h/0h derivation xprv. A subsequent improvement to
# importdescriptors could avoid the need to handle xprvs, by recognizing
# an xpub for which it already has the matching private key material.

class WalletSignerMuSig2Test(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.extra_args = [
            [f"-signer={self.mock_signer_path()}", '-keypool=10'],
        ]

    def skip_test_if_missing_module(self):
        self.skip_if_no_external_signer()
        self.skip_if_no_wallet()

    def run_test(self):
        self.test_create_wallet()
        self.test_display_address()

    def test_create_wallet(self):
        self.log.info('Create an external-signer wallet with a MuSig2 descriptor')

        musig_descriptor = f"tr(musig({LOCAL_ORIGIN}{LOCAL_XPRV},{DEVICE_ORIGIN}{DEVICE_XPUB})/<0;1>/*)"

        # Blank wallet so createwallet doesn't import the device's single-sig
        # descriptors (which would clutter the wallet).
        self.nodes[0].createwallet(
            wallet_name='hww_musig',
            external_signer=True,
            disable_private_keys=False,
            blank=True,
        )
        hww_musig = self.nodes[0].get_wallet_rpc('hww_musig')

        result = hww_musig.importdescriptors([{
            "desc": descsum_create(musig_descriptor),
            "active": True,
            "timestamp": "now",
        }])
        assert_equal(result[0]["success"], True)

        # Reload so the imported descriptor is managed by
        # ExternalSignerScriptPubKeyMan rather than the plain
        # DescriptorScriptPubKeyMan it was added to on import.
        self.nodes[0].unloadwallet('hww_musig')
        self.nodes[0].loadwallet('hww_musig')

        descs = self.nodes[0].get_wallet_rpc('hww_musig').listdescriptors()["descriptors"]
        active_musig = [d for d in descs if d["active"] and d["desc"].startswith("tr(musig(")]
        # One active descriptor each for receive and change.
        assert_equal(len(active_musig), 2)

    def test_display_address(self):
        self.log.info('Display an address from the MuSig2 descriptor')
        hww_musig = self.nodes[0].get_wallet_rpc('hww_musig')

        addr = hww_musig.getnewaddress(address_type="bech32m")
        addr_info = hww_musig.getaddressinfo(addr)
        assert_equal(addr_info["ismine"], True)
        assert_equal(addr_info["solvable"], True)
        # This is not expected to work on a real device, which needs additional
        # information such as a BIP388 policy.
        assert_equal(hww_musig.walletdisplayaddress(addr), {"address": addr})


if __name__ == '__main__':
    WalletSignerMuSig2Test(__file__).main()
