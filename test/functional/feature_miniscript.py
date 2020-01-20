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

    def test_miniscript_expr(self, desc_str, property_str,
                             alt_desc_str=None):
        node = Node.from_desc(desc_str)
        node_from_script = Node.from_script(node.script)
        assert(''.join(sorted(property_str)) ==
               ''.join(sorted(node.p.to_string()))
               )
        assert(node.desc == desc_str)
        assert(node_from_script.script == node.script)
        if alt_desc_str is None:
            assert(node_from_script.desc == node.desc)
        else:
            assert(node_from_script.desc == alt_desc_str)
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

        # Test (de)serialization of composable expressions.

        # c:pkh(key)
        pkh = list(keyhash_dict)[0]
        desc10 = "c:pk_h({})".format(pkh.hex())
        self.test_miniscript_expr(desc10, "Bndemus")

        # and_v(vc:pk_h(key), c:pk_h(key)) expression.
        pkh0 = list(keyhash_dict)[0]
        pkh1 = list(keyhash_dict)[1]
        desc11 = "and_v(vc:pk_h({}),c:pk_h({}))".format(pkh0.hex(), pkh1.hex())
        self.test_miniscript_expr(desc11, "Bnfmus")

        # and_v(vc:pk_h(key),older(delay))) expression.
        pkh0 = list(keyhash_dict)[0]
        delay = 10
        desc12 = "and_v(vc:pk_h({}),older({}))".format(
            pkh0.hex(), delay)
        self.test_miniscript_expr(desc12, "Bnfms")

        # or_b(c:pk(key),a:and_b(c:pk_h(key),sc:pk(key)))
        pk_hex_list = self.get_pubkey_hex_list(2, keyhash_dict)
        pkh0 = list(keyhash_dict)[0]
        desc13 = "or_b(c:pk({}),a:and_b(c:pk_h({}),sc:pk({})))".format(
            pk_hex_list[0], pkh0.hex(), pk_hex_list[1])
        self.test_miniscript_expr(desc13, "Bdemus")

        # or_b(c:pk(key),a:and_n(c:pk(key),c:pk_h(key)))
        pk_hex_list = self.get_pubkey_hex_list(2, keyhash_dict)
        pkh0 = list(keyhash_dict)[0]
        desc14 = "or_b(c:pk({}),a:and_n(c:pk({}),c:pk_h({})))".format(
            *pk_hex_list, pkh0.hex())
        self.test_miniscript_expr(desc14, "Bdemus")

        # or_b(c:pk(key),sc:pk(key))
        pk_hex_list = self.get_pubkey_hex_list(2, keyhash_dict)
        desc15 = "or_b(c:pk({}),sc:pk({}))".format(*pk_hex_list)
        self.test_miniscript_expr(desc15, "Bdemus")

        # or_d(c:pk_h(key),c:pk_h(key))
        pk_hex_list = self.get_pubkey_hex_list(2, keyhash_dict)
        desc16 = "or_d(c:pk({}),c:pk({}))".format(*pk_hex_list)
        self.test_miniscript_expr(desc16, "Bdemus")

        # t:or_c(c:pk(key),and_v(vc:pk(key),or_c(c:pk(key),v:hash160(h))))
        # and_v(or_c(c:pk(key),and_v(vc:pk(key),or_c(c:pk(key),v:hash160(h)))),1)
        h160_digest = list(hash160_dict)[0]
        pk_hex_list = self.get_pubkey_hex_list(3, keyhash_dict)
        desc17 = ("t:or_c(c:pk({}),and_v(vc:pk({}),or_c(c:pk({})," +
                 "v:hash160({}))))").format(*pk_hex_list, h160_digest.hex())
        desc17_alt = ("and_v(or_c(c:pk({}),and_v(vc:pk({}),or_c(c:pk({})," +
                     "v:hash160({})))),1)").format(*pk_hex_list,
                                                   h160_digest.hex())
        self.test_miniscript_expr(desc17, "Bufsm", desc17_alt)

        # t:or_c(c:pk(key),and_v(vc:pk(key),or_c(c:pk(key),v:sha256(h))))
        # and_v(or_c(c:pk(key),and_v(vc:pk(key),or_c(c:pk(key),v:sha256(h)))),1)
        sha256_digest = list(sha256_dict)[0]
        pk_hex_list = self.get_pubkey_hex_list(3, keyhash_dict)
        desc18 = ("t:or_c(c:pk({}),and_v(vc:pk({}),or_c(c:pk({})," +
                 "v:sha256({}))))").format(*pk_hex_list, sha256_digest.hex())
        desc18_alt = ("and_v(or_c(c:pk({}),and_v(vc:pk({})," +
                     "or_c(c:pk({}),v:sha256({})))),1)").format(
                         *pk_hex_list, sha256_digest.hex())
        self.test_miniscript_expr(desc18, "Bufsm", desc18_alt)

        # or_i(and_v(vc:pk_h(key_local),hash256(h)),older(20))
        h256_digest = list(hash256_dict)[0]
        pkh0 = list(keyhash_dict)[0]
        delay = 20
        desc19 = "or_i(and_v(vc:pk_h({}),hash256({})),older({}))".format(
            pkh0.hex(), h256_digest.hex(), delay)
        self.test_miniscript_expr(desc19, "Bfm")

        # andor(c:pk(key),older(25),c:pk(key))
        delay = 25
        pk_hex_list = self.get_pubkey_hex_list(2, keyhash_dict)
        desc20 = "andor(c:pk({}),older({}),c:pk({}))".format(
            pk_hex_list[0], delay, pk_hex_list[1])
        self.test_miniscript_expr(desc20, "Bdems")

        # andor(c:pk(key),or_i(and_v(vc:pk_h(key),hash160(h)),older(35)),c:pk(key))
        older = 35
        pkh0 = list(keyhash_dict)[0]
        pk_hex_list = self.get_pubkey_hex_list(2, keyhash_dict)
        ripemd160_digest = list(ripemd160_dict)[0]
        desc21 = ("andor(c:pk({}),or_i(and_v(vc:pk_h({}),ripemd160({}))," +
                  "older({})),c:pk({}))").format(
            pk_hex_list[0], pkh0.hex(), ripemd160_digest.hex(), older,
            pk_hex_list[1])
        self.test_miniscript_expr(desc21, "Bdems")

        # thresh(k,c:pk(key),sc:pk(key),sc:pk(key),sdv:after(30))
        n = 4
        after = 30
        for k in range(2, n):
            pk_hex_list = self.get_pubkey_hex_list(n-1, keyhash_dict)
            desc22 = ("thresh({},c:pk({}),sc:pk({}),sc:pk({})," +
                           "sdv:after({}))").format(k, *pk_hex_list, after)
            self.test_miniscript_expr(desc22, "Bdmus")

        # thresh(k,c:pk(key),sc:pk(key),sc:pk(key),sc:pk(key),sc:pk(key))
        n = 5
        for k in range(2, n):
            pk_hex_list = self.get_pubkey_hex_list(n, keyhash_dict)
            desc23 = ("thresh({},c:pk({}),sc:pk({}),sc:pk({}),sc:pk({})" +
                           ",sc:pk({}))").format(k, *pk_hex_list)
            self.test_miniscript_expr(desc23, "Bdemus")

        # or_d(thresh_m(1,key),or_b(thresh_m(3,key,key,key),su:after(50)))
        # or_d(thresh_m(1,key),or_b(thresh_m(3,key,key,key),s:or_i(after(50),0)))
        k1 = 1
        k2 = 2
        after = 30
        pk_hex_list = self.get_pubkey_hex_list(4, keyhash_dict)
        desc24 = ("or_d(thresh_m({},{}),or_b(thresh_m({},{},{},{})," +
                         "su:after({})))").format(k1, pk_hex_list[0], k2,
                                                  pk_hex_list[1],
                                                  pk_hex_list[2],
                                                  pk_hex_list[3], after)
        desc24_alt = ("or_d(thresh_m({},{}),or_b(thresh_m({},{},{},{})," +
                      "s:or_i(after({}),0)))").format(k1, pk_hex_list[0], k2,
                                                      pk_hex_list[1],
                                                      pk_hex_list[2],
                                                      pk_hex_list[3], after)
        self.test_miniscript_expr(desc24, "Bdemu", desc24_alt)

        # uuj:and_v(v:thresh_m(2,key,key),after(10))
        # or_i(or_i(j:and_v(v:thresh_m(2,key,key),after(time)),0),0)
        k = 2
        delay = 40
        pk_hex_list = self.get_pubkey_hex_list(2, keyhash_dict)
        desc25 = "uuj:and_v(v:thresh_m({},{},{}),after({}))".format(
            k, pk_hex_list[0], pk_hex_list[1], delay)
        desc25_alt = ("or_i(or_i(j:and_v(v:thresh_m({},{},{}),after({})),0)," +
                      "0)").format(k, pk_hex_list[0], pk_hex_list[1], delay)
        self.test_miniscript_expr(desc25, "Bdsm", desc25_alt)

        # or_b(un:thresh_m(k,key,key),al:older(delay))
        # or_b(or_i(n:thresh_m(k,key,key),0),a:or_i(0,older(delay)))
        k = 2
        older = 45
        pk_hex_list = self.get_pubkey_hex_list(2, keyhash_dict)
        desc26 = "or_b(un:thresh_m({},{},{}),al:older({}))".format(
            k, pk_hex_list[0], pk_hex_list[1], older)
        desc26_alt = ("or_b(or_i(n:thresh_m({},{},{}),0)," +
                      "a:or_i(0,older({})))").format(k, pk_hex_list[0],
                                                     pk_hex_list[1],
                                                     older)
        self.test_miniscript_expr(desc26, "Bdu", desc26_alt)


if __name__ == '__main__':
    MiniscriptTest().main()
