#!/usr/bin/env python3
# Copyright (c) 2026-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

def setup(name):
    """init a new multisig wallet."""
    return {
        "name": name,
        "threshold": None,
        "type": None,
        "cosigners": [],
        "descriptor": None,
    }


def create_wallet(rpc, name, *, holds_key=True, passphrase=None):
    """Create the local wallet for multisig wallet name.

    A participant that contributes a key needs private keys enabled, both to hold that key and to sign.
    A coordinator that contributes no key passes holds_key=False and gets a watch-only wallet.

    Returns the wallet name, which is local and not part of the setup.
    """
    if passphrase is not None and not holds_key:
        raise ValueError("passphrase only applies to a wallet that holds a key")
    options = {} if passphrase is None else {"passphrase": passphrase}
    rpc.createwallet(wallet_name=name, blank=True, disable_private_keys=not holds_key, **options)
    return name


def account_path(chain, account=0):
    coin_type = 0 if chain == "main" else 1
    return f"m/87h/{coin_type}h/{account}h"


def generate_key(rpc, *, account=0):
    """Generate this device's key and return its key expression.
    """
    master_xpub = rpc.addhdkey()["xpub"]
    path = account_path(rpc.getblockchaininfo()["chain"], account)
    key = rpc.derivehdkey(path, hdkey=master_xpub)
    return f"{key['origin']}{key['xpub']}"
