#!/usr/bin/env python3
# Copyright (c) 2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test miniscript implementation."""

from test_framework.key import ECKey
from test_framework.miniscript import Node
from test_framework.messages import hash256, sha256
from test_framework.script import hash160
from test_framework.test_framework import BitcoinTestFramework

import hashlib


class MiniscriptTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True

    def test_miniscript_expr(self, desc_str, property_str):
        node = Node.from_desc(desc_str)
        node_from_script = Node.from_script(node.script)
        assert(''.join(sorted(property_str)) ==
               ''.join(sorted(node.p.to_string()))
               )
        assert(node.desc == desc_str)
        assert(node_from_script.desc == node.desc)
        assert(node_from_script.script == node.script)
        return node

    def generate_key_pair(self, keyhash_dict):
        privkey = ECKey()
        privkey.generate()
        pubkeyhash = hash160(privkey.get_pubkey().get_bytes())
        keyhash_dict[pubkeyhash] = privkey

    def get_pubkey_hex_list(self, n, keyhash_dict):
        key_hex_list = []
        for pkh in list(keyhash_dict)[0:n]:
            pk_hex = keyhash_dict[pkh].get_pubkey().get_bytes().hex()
            key_hex_list.append(pk_hex)
        return key_hex_list

    def run_test(self):
        # Generate key pairs.
        keyhash_dict = {}
        for i in range(5):
            self.generate_key_pair(keyhash_dict)

        # Generate hashlocks.
        sha256_dict = {}
        hash256_dict = {}
        ripemd160_dict = {}
        hash160_dict = {}
        for i in range(5):
            secret = ECKey()
            secret.generate()
            sha256_dict[sha256(secret.get_bytes())] = secret.get_bytes()
            hash256_dict[hash256(secret.get_bytes())] = secret.get_bytes()
            ripemd160_dict[hashlib.new(
                'ripemd160', secret.get_bytes()).digest()] = secret.get_bytes()
            hash160_dict[hash160(secret.get_bytes())] = secret.get_bytes()

        # Test (de)serialization of terminal expressions.

        # pk(key)
        pk_hex = self.get_pubkey_hex_list(1, keyhash_dict)[0]
        desc0 = "pk({})".format(pk_hex)
        self.test_miniscript_expr(desc0, "Konduesm")

        # pk_h(key)
        pkh = list(keyhash_dict)[0]
        desc1 = "pk_h({})".format(pkh.hex())
        self.test_miniscript_expr(desc1, "Knduesm")

        # older(delay)
        delay = 20
        desc2 = "older({})".format(delay)
        self.test_miniscript_expr(desc2, "Bzfm")

        # after(time)
        time = 21
        desc3 = "after({})".format(time)
        self.test_miniscript_expr(desc3, "Bzfm")

        # sha256(h)
        sha256_digest = list(sha256_dict)[0]
        desc5 = "sha256({})".format(sha256_digest.hex())
        self.test_miniscript_expr(desc5, "Bondum")

        # hash256(h)
        h256_digest = list(hash256_dict)[0]
        desc6 = "hash256({})".format(h256_digest.hex())
        self.test_miniscript_expr(desc6, "Bondum")

        # ripemd160(h)
        ripemd160_digest = list(ripemd160_dict)[0]
        desc7 = "ripemd160({})".format(ripemd160_digest.hex())
        self.test_miniscript_expr(desc7, "Bondum")

        # hash160(h)
        h160_digest = list(hash160_dict)[0]
        desc8 = "hash160({})".format(h160_digest.hex())
        self.test_miniscript_expr(desc8, "Bondum")

        # thresh_m(k, pk, pk, ...)
        k = 2
        pk_hex_list = self.get_pubkey_hex_list(3, keyhash_dict)
        desc9 = ("thresh_m({},{},{},{})").format(k, pk_hex_list[0],
                                                 pk_hex_list[1],
                                                 pk_hex_list[2])
        self.test_miniscript_expr(desc9, "Bnduesm")


if __name__ == '__main__':
    MiniscriptTest().main()
