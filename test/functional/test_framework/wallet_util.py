#!/usr/bin/env python3
# Copyright (c) 2018-2021 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Useful util functions for testing the wallet"""
from collections import namedtuple

from test_framework.address import (
    byte_to_base58,
    key_to_p2pkh,
    key_to_p2sh_p2wpkh,
    key_to_p2wpkh,
    script_to_p2sh,
    script_to_p2sh_p2wsh,
    script_to_p2wsh,
)
from test_framework.key import ECKey
from test_framework.script_util import (
    key_to_p2pkh_script,
    key_to_p2wpkh_script,
    keys_to_multisig_script,
    script_to_p2sh_script,
    script_to_p2wsh_script,
)

Key = namedtuple('Key', ['privkey',
                         'pubkey',
                         'p2pkh_script',
                         'p2pkh_addr',
                         'p2wpkh_script',
                         'p2wpkh_addr',
                         'p2sh_p2wpkh_script',
                         'p2sh_p2wpkh_redeem_script',
                         'p2sh_p2wpkh_addr'])

Multisig = namedtuple('Multisig', ['privkeys',
                                   'pubkeys',
                                   'p2sh_script',
                                   'p2sh_addr',
                                   'redeem_script',
                                   'p2wsh_script',
                                   'p2wsh_addr',
                                   'p2sh_p2wsh_script',
                                   'p2sh_p2wsh_addr'])

def get_key(node):
    """Generate a fresh key on node

    Returns a named tuple of privkey, pubkey and all address and scripts."""
    addr = node.getnewaddress()
    pubkey = node.getaddressinfo(addr)['pubkey']
    return Key(privkey=node.dumpprivkey(addr),
               pubkey=pubkey,
               p2pkh_script=key_to_p2pkh_script(pubkey).hex(),
               p2pkh_addr=key_to_p2pkh(pubkey),
               p2wpkh_script=key_to_p2wpkh_script(pubkey).hex(),
               p2wpkh_addr=key_to_p2wpkh(pubkey),
               p2sh_p2wpkh_script=script_to_p2sh_script(key_to_p2wpkh_script(pubkey)).hex(),
               p2sh_p2wpkh_redeem_script=key_to_p2wpkh_script(pubkey).hex(),
               p2sh_p2wpkh_addr=key_to_p2sh_p2wpkh(pubkey))

def get_generate_key():
    """Generate a fresh key

    Returns a named tuple of privkey, pubkey and all address and scripts."""
    eckey = ECKey()
    eckey.generate()
    privkey = bytes_to_wif(eckey.get_bytes())
    pubkey = eckey.get_pubkey().get_bytes().hex()
    return Key(privkey=privkey,
               pubkey=pubkey,
               p2pkh_script=key_to_p2pkh_script(pubkey).hex(),
               p2pkh_addr=key_to_p2pkh(pubkey),
               p2wpkh_script=key_to_p2wpkh_script(pubkey).hex(),
               p2wpkh_addr=key_to_p2wpkh(pubkey),
               p2sh_p2wpkh_script=script_to_p2sh_script(key_to_p2wpkh_script(pubkey)).hex(),
               p2sh_p2wpkh_redeem_script=key_to_p2wpkh_script(pubkey).hex(),
               p2sh_p2wpkh_addr=key_to_p2sh_p2wpkh(pubkey))

def get_multisig(node):
    """Generate a fresh 2-of-3 multisig on node

    Returns a named tuple of privkeys, pubkeys and all address and scripts."""
    addrs = []
    pubkeys = []
    for _ in range(3):
        addr = node.getaddressinfo(node.getnewaddress())
        addrs.append(addr['address'])
        pubkeys.append(addr['pubkey'])
    script_code = keys_to_multisig_script(pubkeys, k=2)
    witness_script = script_to_p2wsh_script(script_code)
    return Multisig(privkeys=[node.dumpprivkey(addr) for addr in addrs],
                    pubkeys=pubkeys,
                    p2sh_script=script_to_p2sh_script(script_code).hex(),
                    p2sh_addr=script_to_p2sh(script_code),
                    redeem_script=script_code.hex(),
                    p2wsh_script=witness_script.hex(),
                    p2wsh_addr=script_to_p2wsh(script_code),
                    p2sh_p2wsh_script=script_to_p2sh_script(witness_script).hex(),
                    p2sh_p2wsh_addr=script_to_p2sh_p2wsh(script_code))

def test_address(node, address, **kwargs):
    """Get address info for `address` and test whether the returned values are as expected."""
    addr_info = node.getaddressinfo(address)
    for key, value in kwargs.items():
        if value is None:
            if key in addr_info.keys():
                raise AssertionError("key {} unexpectedly returned in getaddressinfo.".format(key))
        elif addr_info[key] != value:
            raise AssertionError("key {} value {} did not match expected value {}".format(key, addr_info[key], value))

def bytes_to_wif(b, compressed=True):
    if compressed:
        b += b'\x01'
    return byte_to_base58(b, 239)

def generate_wif_key():
    # Makes a WIF privkey for imports
    k = ECKey()
    k.generate()
    return bytes_to_wif(k.get_bytes(), k.is_compressed)
