#!/usr/bin/env python3
# Copyright (c) 2026-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test wallet gethdkey RPC."""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_raises_rpc_error,
)
from test_framework.descriptors import descsum_create
from test_framework.wallet_util import WalletUnlock


class WalletGetHDKeyTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def init_wallet(self, *, node):
        pass  # Don't create default wallet

    def run_test(self):
        self.test_derivation_vectors()
        self.test_basic_gethdkey()
        self.test_path_formats()
        self.test_error_cases()
        self.test_multisig_workflow()

    def test_derivation_vectors(self):
        """Verify gethdkey produces exact expected values from BIP32 test vectors."""
        self.log.info("Test gethdkey against BIP32 test vectors")

        # BIP32 Test Vector 1 - converted to regtest (tprv/tpub) format
        MASTER_TPRV = "tprv8ZgxMBicQKsPeDgjzdC36fs6bMjGApWDNLR9erAXMs5skhMv36j9MV5ecvfavji5khqjWaWSFhN3YcCUUdiKH6isR4Pwy3U5y5egddBr16m"
        MASTER_TPUB = "tpubD6NzVbkrYhZ4XgiXtGrdW5XDAPFCL9h7we1vwNCpn8tGbBcgfVYjXyhWo4E1xkh56hjod1RhGjxbaTLV3X4FyWuejifB9jusQ46QzG87VKp"

        VECTORS = [
            ("m", MASTER_TPUB, MASTER_TPRV),
            ("m/0'",
             "tpubD8eQVK4Kdxg3gHrF62jGP7dKVCoYiEB8dFSpuTawkL5YxTus5j5pf83vaKnii4bc6v2NVEy81P2gYrJczYne3QNNwMTS53p5uzDyHvnw2jm",
             "tprv8bxNLu25VazNnppTCP4fyhyCvBHcYtzE3wr3cwYeL4HA7yf6TLGEUdS4QC1vLT63TkjRssqJe4CvGNEC8DzW5AoPUw56D1Ayg6HY4oy8QZ9"),
            ("m/0'/1",
             "tpubDApXh6cD2fZ7WjtgpHd8yrWyYaneiFuRZa7fVjMkgxsmC1QzoXW8cgx9zQFJ81Jx4deRGfRE7yXA9A3STsxXj4CKEZJHYgpMYikkas9DBTP",
             "tprv8e8VYgZxtHsSdGrtvdxYaSrryZGiYviWzGWtDDKTGh5NMXAEB8gYSCLHpFCywNs5uqV7ghRjimALQJkRFZnUrLHpzi2pGkwqLtbubgWuQ8q"),
            ("m/0'/1/2'",
             "tpubDDRojdS4jYQXNugn4t2WLrZ7mjfAyoVQu7MLk4eurqFCbrc7cHLZX8W5YRS8ZskGR9k9t3PqVv68bVBjAyW4nWM9pTGRddt3GQftg6MVQsm",
             "tprv8gjmbDPpbAirVSezBEMuwSu1Ci9EpUJWKokZTYccSZSomNMLytWyLdtDNHRbucNaRJWWHANf9AzEdWVAqahfyRjVMKbNRhBmxAM8EJr7R15"),
        ]

        self.nodes[0].createwallet(wallet_name="vectors", blank=True)
        wallet = self.nodes[0].get_wallet_rpc("vectors")

        desc = f"wpkh({MASTER_TPRV}/0/*)"
        wallet.importdescriptors([{"desc": descsum_create(desc), "timestamp": "now", "active": True, "range": [0, 10]}])

        for path, expected_tpub, expected_tprv in VECTORS:
            result = wallet.gethdkey(path, {"private": True})
            assert_equal(result["xpub"], expected_tpub)
            assert_equal(result["xprv"], expected_tprv)

    def test_basic_gethdkey(self):
        self.log.info("Test gethdkey basics")
        self.nodes[0].createwallet("basic")
        wallet = self.nodes[0].get_wallet_rpc("basic")

        # Root path returns xpub only, no fingerprint/origin
        result = wallet.gethdkey("m")
        assert "xpub" in result
        assert "xprv" not in result
        assert "fingerprint" not in result
        root_xpub = result["xpub"]

        # With private=true, returns xprv
        result = wallet.gethdkey("m", {"private": True})
        assert_equal(result["xpub"], root_xpub)
        assert "xprv" in result

        # Derived path includes fingerprint and origin
        result = wallet.gethdkey("m/84'/0'/0'")
        assert "fingerprint" in result
        assert "origin" in result
        assert result["origin"].startswith("[" + result["fingerprint"])
        assert "/84'/0'/0'" in result["origin"]

        # Encrypted wallet requires unlock for hardened derivation or private key
        self.log.info("Test encrypted wallet behavior")
        wallet.encryptwallet("pass")

        # Can get xpub at root without unlock
        result = wallet.gethdkey("m")
        assert "xpub" in result

        # Hardened derivation requires unlock
        assert_raises_rpc_error(-13, "Please enter the wallet passphrase", wallet.gethdkey, "m/84'/0'/0'")

        # Private key requires unlock
        assert_raises_rpc_error(-13, "Please enter the wallet passphrase", wallet.gethdkey, "m", {"private": True})

        with WalletUnlock(wallet, "pass"):
            result = wallet.gethdkey("m/84'/0'/0'", {"private": True})
            assert "xprv" in result

    def test_path_formats(self):
        self.log.info("Test path formats")
        self.nodes[0].createwallet("pathtest")
        wallet = self.nodes[0].get_wallet_rpc("pathtest")

        # Both ' and h notation produce same result
        result_apostrophe = wallet.gethdkey("m/84'/0'/0'")
        result_h = wallet.gethdkey("m/84h/0h/0h")
        assert_equal(result_apostrophe["xpub"], result_h["xpub"])

        # Non-hardened derivation works without private key
        result = wallet.gethdkey("m/0/0/0")
        assert "xpub" in result

    def test_error_cases(self):
        self.log.info("Test error cases")
        node = self.nodes[0]

        # Invalid paths
        node.createwallet("errtest")
        wallet = node.get_wallet_rpc("errtest")
        assert_raises_rpc_error(-8, "Invalid BIP 32 keypath", wallet.gethdkey, "invalid")
        assert_raises_rpc_error(-8, "Invalid BIP 32 keypath", wallet.gethdkey, "m/abc")

        # Path exceeds maximum BIP 32 depth (255)
        long_path = "m" + "/0" * 256
        assert_raises_rpc_error(-8, "Path exceeds maximum", wallet.gethdkey, long_path)

        # Watch-only wallet has no HD key
        node.createwallet(wallet_name="watchonly", disable_private_keys=True)
        wallet = node.get_wallet_rpc("watchonly")
        assert_raises_rpc_error(-4, "does not have an active HD key", wallet.gethdkey, "m")

        # Blank wallet has no HD key
        node.createwallet(wallet_name="blank", blank=True)
        wallet = node.get_wallet_rpc("blank")
        assert_raises_rpc_error(-4, "does not have an active HD key", wallet.gethdkey, "m")

    def test_multisig_workflow(self):
        """Test 2-of-3 multisig using gethdkey to derive keys at BIP 87 path."""
        self.log.info("Test multisig workflow with gethdkey")
        node = self.nodes[0]

        # Create participant wallets and extract keys
        participants = []
        xpub_with_origins = []
        participant_keys = []

        for n in range(3):
            node.createwallet(f"participant_{n}")
            wallet = node.get_wallet_rpc(f"participant_{n}")
            participants.append(wallet)

            result = wallet.gethdkey("m/87'/1'/0'", {"private": True})
            participant_keys.append(result)
            xpub_with_origins.append(f"{result['origin']}{result['xpub']}/<0;1>/*")

        # Create watch-only multisig wallet
        multisig_desc = f"wsh(sortedmulti(2,{','.join(xpub_with_origins)}))"
        node.createwallet(wallet_name="multisig", disable_private_keys=True, blank=True)
        multisig = node.get_wallet_rpc("multisig")
        multisig.importdescriptors([{"desc": descsum_create(multisig_desc), "active": True, "timestamp": "now"}])

        # Import multisig descriptor with xprv into participant wallets
        for n, wallet in enumerate(participants):
            keys = []
            for i in range(3):
                if i == n:
                    keys.append(f"{participant_keys[n]['origin']}{participant_keys[n]['xprv']}/<0;1>/*")
                else:
                    keys.append(xpub_with_origins[i])
            priv_desc = f"wsh(sortedmulti(2,{','.join(keys)}))"
            wallet.importdescriptors([{"desc": descsum_create(priv_desc), "timestamp": "now"}])

        # Fund multisig
        node.createwallet("funder")
        funder = node.get_wallet_rpc("funder")
        self.generatetoaddress(node, 101, funder.getnewaddress())
        funder.sendtoaddress(multisig.getnewaddress(), 1.0)
        self.generatetoaddress(node, 1, funder.getnewaddress())
        assert_equal(multisig.getbalance(), 1.0)

        # Create and sign PSBT with 2 participants
        psbt = multisig.walletcreatefundedpsbt([], [{participants[0].getnewaddress(): 0.5}])["psbt"]
        psbt_1 = participants[0].walletprocesspsbt(psbt)["psbt"]
        psbt_2 = participants[1].walletprocesspsbt(psbt)["psbt"]
        combined = node.combinepsbt([psbt_1, psbt_2])

        # Finalize and broadcast
        finalized = node.finalizepsbt(combined)
        assert finalized["complete"]
        node.sendrawtransaction(finalized["hex"])

        self.log.info("Multisig workflow completed successfully")


if __name__ == '__main__':
    WalletGetHDKeyTest(__file__).main()
