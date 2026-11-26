#!/usr/bin/env python3
# Copyright (c) 2019-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test getdescriptorinfo RPC.
"""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.descriptors import descsum_create
from test_framework.extendedkey import ExtendedPrivateKey, ExtendedPublicKey
from test_framework.key import ECPubKey, H_POINT
from test_framework.util import (
    assert_equal,
    assert_raises_rpc_error,
)
from test_framework.wallet_util import generate_keypair


class DescriptorTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.extra_args = [["-disablewallet"]]
        self.wallet_names = []

    def test_desc(self, desc, isrange, issolvable, hasprivatekeys, expanded_descs=None):
        info = self.nodes[0].getdescriptorinfo(desc)
        assert_equal(info, self.nodes[0].getdescriptorinfo(descsum_create(desc)))
        if expanded_descs is not None:
            assert_equal(info["descriptor"], descsum_create(expanded_descs[0]))
            assert_equal(info["multipath_expansion"], [descsum_create(x) for x in expanded_descs])
        else:
            assert_equal(info['descriptor'], descsum_create(desc))
            assert "multipath_expansion" not in info
        assert_equal(info['isrange'], isrange)
        assert_equal(info['issolvable'], issolvable)
        assert_equal(info['hasprivatekeys'], hasprivatekeys)

    def test_verbose(self):
        self.log.info("Test verbose output")
        node = self.nodes[0]
        xpub = "tpubD6NzVbkrYhZ4WaWSyoBvQwbpLkojyoTZPRsgXELWz3Popb3qkjcJyJUGLnL4qHHoQvao8ESaAstxYSnhyswJ76uZPStJRJCTKvosUCJZL5B"
        xprv = "tprv8ZgxMBicQKsPd7Uf69XL1XwhmjHopUGep8GuEiJDZmbQz6o58LninorQAfcKZWARbtRtfnLcJ5MQ2AtHcQJCCRUcMRvmDUjyEmNUWwx8UbK"  # Private form of xpub.
        pk_a = "0279be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798"
        pk_b = "02c6047f9441ed7d6d3045406e95c07cd85c778e4b8cef3ca7abac09b95c709ee5"
        pk_c = "02f9308a019258c31049344f85f89d5229b531c845836f99b08601f113bce036f9"
        pk_d = "03fff97bd5755eeea420453a14355235d382f6472f8568a18b2f057a1460297556"

        desc = f"wsh(or_d(pk([deadbeef/48h/1h/0h/2h]{xpub}/0/*),and_v(v:pk({pk_b}),older(144))))"
        info = node.getdescriptorinfo(desc)
        assert "keys" not in info and "policy" not in info and "analysis" not in info
        assert_equal(node.getdescriptorinfo(desc, False), info)
        verbose = node.getdescriptorinfo(desc, True)
        assert_equal({k: v for k, v in verbose.items() if k not in ("keys", "policy", "analysis")}, info)
        assert_equal(verbose["keys"], [
            {"role": "ecdsa", "type": "bip32", "xpub": xpub, "derivation": "/0/*", "origin": {"master_fingerprint": "deadbeef", "path": "m/48h/1h/0h/2h"}},
            {"role": "ecdsa", "type": "pubkey", "pubkey": pk_b},
        ])
        assert_equal(verbose["policy"], [
            {"function": "wsh", "children": [{"node": 1, "role": "script"}]},
            {"fragment": "or_d", "children": [{"node": 2, "role": "left"}, {"node": 4, "role": "right"}]},
            {"fragment": "c:", "children": [{"node": 3, "role": "wrapped"}]},
            {"fragment": "pk_k", "keys": [0]},
            {"fragment": "and_v", "children": [{"node": 5, "role": "left"}, {"node": 8, "role": "right"}]},
            {"fragment": "v:", "children": [{"node": 6, "role": "wrapped"}]},
            {"fragment": "c:", "children": [{"node": 7, "role": "wrapped"}]},
            {"fragment": "pk_k", "keys": [1]},
            {"fragment": "older", "value": 144},
        ])

        self.log.info("Test verbose output for Taproot and MuSig")
        verbose = node.getdescriptorinfo(f"tr(musig({pk_a},{pk_b}),{{pk({pk_c[2:]}),and_v(v:pk({pk_d}),after(10))}})", True)
        assert_equal(verbose["keys"], [
            {"role": "taproot_internal", "type": "musig", "participants": [1, 2]},
            {"role": "musig_participant", "type": "pubkey", "pubkey": pk_a},
            {"role": "musig_participant", "type": "pubkey", "pubkey": pk_b},
            {"role": "tapscript", "type": "pubkey", "pubkey": pk_c[2:]},
            {"role": "tapscript", "type": "pubkey", "pubkey": pk_d},
        ])
        assert_equal(verbose["policy"], [
            {"function": "tr", "keys": [0], "children": [{"node": 1, "role": "tapleaf", "depth": 1}, {"node": 2, "role": "tapleaf", "depth": 1}]},
            {"function": "pk", "keys": [3]},
            {"fragment": "and_v", "children": [{"node": 3, "role": "left"}, {"node": 6, "role": "right"}]},
            {"fragment": "v:", "children": [{"node": 4, "role": "wrapped"}]},
            {"fragment": "c:", "children": [{"node": 5, "role": "wrapped"}]},
            {"fragment": "pk_k", "keys": [4]},
            {"fragment": "after", "value": 10},
        ])
        verbose = node.getdescriptorinfo(f"tr(musig({xpub}/0,{xpub}/1)/0/*,{{pk({xpub}/2),{{pk({xpub}/3),pk({xpub}/4)}}}})", True)
        assert_equal(verbose["keys"][0], {"role": "taproot_internal", "type": "musig", "participants": [1, 2], "derivation": "/0/*"})
        assert_equal(verbose["keys"][1], {"role": "musig_participant", "type": "bip32", "xpub": xpub, "derivation": "/0"})
        assert_equal([child["depth"] for child in verbose["policy"][0]["children"]], [1, 2, 2])

        self.log.info("Test verbose output for child roles")
        policy = node.getdescriptorinfo(f"wsh(andor(pk({xpub}/0),or_i(pk({xpub}/1),pk({xpub}/2)),thresh(1,pk({xpub}/3),s:pk({xpub}/4))))", True)["policy"]
        assert_equal([child["role"] for child in policy[1]["children"]], ["condition", "if_true", "if_false"])
        assert_equal(policy[4]["fragment"], "or_i")
        assert_equal([child["role"] for child in policy[4]["children"]], ["if_true", "if_false"])
        assert_equal(policy[9]["fragment"], "thresh")
        assert_equal(policy[9]["value"], 1)
        assert_equal([child["role"] for child in policy[9]["children"]], ["threshold_input", "threshold_input"])

        self.log.info("Test verbose output for hashes, sorted multisig, and opaque scripts")
        sha256 = "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"
        policy = node.getdescriptorinfo(f"wsh(and_v(v:sha256({sha256}),multi(1,{pk_a},{pk_b})))", True)["policy"]
        assert_equal(policy[3], {"fragment": "sha256", "data": sha256})
        assert_equal(policy[4], {"fragment": "multi", "value": 1, "keys": [0, 1]})
        verbose = node.getdescriptorinfo(f"sh(wsh(sortedmulti(1,{pk_b},{pk_a})))", True)
        assert_equal([key["pubkey"] for key in verbose["keys"]], [pk_b, pk_a])
        assert_equal(verbose["policy"][2], {"function": "sortedmulti", "value": 1, "keys": [0, 1]})
        verbose = node.getdescriptorinfo("raw(51)", True)
        assert_equal(verbose["keys"], [])
        assert_equal(verbose["policy"], [{"function": "raw", "data": "51"}])
        verbose = node.getdescriptorinfo("addr(bcrt1qw508d6qejxtdg4y5r3zarvary0c5xw7kygt080)", True)
        assert_equal(verbose["policy"], [{"function": "addr", "data": "0014751e76e8199196d454941c45d1b3a323f1433bd6"}])
        assert_equal(node.getdescriptorinfo(f"rawtr({pk_a[2:]})", True)["keys"], [{"role": "taproot_output", "type": "pubkey", "pubkey": pk_a[2:]}])

        self.log.info("Test that verbose output contains no private keys")
        public = node.getdescriptorinfo(f"wpkh([deadbeef/84h]{xpub}/0/*h)", True)
        private = node.getdescriptorinfo(f"wpkh([deadbeef/84h]{xprv}/0/*h)", True)
        assert_equal(private["hasprivatekeys"], True)
        assert_equal(private["keys"], public["keys"])
        assert_equal(private["policy"], public["policy"])
        assert_equal(public["keys"][0]["derivation"], "/0/*h")

        self.log.info("Test verbose output for an origin without a path and for MuSig participant origins")
        keys = node.getdescriptorinfo(f"wpkh([deadbeef]{xpub}/0/*)", True)["keys"]
        assert_equal(keys[0]["origin"], {"master_fingerprint": "deadbeef", "path": "m"})
        keys = node.getdescriptorinfo(f"tr(musig([deadbeef/1h]{pk_a},{pk_b}))", True)["keys"]
        assert_equal(keys[1]["origin"], {"master_fingerprint": "deadbeef", "path": "m/1h"})
        assert "origin" not in keys[0] and "origin" not in keys[2]

        self.log.info("Test that verbose output describes the first multipath descriptor")
        verbose = node.getdescriptorinfo(f"wpkh({xpub}/<0;1>/*)", True)
        assert_equal(verbose["keys"], node.getdescriptorinfo(f"wpkh({xpub}/0/*)", True)["keys"])

    def test_analysis(self):
        node = self.nodes[0]

        def analyze(desc):
            return node.getdescriptorinfo(desc, True)["analysis"]

        def recognized(shape, cosigners=(0, 1, 2), **fields):
            """A recognized 2-of-3 setup, unless fields say otherwise."""
            return {"recognized": True, "shape": shape, "sorted": True, "threshold": 2, "cosigners": list(cosigners), "issues": [], **fields}

        def unrecognized(*issues):
            """An unrecognized setup: only its issues, as (type, key IDs) pairs."""
            return {"recognized": False, "issues": [{"type": kind, "keys": keys} for kind, keys in issues]}

        # Cosigner account keys with their origins, as exported for a multisig setup.
        masters = [ExtendedPrivateKey.from_seed(bytes([i + 1]) * 32) for i in range(4)]
        a, b, c, outsider = (f"[{m._fingerprint().hex()}/48h/1h/0h/2h]{m.derive_path('m/48h/1h/0h/2h').pubkey().to_string()}" for m in masters)
        r = "/<0;1>/*"
        paths = f"{a}{r},{b}{r},{c}{r}"
        # The NUMS point H as an xpub at a given depth, as some tools write the key path.
        nums_key = ECPubKey()
        nums_key.set(bytes.fromhex(f"02{H_POINT}"))
        def nums_xpub(depth=0):
            return ExtendedPublicKey(nums_key, bytes(32), depth, bytes(4), child_num=0).to_string()

        self.log.info("Test multisig analysis of P2WSH setups")
        assert_equal(analyze(f"wsh(sortedmulti(2,{paths}))"), recognized("wsh_multi"))
        info = node.getdescriptorinfo(f"wsh(sortedmulti(2,{paths}))", True)
        assert_equal([info["keys"][id]["origin"]["path"] for id in info["analysis"]["cosigners"]], ["m/48h/1h/0h/2h"] * 3)
        # Unlike the multisig wizard, unsorted keys and a single path are recognized:
        # both are valid k-of-n setups, and multipath_expansion shows the paths.
        assert_equal(analyze(f"wsh(multi(2,{paths}))"), recognized("wsh_multi", sorted=False))
        assert_equal(analyze(f"wsh(sortedmulti(2,{a}/0/*,{b}/0/*,{c}/0/*))"), recognized("wsh_multi"))

        self.log.info("Test multisig analysis of Taproot setups")
        leaf = f"sortedmulti_a(2,{paths})"
        assert_equal(analyze(f"tr({H_POINT},{leaf})"), recognized("tr_multi_a", cosigners=[1, 2, 3], key_path="nums"))
        musig = analyze(f"tr(musig({a},{b},{c}){r},{leaf})")
        assert_equal(musig, recognized("tr_multi_a", cosigners=[4, 5, 6], key_path="musig"))
        # The order of musig() participants changes neither the analysis nor the addresses.
        assert_equal(analyze(f"tr(musig({c},{b},{a}){r},{leaf})"), musig)
        assert_equal(node.deriveaddresses(descsum_create(f"tr(musig({c},{b},{a}){r},{leaf})"), 0),
                     node.deriveaddresses(descsum_create(f"tr(musig({a},{b},{c}){r},{leaf})"), 0))
        # Unlike the multisig wizard, a musig() of `threshold` cosigners is recognized:
        # any two of them can already spend through the leaf. With a 3-of-3 leaf it can't.
        assert_equal(analyze(f"tr(musig({a},{b}){r},{leaf})"), recognized("tr_multi_a", cosigners=[3, 4, 5], key_path="musig"))
        assert_equal(analyze(f"tr(musig({a},{b}){r},sortedmulti_a(3,{paths}))"), unrecognized(("key_path_spendable", [0])))
        # A key path that an outsider, or one cosigner alone, can spend.
        assert_equal(analyze(f"tr({outsider}{r},{leaf})"), unrecognized(("key_path_spendable", [0])))
        assert_equal(analyze(f"tr({a}{r},{leaf})"), unrecognized(("key_path_spendable", [0])))
        # A musig() participant that only its holder can derive.
        assert_equal(analyze(f"tr(musig({a}/0h,{b},{c}){r},{leaf})"), unrecognized(("hardened_derivation", [1])))
        # A repeated musig() participant: A and B can already spend through the leaf, but not through a 3-of-3 one.
        assert_equal(analyze(f"tr(musig({a},{a},{b}){r},{leaf})"), unrecognized(("repeated_key", [1, 2])))
        assert_equal(analyze(f"tr(musig({a},{a},{b}){r},sortedmulti_a(3,{paths}))"), unrecognized(("repeated_key", [1, 2]), ("key_path_spendable", [0])))
        # musig() participants with the same root key, deriving different keys.
        assert_equal(analyze(f"tr(musig({a}/0/*,{a}/1/*,{b}/0/*),{leaf})"), unrecognized(("repeated_key", [1, 2])))
        # musig() participants that derive the same key: A and its grandchild xpub,
        # which the leaf can't link to A.
        a00 = masters[0].derive_path('m/48h/1h/0h/2h/0/0').pubkey().to_string()
        assert_equal(analyze(f"tr(musig({a}/0/0/*,{a00}/*),sortedmulti_a(2,{a}/1/*,{a00}/1/*,{c}/0/*))"), unrecognized(("repeated_key", [1, 2])))
        # An internal key past the maximum BIP32 depth has no addresses. A musig()
        # aggregate starts at depth 0, so it derives 255 steps at most.
        for internal, derivable in [(f"{nums_xpub(254)}/*", True), (f"{nums_xpub(255)}/*", False),
                                    (f"musig({a},{b},{c})" + "/0" * 254 + "/*", True), (f"musig({a},{b},{c})" + "/0" * 255 + "/*", False)]:
            desc = f"tr({internal},{leaf})"
            if derivable:
                assert_equal(analyze(desc)["recognized"], True)
                node.deriveaddresses(descsum_create(desc), 0)
            else:
                assert_equal(analyze(desc), unrecognized(("not_derivable", [0])))
                assert_raises_rpc_error(-5, "Cannot derive script without private keys", node.deriveaddresses, descsum_create(desc), range=0)

        self.log.info("Test multisig analysis of unsupported shapes")
        for desc in [f"wpkh({a}{r})",
                     f"sh(sortedmulti(2,{paths}))",
                     f"tr(musig({a},{b},{c}){r},{{{leaf},pk({a}{r})}})"]:
            assert_equal(analyze(desc), unrecognized(("unsupported_shape", [])))

        self.log.info("Test multisig analysis of repeated and unusable cosigner keys")
        # The same key twice lets its holder alone meet a threshold of 2.
        assert_equal(analyze(f"wsh(sortedmulti(2,{a}{r},{a}{r},{c}{r}))"), unrecognized(("repeated_key", [0, 1])))
        assert_equal(analyze(f"wsh(sortedmulti(2,{a}/0/*,{a}/1/*,{c}/0/*))"), unrecognized(("repeated_key", [0, 1])))
        # A's own child xpub at /0 is A's key too, even with a different suffix.
        a0 = f"[{masters[0]._fingerprint().hex()}/48h/1h/0h/2h/0]{masters[0].derive_path('m/48h/1h/0h/2h/0').pubkey().to_string()}"
        assert_equal(analyze(f"wsh(sortedmulti(2,{a}/0/*,{a0}/1/*,{c}/0/*))"), unrecognized(("repeated_key", [0, 1])))
        # A's account key is a hardened child of its parent xpub, which can't be derived without the private key.
        a_parent = f"[{masters[0]._fingerprint().hex()}/48h/1h/0h]{masters[0].derive_path('m/48h/1h/0h').pubkey().to_string()}"
        assert_equal(analyze(f"wsh(sortedmulti(2,{a_parent}/0/*,{a}/0/*,{c}/0/*))"), recognized("wsh_multi"))
        # Slots linked through a chain are one group: the first and last share a root,
        # the last two derive the same key, the first two have nothing in common.
        a12 = f"[{masters[0]._fingerprint().hex()}/48h/1h/0h/2h/1/2]{masters[0].derive_path('m/48h/1h/0h/2h/1/2').pubkey().to_string()}"
        assert_equal(analyze(f"wsh(sortedmulti(2,{a}/5/*,{a12}/*,{a}/1/2/*))"), unrecognized(("repeated_key", [0, 1, 2])))
        assert_equal(analyze(f"tr({H_POINT},sortedmulti_a(2,{a}{r},{b}{r},{a}{r}))"), unrecognized(("repeated_key", [1, 3])))
        pubkey = "02f9308a019258c31049344f85f89d5229b531c845836f99b08601f113bce036f9"
        assert_equal(analyze(f"wsh(sortedmulti(2,{a}{r},[deadbeef/48h/1h/0h/2h]{pubkey},{c}{r}))"), unrecognized(("not_extended_key", [1])))
        assert_equal(analyze(f"wsh(sortedmulti(2,{a}{r},{b}/0/5,{c}{r}))"), unrecognized(("not_ranged", [1])))
        assert_equal(analyze(f"wsh(sortedmulti(2,{a}{r},{b}/0/*h,{c}{r}))"), unrecognized(("hardened_derivation", [1])))
        # B's xpub claiming the maximum BIP32 depth, from which no address can be derived.
        b_key = masters[1].derive_path('m/48h/1h/0h/2h').pubkey()
        b_deep = ExtendedPublicKey(b_key.pubkey, b_key.chaincode, depth=255, parent_fingerprint_bytes=b_key.parent_fingerprint_bytes, child_num=b_key.child_num).to_string()
        assert_equal(analyze(f"wsh(sortedmulti(2,{a}{r},{b_deep}{r},{c}{r}))"), unrecognized(("not_derivable", [1])))
        # No one can sign for H, which would leave a 2-of-2 unspendable.
        assert_equal(analyze(f"wsh(sortedmulti(2,{a}{r},{nums_xpub()}{r}))"), unrecognized(("unspendable_key", [1])))
        # One path is a recognized setup, but the other needs a private key. The issue
        # is returned whichever path comes first.
        for path in ["/<0;1h>/*", "/<0h;1>/*"]:
            assert_equal(analyze(f"wsh(sortedmulti(2,{a}{r},{b}{path},{c}{r}))"), unrecognized(("hardened_derivation", [1]), ("branches_differ", [])))

    def run_test(self):
        assert_raises_rpc_error(-1, 'getdescriptorinfo', self.nodes[0].getdescriptorinfo)
        # cli handles wrong types differently
        if not self.options.usecli:
            assert_raises_rpc_error(-3, 'JSON value of type number is not of expected type string', self.nodes[0].getdescriptorinfo, 1)
        assert_raises_rpc_error(-5, "'' is not a valid descriptor function", self.nodes[0].getdescriptorinfo, "")
        assert_raises_rpc_error(-5, "pk(): Key ' 0279be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798' is invalid due to whitespace", self.nodes[0].getdescriptorinfo, "pk( 0279be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798)")
        assert_raises_rpc_error(-5, "pk(): Key '0279be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798 ' is invalid due to whitespace", self.nodes[0].getdescriptorinfo, "pk(0279be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798 )")
        assert_raises_rpc_error(-5, "Multi: Key ' 03a0434d9e47f3c86235477c7b1ae6ae5d3442d49b1943c2b752a68e2a47e247c7' is invalid due to whitespace", self.nodes[0].getdescriptorinfo, "wsh(multi(2, 03a0434d9e47f3c86235477c7b1ae6ae5d3442d49b1943c2b752a68e2a47e247c7,03774ae7f858a9411e5ef4246b70c65aac5649980be5c17891bbec17895da008cb,03d01115d548e7561b15c38f004d734633687cf4419620095bc5b0f47070afe85a))")
        assert_raises_rpc_error(-5, "Multi: Key ' 03774ae7f858a9411e5ef4246b70c65aac5649980be5c17891bbec17895da008cb' is invalid due to whitespace", self.nodes[0].getdescriptorinfo, "wsh(multi(2,03a0434d9e47f3c86235477c7b1ae6ae5d3442d49b1943c2b752a68e2a47e247c7, 03774ae7f858a9411e5ef4246b70c65aac5649980be5c17891bbec17895da008cb,03d01115d548e7561b15c38f004d734633687cf4419620095bc5b0f47070afe85a))")
        priv_key = generate_keypair(wif=True)[0]
        assert_raises_rpc_error(-5, f"pk(): Key ' {priv_key}' is invalid due to whitespace", self.nodes[0].getdescriptorinfo, f"pk( {priv_key})")

        # P2PK output with the specified public key.
        self.test_desc('pk(0279be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798)', isrange=False, issolvable=True, hasprivatekeys=False)
        # P2PKH output with the specified public key.
        self.test_desc('pkh(02c6047f9441ed7d6d3045406e95c07cd85c778e4b8cef3ca7abac09b95c709ee5)', isrange=False, issolvable=True, hasprivatekeys=False)
        # P2WPKH output with the specified public key.
        self.test_desc('wpkh(02f9308a019258c31049344f85f89d5229b531c845836f99b08601f113bce036f9)', isrange=False, issolvable=True, hasprivatekeys=False)
        # P2SH-P2WPKH output with the specified public key.
        self.test_desc('sh(wpkh(03fff97bd5755eeea420453a14355235d382f6472f8568a18b2f057a1460297556))', isrange=False, issolvable=True, hasprivatekeys=False)
        # Any P2PK, P2PKH, P2WPKH, or P2SH-P2WPKH output with the specified public key.
        self.test_desc('combo(0279be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798)', isrange=False, issolvable=True, hasprivatekeys=False)
        # An (overly complicated) P2SH-P2WSH-P2PKH output with the specified public key.
        self.test_desc('sh(wsh(pkh(02e493dbf1c10d80f3581e4904930b1404cc6c13900ee0758474fa94abe8c4cd13)))', isrange=False, issolvable=True, hasprivatekeys=False)
        # A bare *1-of-2* multisig output with keys in the specified order.
        self.test_desc('multi(1,022f8bde4d1a07209355b4a7250a5c5128e88b84bddc619ab7cba8d569b240efe4,025cbdf0646e5db4eaa398f365f2ea7a0e3d419b7e0330e39ce92bddedcac4f9bc)', isrange=False, issolvable=True, hasprivatekeys=False)
        # A P2SH *2-of-2* multisig output with keys in the specified order.
        self.test_desc('sh(multi(2,022f01e5e15cca351daff3843fb70f3c2f0a1bdd05e5af888a67784ef3e10a2a01,03acd484e2f0c7f65309ad178a9f559abde09796974c57e714c35f110dfc27ccbe))', isrange=False, issolvable=True, hasprivatekeys=False)
        # A P2WSH *2-of-3* multisig output with keys in the specified order.
        self.test_desc('wsh(multi(2,03a0434d9e47f3c86235477c7b1ae6ae5d3442d49b1943c2b752a68e2a47e247c7,03774ae7f858a9411e5ef4246b70c65aac5649980be5c17891bbec17895da008cb,03d01115d548e7561b15c38f004d734633687cf4419620095bc5b0f47070afe85a))', isrange=False, issolvable=True, hasprivatekeys=False)
        # A P2SH-P2WSH *1-of-3* multisig output with keys in the specified order.
        self.test_desc('sh(wsh(multi(1,03f28773c2d975288bc7d1d205c3748651b075fbc6610e58cddeeddf8f19405aa8,03499fdf9e895e719cfd64e67f07d38e3226aa7b63678949e6e49b241a60e823e4,02d7924d4f7d43ea965a465ae3095ff41131e5946f3c85f79e44adbcf8e27e080e)))', isrange=False, issolvable=True, hasprivatekeys=False)
        # A P2PK output with the public key of the specified xpub.
        self.test_desc('pk(tpubD6NzVbkrYhZ4WaWSyoBvQwbpLkojyoTZPRsgXELWz3Popb3qkjcJyJUGLnL4qHHoQvao8ESaAstxYSnhyswJ76uZPStJRJCTKvosUCJZL5B)', isrange=False, issolvable=True, hasprivatekeys=False)
        # A P2PKH output with child key *1'/2* of the specified xpub.
        self.test_desc("pkh(tpubD6NzVbkrYhZ4WaWSyoBvQwbpLkojyoTZPRsgXELWz3Popb3qkjcJyJUGLnL4qHHoQvao8ESaAstxYSnhyswJ76uZPStJRJCTKvosUCJZL5B/1h/2)", isrange=False, issolvable=True, hasprivatekeys=False)
        # A set of P2PKH outputs, but additionally specifies that the specified xpub is a child of a master with fingerprint `d34db33f`, and derived using path `44'/0'/0'`.
        self.test_desc("pkh([d34db33f/44h/0h/0h]tpubD6NzVbkrYhZ4WaWSyoBvQwbpLkojyoTZPRsgXELWz3Popb3qkjcJyJUGLnL4qHHoQvao8ESaAstxYSnhyswJ76uZPStJRJCTKvosUCJZL5B/1/*)", isrange=True, issolvable=True, hasprivatekeys=False)
        # A set of *1-of-2* P2WSH multisig outputs where the first multisig key is the *1/0/`i`* child of the first specified xpub and the second multisig key is the *0/0/`i`* child of the second specified xpub, and `i` is any number in a configurable range (`0-1000` by default).
        self.test_desc("wsh(multi(1,tpubD6NzVbkrYhZ4WaWSyoBvQwbpLkojyoTZPRsgXELWz3Popb3qkjcJyJUGLnL4qHHoQvao8ESaAstxYSnhyswJ76uZPStJRJCTKvosUCJZL5B/1/0/*,tpubD6NzVbkrYhZ4WaWSyoBvQwbpLkojyoTZPRsgXELWz3Popb3qkjcJyJUGLnL4qHHoQvao8ESaAstxYSnhyswJ76uZPStJRJCTKvosUCJZL5B/0/0/*))", isrange=True, issolvable=True, hasprivatekeys=False)
        # A multipath descriptor
        self.test_desc("wpkh(tpubD6NzVbkrYhZ4WaWSyoBvQwbpLkojyoTZPRsgXELWz3Popb3qkjcJyJUGLnL4qHHoQvao8ESaAstxYSnhyswJ76uZPStJRJCTKvosUCJZL5B/<0;1>/*)", isrange=True, issolvable=True, hasprivatekeys=False,
            expanded_descs=["wpkh(tpubD6NzVbkrYhZ4WaWSyoBvQwbpLkojyoTZPRsgXELWz3Popb3qkjcJyJUGLnL4qHHoQvao8ESaAstxYSnhyswJ76uZPStJRJCTKvosUCJZL5B/0/*)", "wpkh(tpubD6NzVbkrYhZ4WaWSyoBvQwbpLkojyoTZPRsgXELWz3Popb3qkjcJyJUGLnL4qHHoQvao8ESaAstxYSnhyswJ76uZPStJRJCTKvosUCJZL5B/1/*)"])
        self.test_desc("wsh(multi(1,tpubD6NzVbkrYhZ4WaWSyoBvQwbpLkojyoTZPRsgXELWz3Popb3qkjcJyJUGLnL4qHHoQvao8ESaAstxYSnhyswJ76uZPStJRJCTKvosUCJZL5B/<1;2>/0/*,tpubD6NzVbkrYhZ4WaWSyoBvQwbpLkojyoTZPRsgXELWz3Popb3qkjcJyJUGLnL4qHHoQvao8ESaAstxYSnhyswJ76uZPStJRJCTKvosUCJZL5B/<2;3>/0/*))", isrange=True, issolvable=True, hasprivatekeys=False,
            expanded_descs=["wsh(multi(1,tpubD6NzVbkrYhZ4WaWSyoBvQwbpLkojyoTZPRsgXELWz3Popb3qkjcJyJUGLnL4qHHoQvao8ESaAstxYSnhyswJ76uZPStJRJCTKvosUCJZL5B/1/0/*,tpubD6NzVbkrYhZ4WaWSyoBvQwbpLkojyoTZPRsgXELWz3Popb3qkjcJyJUGLnL4qHHoQvao8ESaAstxYSnhyswJ76uZPStJRJCTKvosUCJZL5B/2/0/*))", "wsh(multi(1,tpubD6NzVbkrYhZ4WaWSyoBvQwbpLkojyoTZPRsgXELWz3Popb3qkjcJyJUGLnL4qHHoQvao8ESaAstxYSnhyswJ76uZPStJRJCTKvosUCJZL5B/2/0/*,tpubD6NzVbkrYhZ4WaWSyoBvQwbpLkojyoTZPRsgXELWz3Popb3qkjcJyJUGLnL4qHHoQvao8ESaAstxYSnhyswJ76uZPStJRJCTKvosUCJZL5B/3/0/*))"])

        self.test_verbose()
        self.test_analysis()


if __name__ == '__main__':
    DescriptorTest(__file__).main()
