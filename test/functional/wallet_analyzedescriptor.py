#!/usr/bin/env python3
# Copyright (c) 2026-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the analyzedescriptor RPC."""

from test_framework.descriptors import descsum_create
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_raises_rpc_error,
)
from test_framework.wallet_util import generate_keypair

MULTIPATH = "/<0;1>/*"
# A private key that none of the cosigner wallets hold.
TPRV = "tprv8ZgxMBicQKsPd7Uf69XL1XwhmjHopUGep8GuEiJDZmbQz6o58LninorQAfcKZWARbtRtfnLcJ5MQ2AtHcQJCCRUcMRvmDUjyEmNUWwx8UbK"


class WalletAnalyzeDescriptorTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.wallet_names = []

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def create_cosigner(self, name):
        """Create a wallet with a new HD key, and return it with its cosigner key expression."""
        self.nodes[0].createwallet(wallet_name=name, blank=True)
        wallet = self.nodes[0].get_wallet_rpc(name)
        wallet.addhdkey()
        key = wallet.derivehdkey("m/48h/1h/0h/2h")
        return wallet, f"{key['origin']}{key['xpub']}"

    def wallet_matches(self, wallet, desc):
        return [key["wallet"] for key in wallet.analyzedescriptor(desc)["keys"]]

    def test_errors(self):
        node = self.nodes[0]
        self.log.info("Test that analyzedescriptor needs a wallet")
        desc = descsum_create(f"wpkh({self.keys[0]}/0/*)")
        node.unloadwallet("Alice")
        node.unloadwallet("Bob")
        node.unloadwallet("Carol")
        assert_raises_rpc_error(-18, "No wallet is loaded", node.analyzedescriptor, desc)
        node.loadwallet("Alice")
        node.loadwallet("Bob")
        assert_raises_rpc_error(-19, "Multiple wallets are loaded", node.analyzedescriptor, desc)
        node.loadwallet("Carol")
        assert_raises_rpc_error(-5, "is not a valid descriptor function", self.alice.analyzedescriptor, "foo()")

    def test_cosigners(self):
        node = self.nodes[0]
        self.log.info("Test that each cosigner finds its own slot")
        desc = descsum_create(f"wsh(sortedmulti(2,{','.join(key + MULTIPATH for key in self.keys)}))")
        info = node.getdescriptorinfo(desc, True)
        for i, wallet in enumerate(self.cosigners):
            result = wallet.analyzedescriptor(desc)
            assert_equal([key["wallet"] for key in result["keys"]], ["mine" if j == i else "not_mine" for j in range(3)])
            assert_equal(result["can_sign"], True)
            # The same keys and analysis as getdescriptorinfo, which needs no wallet.
            assert_equal([{k: v for k, v in key.items() if k != "wallet"} for key in result["keys"]], info["keys"])
            assert_equal(result["analysis"], info["analysis"])
            assert_equal(result["first_address"], node.deriveaddresses(desc, [0, 0])[0][0])

        outsider, _ = self.create_cosigner("Dave")
        assert_equal(self.wallet_matches(outsider, desc), ["not_mine"] * 3)
        assert_equal(outsider.analyzedescriptor(desc)["can_sign"], False)

        self.log.info("Test MuSig participants and Taproot leaf keys")
        leaf = f"sortedmulti_a(2,{','.join(key + MULTIPATH for key in self.keys)})"
        desc = descsum_create(f"tr(musig({','.join(self.keys)}){MULTIPATH},{leaf})")
        # The musig() aggregate itself is never matched, only its participants.
        assert_equal(self.wallet_matches(self.alice, desc), ["not_mine", "mine", "not_mine", "not_mine", "mine", "not_mine", "not_mine"])

        self.log.info("Test that the wallet can sign for an unrecognized setup it holds a key of")
        desc = descsum_create(f"wsh(sortedmulti(2,{self.keys[0]}{MULTIPATH},{self.keys[0]}{MULTIPATH},{self.keys[1]}{MULTIPATH}))")
        result = self.alice.analyzedescriptor(desc)
        assert_equal(result["analysis"]["recognized"], False)
        assert_equal([key["wallet"] for key in result["keys"]], ["mine", "mine", "not_mine"])
        assert_equal(result["can_sign"], True)

        self.log.info("Test a single-sig descriptor")
        desc = descsum_create(f"wpkh({self.keys[0]}/0/*)")
        result = self.alice.analyzedescriptor(desc)
        assert_equal(result["analysis"], {"recognized": False, "issues": [{"type": "unsupported_shape", "keys": []}]})
        assert_equal([key["wallet"] for key in result["keys"]], ["mine"])
        assert_equal(result["can_sign"], True)
        assert_equal(result["first_address"], self.nodes[0].deriveaddresses(desc, [0, 0])[0])
        assert_equal(self.cosigners[1].analyzedescriptor(desc)["can_sign"], False)

        self.log.info("Test that private keys in the descriptor derive the first address")
        desc = descsum_create(f"wpkh({TPRV}/84h/1h/0h/0/*)")
        assert_equal(self.alice.analyzedescriptor(desc)["first_address"], self.nodes[0].deriveaddresses(desc, [0, 0])[0])

    def test_locked_wallet(self):
        self.log.info("Test that a locked wallet reports its cosigner key as unknown")
        desc = descsum_create(f"wsh(sortedmulti(2,{','.join(key + MULTIPATH for key in self.keys)}))")
        self.alice.encryptwallet("passphrase")
        result = self.alice.analyzedescriptor(desc)
        assert_equal([key["wallet"] for key in result["keys"]], ["unknown", "not_mine", "not_mine"])
        assert "can_sign" not in result
        # Everything that needs no private key is still returned.
        assert_equal(result["analysis"]["recognized"], True)
        assert "first_address" in result

        self.alice.walletpassphrase("passphrase", 60)
        assert_equal(self.wallet_matches(self.alice, desc), ["mine", "not_mine", "not_mine"])
        self.alice.walletlock()

    def test_generated_address(self):
        self.log.info("Test a public key from one of the wallet's own addresses")
        node = self.nodes[0]
        node.createwallet(wallet_name="Erin")
        erin = node.get_wallet_rpc("Erin")
        info = erin.getaddressinfo(erin.getnewaddress(address_type="bech32"))
        other = "02c6047f9441ed7d6d3045406e95c07cd85c778e4b8cef3ca7abac09b95c709ee5"
        desc = descsum_create(f"wsh(multi(1,{info['pubkey']},{other}))")
        result = erin.analyzedescriptor(desc)
        assert_equal([key["wallet"] for key in result["keys"]], ["mine", "not_mine"])
        assert_equal(result["can_sign"], True)
        # Also with the origin that getaddressinfo shows.
        assert_equal(self.wallet_matches(erin, info["desc"]), ["mine"])
        assert_equal(self.wallet_matches(self.cosigners[1], desc), ["not_mine", "not_mine"])

    def test_musig_only(self):
        self.log.info("Test a key the wallet holds only as a musig() participant")
        node = self.nodes[0]
        node.createwallet(wallet_name="Frank", blank=True)
        frank = node.get_wallet_rpc("Frank")
        private = descsum_create(f"tr(musig({TPRV},{self.keys[1]}){MULTIPATH})")
        assert_equal(frank.importdescriptors([{"desc": private, "timestamp": "now"}])[0]["success"], True)
        # gethdkeys doesn't list musig() participants.
        assert_equal(frank.gethdkeys(), [])
        result = frank.analyzedescriptor(node.getdescriptorinfo(private)["descriptor"])
        assert_equal([key["wallet"] for key in result["keys"]], ["not_mine", "mine", "watch_only"])
        assert_equal(result["can_sign"], True)

    def test_taproot_parity(self):
        self.log.info("Test that a Taproot key matches with either prefix, an ECDSA key only exactly")
        node = self.nodes[0]
        node.createwallet(wallet_name="Grace", blank=True)
        grace = node.get_wallet_rpc("Grace")
        # A private key whose public key has an odd Y (03 prefix).
        while True:
            wif, pubkey = generate_keypair(wif=True)
            if pubkey[0] == 3:
                break
        assert_equal(grace.importdescriptors([{"desc": descsum_create(f"wpkh({wif})"), "timestamp": "now"}])[0]["success"], True)
        x = pubkey[1:].hex()
        # Taproot uses only x: both prefixes are the wallet's key.
        assert_equal(self.wallet_matches(grace, descsum_create(f"tr(02{x})")), ["mine"])
        assert_equal(self.wallet_matches(grace, descsum_create(f"tr(03{x})")), ["mine"])
        # ECDSA uses the full key: the other prefix is a different key.
        assert_equal(self.wallet_matches(grace, descsum_create(f"wsh(multi(1,02{x}))")), ["not_mine"])
        assert_equal(self.wallet_matches(grace, descsum_create(f"wsh(multi(1,03{x}))")), ["mine"])

    def test_watch_only(self):
        self.log.info("Test a watch-only wallet with the multisig descriptor")
        node = self.nodes[0]
        node.createwallet(wallet_name="Watcher", disable_private_keys=True)
        watcher = node.get_wallet_rpc("Watcher")
        desc = descsum_create(f"wsh(sortedmulti(2,{','.join(key + MULTIPATH for key in self.keys)}))")
        assert_equal(watcher.importdescriptors([{"desc": desc, "timestamp": "now"}])[0]["success"], True)
        result = watcher.analyzedescriptor(desc)
        assert_equal([key["wallet"] for key in result["keys"]], ["watch_only"] * 3)
        assert_equal(result["can_sign"], False)

    def test_disabled_wallet(self):
        self.log.info("Test that analyzedescriptor is not available with -disablewallet")
        self.restart_node(0, extra_args=["-disablewallet"])
        assert_raises_rpc_error(-32601, "Method not found", self.nodes[0].analyzedescriptor, "foo()")

    def run_test(self):
        cosigners = [self.create_cosigner(name) for name in ("Alice", "Bob", "Carol")]
        self.cosigners = [wallet for wallet, _ in cosigners]
        self.keys = [key for _, key in cosigners]
        self.alice = self.cosigners[0]

        self.test_errors()
        self.test_cosigners()
        self.test_locked_wallet()
        self.test_generated_address()
        self.test_musig_only()
        self.test_taproot_parity()
        self.test_watch_only()
        self.test_disabled_wallet()


if __name__ == "__main__":
    WalletAnalyzeDescriptorTest(__file__).main()
