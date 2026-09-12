#!/usr/bin/env python3
# Copyright (c) 2026-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test per-key labels keyed by master fingerprint.

RPCs tested are:
    - setkeylabel
    - getkeylabel
    - listkeylabels
    - getaddressinfo (key_labels field)
"""
from test_framework.descriptors import descsum_create
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal, assert_raises_rpc_error


class WalletKeyLabelsTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        node = self.nodes[0]
        self.wallet_name = "keylabels_test"
        node.createwallet(wallet_name=self.wallet_name)
        wallet = node.get_wallet_rpc(self.wallet_name)

        # A descriptor wallet reports the master fingerprint for each of its keys.
        addr = wallet.getnewaddress()
        info = wallet.getaddressinfo(addr)
        fingerprint = info["hdmasterfingerprint"]
        assert_equal(len(fingerprint), 8)
        # No per-key label is set yet: `labels` carries only the per-address label
        # (the empty default for a fresh receive address) and `key_labels` is empty.
        assert_equal(info["labels"], [""])
        assert_equal(info["key_labels"], {})

        # set / get / list
        wallet.setkeylabel(fingerprint, "my hardware wallet")
        assert_equal(wallet.getkeylabel(fingerprint), "my hardware wallet")
        assert_equal(wallet.listkeylabels(), {fingerprint: "my hardware wallet"})

        # gethdkeys reports the fingerprint and per-key label for each HD key.
        hdkeys = wallet.gethdkeys()
        assert_equal(len(hdkeys), 1)
        assert_equal(hdkeys[0]["fingerprint"], fingerprint)
        assert_equal(hdkeys[0]["label"], "my hardware wallet")

        # Per-key labels live in their own `key_labels` object, keyed by fingerprint,
        # and are never merged into the per-address `labels` array.
        info = wallet.getaddressinfo(addr)
        assert_equal(info["labels"], [""])
        assert_equal(info["key_labels"], {fingerprint: "my hardware wallet"})

        # Change addresses have no address-book entry, so `labels` is empty while the
        # key label is still reported under `key_labels`.
        change_info = wallet.getaddressinfo(wallet.getrawchangeaddress())
        assert_equal(change_info["labels"], [])
        assert_equal(change_info["key_labels"], {fingerprint: "my hardware wallet"})

        # Per-key labels do not replace, per-address labels.
        wallet.setlabel(addr, "receiving")
        info = wallet.getaddressinfo(addr)
        assert_equal(info["labels"], ["receiving"])
        assert_equal(info["key_labels"], {fingerprint: "my hardware wallet"})

        # Both the label and the per-key label is the same and both must appear.
        wallet.setlabel(addr, "my hardware wallet")
        info = wallet.getaddressinfo(addr)
        assert_equal(info["labels"], ["my hardware wallet"])
        assert_equal(info["key_labels"], {fingerprint: "my hardware wallet"})
        # Reset the address label for the next tests.
        wallet.setlabel(addr, "receiving")

        # Persistence across reload.
        node.unloadwallet(self.wallet_name)
        node.loadwallet(self.wallet_name)
        wallet = node.get_wallet_rpc(self.wallet_name)
        assert_equal(wallet.getkeylabel(fingerprint), "my hardware wallet")
        assert_equal(wallet.listkeylabels(), {fingerprint: "my hardware wallet"})

        # Clearing via an empty label leaves the per-address label untouched.
        wallet.setkeylabel(fingerprint, "")
        assert_equal(wallet.getkeylabel(fingerprint), None)
        assert_equal(wallet.listkeylabels(), {})
        info = wallet.getaddressinfo(addr)
        assert_equal(info["labels"], ["receiving"])
        assert_equal(info["key_labels"], {})

        # After clearing the per-key label, gethdkeys still reports the
        # fingerprint but no longer includes a `label` field.
        hdkeys = wallet.gethdkeys()
        assert_equal(hdkeys[0]["fingerprint"], fingerprint)
        assert "label" not in hdkeys[0]

        # Invalid fingerprints are rejected.
        assert_raises_rpc_error(-5, "fingerprint must be 8 hex characters", wallet.setkeylabel, "nothex", "x")
        assert_raises_rpc_error(-5, "fingerprint must be 8 hex characters", wallet.setkeylabel, "deadbeef00", "x")
        assert_raises_rpc_error(-5, "fingerprint must be 8 hex characters", wallet.getkeylabel, "zz")

        # Multisig: a wsh(multi(...)) address reports one `key_labels` entry per
        # participant key, keyed by fingerprint. A 2-of-3 multisig is watched from a
        # private-keys-disabled wallet, and all three participating fingerprints are
        # labelled there.
        node.createwallet(wallet_name="cosigner1")
        co1 = node.get_wallet_rpc("cosigner1")
        co1_fingerprint = co1.getaddressinfo(co1.getnewaddress())["hdmasterfingerprint"]
        co1_xpub = co1.gethdkeys()[0]["xpub"]

        node.createwallet(wallet_name="cosigner2")
        co2 = node.get_wallet_rpc("cosigner2")
        co2_fingerprint = co2.getaddressinfo(co2.getnewaddress())["hdmasterfingerprint"]
        co2_xpub = co2.gethdkeys()[0]["xpub"]

        own_xpub = wallet.gethdkeys()[0]["xpub"]

        node.createwallet(wallet_name="multi_watch", disable_private_keys=True)
        mw = node.get_wallet_rpc("multi_watch")

        multi_desc = descsum_create(
            "wsh(multi(2,"
            f"[{fingerprint}/0h]{own_xpub}/0/*,"
            f"[{co1_fingerprint}/0h]{co1_xpub}/0/*,"
            f"[{co2_fingerprint}/0h]{co2_xpub}/0/*))"
        )
        assert mw.importdescriptors(
            [{"desc": multi_desc, "timestamp": "now", "range": [0, 0]}]
        )[0]["success"]

        mw.setkeylabel(fingerprint, "main")
        mw.setkeylabel(co1_fingerprint, "cosigner1")
        mw.setkeylabel(co2_fingerprint, "cosigner2")

        # gethdkeys reports per-key labels for each multisig participant key.
        mw_hdkeys = {k["fingerprint"]: k for k in mw.gethdkeys()}
        assert_equal(mw_hdkeys[fingerprint]["label"], "main")
        assert_equal(mw_hdkeys[co1_fingerprint]["label"], "cosigner1")
        assert_equal(mw_hdkeys[co2_fingerprint]["label"], "cosigner2")

        multi_addr = node.deriveaddresses(multi_desc, [0, 0])[0]
        multi_info = mw.getaddressinfo(multi_addr)
        assert_equal(
            multi_info["key_labels"],
            {fingerprint: "main", co1_fingerprint: "cosigner1", co2_fingerprint: "cosigner2"},
        )
        # The watchonly multisig address has no address-book entry, so `labels` is
        # empty; its three securing keys are distinguished only under `key_labels`.
        assert_equal(multi_info["labels"], [])


if __name__ == "__main__":
    WalletKeyLabelsTest(__file__).main()
