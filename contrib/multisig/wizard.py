#!/usr/bin/env python3
# Copyright (c) 2026-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
import re

KEY_EXPRESSION_RE = re.compile(r"^\[[0-9a-f]{8}(/\d+h?)*\][a-zA-Z0-9]+$")
KEY_PATH_MUSIG = "musig"
KEY_PATH_NUMS = "nums"
MAX_KEYS = {"bech32": 20, "bech32m": 999}
MULTIPATH_SUFFIX = "/<0;1>/*"

# BIP 341's NUMS point. Kept so that descriptors from other software still verify here.
NUMS_H = "50929b74c1a04954b78b4b6035e97a5e078a5a0f28ec96d547bfee9ace803ac0"
NUMS_H_XPUB = "xpub661MyMwAqRbcEYS8w7XLSVeEsBXy79zSzH1J8vCdxAZningWLdN3zgtU6QgnecKFpJFPpdzxKrwoaZoV44qAJewsc4kX9vGaCaBExuvJH57"

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

def build_descriptor(keys, threshold, address_type):
    """Build the descriptor string, without its checksum."""
    paths = ",".join(f"{key}{MULTIPATH_SUFFIX}" for key in keys)
    if address_type == "bech32":
        return f"wsh(sortedmulti({threshold},{paths}))"
    aggregate = f"musig({','.join(keys)})"
    return f"tr({aggregate}{MULTIPATH_SUFFIX},sortedmulti_a({threshold},{paths}))"


def assemble(rpc, setup, threshold, address_type="bech32"):
    """Build the shared descriptor from the collected keys.

    address_type is "bech32" for wsh(sortedmulti(...)) or
    "bech32m" for a taproot descriptor tr(musig(), sortedmulti_a(...)).

    The taproot key path is a MuSig2 aggregate of all n keys.

    Records and returns the descriptor with its checksum.
    """
    if address_type not in MAX_KEYS:
        raise ValueError("Unsupported address type")
    keys = [cosigner["key"] for cosigner in setup["cosigners"]]
    n = len(keys)
    if n < 2:
        raise ValueError("A multisig requires at least 2 keys")
    if n > MAX_KEYS[address_type]:
        raise ValueError(f"A multisig cannot have more than {MAX_KEYS[address_type]} keys, {n} were provided")
    if threshold < 1 or threshold > n:
        raise ValueError(f"The threshold must be between 1 and {n}, {threshold} was provided")

    descriptor = build_descriptor(keys, threshold, address_type)

    try:
        info = rpc.getdescriptorinfo(descriptor)
    except Exception as e:
        raise ValueError(f"Invalid descriptor: {e}") from e
    if len(info.get("multipath_expansion", [])) != 2:
        raise ValueError("Descriptor did not expand to a receive and a change path")

    setup["threshold"] = threshold
    setup["type"] = address_type
    setup["descriptor"] = f"{descriptor}#{info['checksum']}"
    return setup["descriptor"]

def split_expressions(expression):
    """Split a descriptor expression on its top level commas.

    For tr(), the script tree is one sub_expressions.
    """
    sub_expressions = []
    depth = 0
    start = 0
    for i, char in enumerate(expression):
        if char in "({":
            depth += 1
        elif char in ")}":
            depth -= 1
        elif char == "," and depth == 0:
            sub_expressions.append(expression[start:i])
            start = i + 1
    sub_expressions.append(expression[start:])
    return sub_expressions


def unwrap(expression, node):
    """Return the expressions of node(...), or None if that is not it."""
    prefix = f"{node}("
    if expression.startswith(prefix) and expression.endswith(")"):
        return split_expressions(expression[len(prefix):-1])
    return None


def parse_multi(expression, node):
    """Parse a sortedmulti(k,key,...) or sortedmulti_a(k,key,...) node."""
    sub_expressions = unwrap(expression, node)
    if sub_expressions is None:
        raise ValueError(f"Expected {node}(...), got: {expression}")
    threshold, *keys = sub_expressions
    if not threshold.isdigit():
        raise ValueError(f"Threshold is not a number: {threshold}")
    if len(keys) < 2:
        raise ValueError("A multisig requires at least 2 keys")
    stripped = []
    for key in keys:
        if not key.endswith(MULTIPATH_SUFFIX):
            raise ValueError(f"Key '{key}' does not derive over {MULTIPATH_SUFFIX}")
        stripped.append(key[:-len(MULTIPATH_SUFFIX)])
    return int(threshold), stripped


def analyze_descriptor(rpc, descriptor):
    """Report what a multisig descriptor requires to spend.

    Accepted shapes are wsh(sortedmulti(...)) and, for taproot,
    tr(KEY,sortedmulti_a(...)) where KEY is either musig() of exactly the
    keys in the leaf, so that spending by key path needs
    every cosigner, or the NUMS point, which is provably unspendable.
    Anything else is rejected.

    Returns: address_type, threshold, keys, key_path (None for wsh, else
    KEY_PATH_MUSIG or KEY_PATH_NUMS), and descriptor, the checksum-less body
    with hardened markers normalised to h.
    """
    try:
        info = rpc.getdescriptorinfo(descriptor)
    except Exception as e:
        raise ValueError(f"Invalid descriptor: {e}") from e
    if len(info.get("multipath_expansion", [])) != 2:
        raise ValueError("Descriptor does not expand to a receive and a change path")

    body = descriptor.split("#")[0].replace("'", "h")
    script = unwrap(body, "wsh")
    if script is not None:
        if len(script) != 1:
            raise ValueError("wsh() takes a single script")
        threshold, keys = parse_multi(script[0], "sortedmulti")
        return {"address_type": "bech32", "threshold": threshold, "keys": keys, "key_path": None, "descriptor": body}

    tree = unwrap(body, "tr")
    if tree is None:
        raise ValueError("Not a wsh() or tr() descriptor")
    if len(tree) != 2:
        raise ValueError("Taproot descriptor must be tr(KEY,sortedmulti_a(...))")
    internal, leaf = tree
    threshold, keys = parse_multi(leaf, "sortedmulti_a")
    participants = None
    if internal.endswith(MULTIPATH_SUFFIX):
        participants = unwrap(internal[:-len(MULTIPATH_SUFFIX)], "musig")
    if participants is not None and sorted(participants) == sorted(keys):
        # MuSigPubkeyProvider sorts the participants before aggregating them
        # (BIP 390), so the order they are written in does not matter.
        key_path = KEY_PATH_MUSIG
    elif internal in (NUMS_H, NUMS_H_XPUB):
        key_path = KEY_PATH_NUMS
    else:
        raise ValueError("Taproot key path is neither musig() of exactly the keys in the leaf nor the NUMS point, so it may be spendable on its own")
    return {"address_type": "bech32m", "threshold": threshold, "keys": keys, "key_path": key_path, "descriptor": body}


def find_own_key(rpc, keys):
    """Finds the keys owned by this wallet and the corresponding private key.

    Use the origin information on the descriptor xpub to derive against unused(KEY )and compare,
    and if that fails, compare the unused(KEY) directly against descriptor xpub.

    Returns an empty list if none of them are ours, which is the expected answer for a
    coordinator that does not contribute keys.

    see #35377

    """
    found = []
    for hdkey in rpc.gethdkeys():
        if not any(d["desc"].startswith("unused(") for d in hdkey["descriptors"]):
            continue
        for candidate in keys:
            xpub = candidate[candidate.index("]") + 1:] if candidate.startswith("[") else candidate
            if xpub == hdkey["xpub"]:
                # unused(KEY) at the same level as the descriptor uses it.
                key = rpc.derivehdkey("m", hdkey=hdkey["xpub"], private=True)
            elif candidate.startswith("["):
                path = f"m/{candidate[10:candidate.index(']')]}"
                key = rpc.derivehdkey(path, hdkey=hdkey["xpub"], private=True)
                if f"{key['origin']}{key['xpub']}".replace("'", "h") != candidate:
                    continue
            else:
                continue
            found.append({"key": candidate,
                          "private_key": f"{candidate[:candidate.index(']') + 1]}{key['xprv']}"
                                         if candidate.startswith("[") else key["xprv"]})
    return found


def verify(rpc, setup):
    """Check the descriptor we are about to import.

    Reports what the descriptor requires to spend, which cosigner we are, and the first receive address,
    which the participants compare with each other out of band.

    Names are local, so any cosigner we have no name for is reported by
    fingerprint.
    """
    if not setup["descriptor"]:
        raise ValueError("There is no descriptor to verify yet")
    policy = analyze_descriptor(rpc, setup["descriptor"])
    ours = {own["key"] for own in find_own_key(rpc, policy["keys"])}
    names = {cosigner["key"]: cosigner["name"] for cosigner in setup["cosigners"]}
    receive, _change = rpc.deriveaddresses(setup["descriptor"], 0)

    return {
        "name": setup["name"],
        "type": policy["address_type"],
        "threshold": policy["threshold"],
        "cosigners": [{"name": names.get(key), "fingerprint": key[1:9], "yours": key in ours}
                      for key in policy["keys"]],
        "holds_your_key": bool(ours),
        "key_path": policy["key_path"],
        "first_address": receive[0],
    }
