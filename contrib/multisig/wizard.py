#!/usr/bin/env python3
# Copyright (c) 2026-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
import re

KEY_EXPRESSION_RE = re.compile(r"^\[[0-9a-f]{8}(/\d+h?)*\][a-zA-Z0-9]+$")


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

def validate_key(rpc, key, *, account=0):
    """Check a key expression a cosigner sent us.

    Returns the key in canonical form. Raises ValueError
    if the key cannot be used.

    The wizard additionally requires the BIP 87 path.

    There is no RPC for validating a key expression,
    so we wrap it in a `pkh` descriptor for getdescriptorinfo
    to parse. See #35918, which exposes ParsePubkey().
    """
    try:
        info = rpc.getdescriptorinfo(f"pkh({key})")
    except Exception as e:
        raise ValueError(f"Invalid key '{key}': {e}") from e
    if info["hasprivatekeys"]:
        raise ValueError(f"Key '{key}' contains private key; only public keys may be shared with cosigners")
    if "multipath_expansion" in info:
        raise ValueError(f"Key '{key}' must not specify a multipath derivation")

    canonical = info["descriptor"][len("pkh("):info["descriptor"].index(")")]
    canonical = canonical.replace("'", "h")
    if not canonical.startswith("["):
        raise ValueError(f"Key '{key}' must include key origin information, formatted [fingerprint/path]xpub")
    if not KEY_EXPRESSION_RE.match(canonical):
        raise ValueError(f"Key '{key}' must not specify a derivation path")

    path = f"m/{canonical[10:canonical.index(']')]}"
    expected = account_path(rpc.getblockchaininfo()["chain"], account)
    if path != expected:
        raise ValueError(f"Key '{key}' is derived at {path}; the wizard uses the BIP 87 path {expected}")
    return canonical


def add_cosigner(rpc, setup, name, key, *, account=0):
    """Validate a cosigner's key expression and record it by name.

    Returns a list of warnings; raises ValueError if the key cannot be used.

    Key labels: the wallet cannot store a name for a key, so names live in the
    setup and are lost on import. Unused() descriptors cannot carry a label either,
    since they don't produce output scripts.
    """
    canonical = validate_key(rpc, key, account=account)
    if any(cosigner["name"] == name for cosigner in setup["cosigners"]):
        raise ValueError(f"There is already a cosigner called {name}")
    xpub = canonical[canonical.index("]") + 1:]
    warnings = []
    for cosigner in setup["cosigners"]:
        if cosigner["key"][cosigner["key"].index("]") + 1:] == xpub:
            raise ValueError(f"Duplicate key: already added as {cosigner['name']}")
        if cosigner["key"][1:9] == canonical[1:9]:
            warnings.append(f"Key has the same fingerprint as {cosigner['name']}")

    setup["cosigners"].append({"name": name, "key": canonical})
    return warnings
