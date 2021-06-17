#!/usr/bin/env python3
# Copyright (c) 2021 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test generation and spending of P2TR addresses."""

import random

from decimal import Decimal
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal
from test_framework.descriptors import descsum_create
from test_framework.script import (CScript, OP_CHECKSIG, taproot_construct)
from test_framework.segwit_addr import encode_segwit_address

# xprvs/xpubs, and m/* derived x-only pubkeys (created using independent implementation)
KEYS = [
    {
        "xprv": "tprv8ZgxMBicQKsPeNLUGrbv3b7qhUk1LQJZAGMuk9gVuKh9sd4BWGp1eMsehUni6qGb8bjkdwBxCbgNGdh2bYGACK5C5dRTaif9KBKGVnSezxV",
        "xpub": "tpubD6NzVbkrYhZ4XqNGAWGWSzmxGWFwVjVTjZxh2fioKbVYi7Jx8fdbprVWsdW7mHwqjchBVas8TLZG4Xwuz4RKU4iaCqiCvoSkFCzQptqk5Y1",
        "pubs": [
            "83d8ee77a0f3a32a5cea96fd1624d623b836c1e5d1ac2dcde46814b619320c18",
            "a30253b018ea6fca966135bf7dd8026915427f24ccf10d4e03f7870f4128569b",
            "a61e5749f2f3db9dc871d7b187e30bfd3297eea2557e9be99897ea8ff7a29a21",
            "8110cf482f66dc37125e619d73075af932521724ffc7108309e88f361efe8c8a",
        ]
    },
    {
        "xprv": "tprv8ZgxMBicQKsPe98QUPieXy5KFPVjuZNpcC9JY7K7buJEm8nWvJogK4kTda7eLjK9U4PnMNbSjEkpjDJazeBZ4rhYNYD7N6GEdaysj1AYSb5",
        "xpub": "tpubD6NzVbkrYhZ4XcACN3PEwNjRpR1g4tZjBVk5pdMR2B6dbd3HYhdGVZNKofAiFZd9okBserZvv58A6tBX4pE64UpXGNTSesfUW7PpW36HuKz",
        "pubs": [
            "f95886b02a84928c5c15bdca32784993105f73de27fa6ad8c1a60389b999267c",
            "71522134160685eb779857033bfc84c7626f13556154653a51dd42619064e679",
            "48957b4158b2c5c3f4c000f51fd2cf0fd5ff8868ebfb194256f5e9131fc74bd8",
            "086dda8139b3a84944010648d2b674b70447be3ae59322c09a4907bc80be62c1",
        ]
    },
    {
        "xprv": "tprv8ZgxMBicQKsPe3ZJmcj9aJ2EPZJYYCh6Lp3v82p75wspgaXmtDZ2RBtkAtWcGnW2VQDzMHQPBkCKMoYTqh1RfJKjv4PcmWVR7KqTpjsdboN",
        "xpub": "tpubD6NzVbkrYhZ4XWb6fGPjyhgLxapUhXszv7ehQYrQWDgDX4nYWcNcbgWcM2RhYo9s2mbZcfZJ8t5LzYcr24FK79zVybsw5Qj3Rtqug8jpJMy",
        "pubs": [
            "9fa5ffb68821cf559001caa0577eeea4978b29416def328a707b15e91701a2f7",
            "8a104c54cd34acba60c97dd8f1f7abc89ba9587afd88dc928e91aca7b1c50d20",
            "13ba6b252a4eb5ef31d39cb521724cdab19a698323f5c17093f28fb1821d052f",
            "f6c2b4863fd5ba1ba09e3a890caed8b75ffbe013ebab31a06ab87cd6f72506af",
        ]
    },
    {
        "xprv": "tprv8ZgxMBicQKsPdKziibn63Rm6aNzp7dSjDnufZMStXr71Huz7iihCRpbZZZ6Voy5HyuHCWx6foHMipzMzUq4tZrtkZ24DJwz5EeNWdsuwX5h",
        "xpub": "tpubD6NzVbkrYhZ4Wo2WcFSgSqRD9QWkGxddo6WSqsVBx7uQ8QEtM7WncKDRjhFEexK119NigyCsFygA4b7sAPQxqebyFGAZ9XVV1BtcgNzbCRR",
        "pubs": [
            "03a669ea926f381582ec4a000b9472ba8a17347f5fb159eddd4a07036a6718eb",
            "bbf56b14b119bccafb686adec2e3d2a6b51b1626213590c3afa815d1fd36f85d",
            "2994519e31bbc238a07d82f85c9832b831705d2ee4a2dbb477ecec8a3f570fe5",
            "68991b5c139a4c479f8c89d6254d288c533aefc0c5b91fac6c89019c4de64988",
        ]
    },
    {
        "xprv": "tprv8ZgxMBicQKsPen4PGtDwURYnCtVMDejyE8vVwMGhQWfVqB2FBPdekhTacDW4vmsKTsgC1wsncVqXiZdX2YFGAnKoLXYf42M78fQJFzuDYFN",
        "xpub": "tpubD6NzVbkrYhZ4YF6BAXtXsqCtmv1HNyvsoSXHDsJzpnTtffH1onTEwC5SnLzCHPKPebh2i7Gxvi9kJNADcpuSmH8oM3rCYcHVtdXHjpYoKnX",
        "pubs": [
            "aba457d16a8d59151c387f24d1eb887efbe24644c1ee64b261282e7baebdb247",
            "c8558b7caf198e892032d91f1a48ee9bdc25462b83b4d0ac62bb7fb2a0df630e",
            "8a4bcaba0e970685858d133a4d0079c8b55bbc755599e212285691eb779ce3dc",
            "b0d68ada13e0d954b3921b88160d4453e9c151131c2b7c724e08f538a666ceb3",
        ]
    },
    {
        "xprv": "tprv8ZgxMBicQKsPd91vCgRmbzA13wyip2RimYeVEkAyZvsEN5pUSB3T43SEBxPsytkxb42d64W2EiRE9CewpJQkzR8HKHLV8Uhk4dMF5yRPaTv",
        "xpub": "tpubD6NzVbkrYhZ4Wc3i6L6N1Pp7cyVeyMcdLrFGXGDGzCfdCa5F4Zs3EY46N72Ws8QDEUYBVwXfDfda2UKSseSdU1fsBegJBhGCZyxkf28bkQ6",
        "pubs": [
            "9b4d495b74887815a1ff623c055c6eac6b6b2e07d2a016d6526ebac71dd99744",
            "8e971b781b7ce7ab742d80278f2dfe7dd330f3efd6d00047f4a2071f2e7553cb",
            "b811d66739b9f07435ccda907ec5cd225355321c35e0a7c7791232f24cf10632",
            "4cd27a5552c272bc80ba544e9cc6340bb906969f5e7a1510b6cef9592683fbc9",
        ]
    },
    {
        "xprv": "tprv8ZgxMBicQKsPdEhLRxxwzTv2t18j7ruoffPeqAwVA2qXJ2P66RaMZLUWQ85SjoA7xPxdSgCB9UZ72m65qbnaLPtFTfHVP3MEmkpZk1Bv8RT",
        "xpub": "tpubD6NzVbkrYhZ4Whj8KcdYPsa9T2efHC6iExzS7gynaJdv8WdripPwjq6NaH5gQJGrLmvUwHY1smhiakUosXNDTEa6qfKUQdLKV6DJBre6XvQ",
        "pubs": [
            "d0c19def28bb1b39451c1a814737615983967780d223b79969ba692182c6006b",
            "cb1d1b1dc62fec1894d4c3d9a1b6738e5ff9c273a64f74e9ab363095f45e9c47",
            "245be588f41acfaeb9481aa132717db56ee1e23eb289729fe2b8bde8f9a00830",
            "5bc4ad6d6187fa82728c85a073b428483295288f8aef5722e47305b5872f7169",
        ]
    },
    {
        "xprv": "tprv8ZgxMBicQKsPcxbqxzcMAwQpiCD8x6qaZEJTxdKxw4w9GuMzDACTD9yhEsHGfqQcfYX4LivosLDDngTykYEp9JnTdcqY7cHqU8PpeFFKyV3",
        "xpub": "tpubD6NzVbkrYhZ4WRddreGwaM4wHDj57S2V8XuFF9NGMLjY7PckqZ23PebZR1wGA4w84uX2vZphdZVsnREjij1ibYjEBTaTVQCEZCLs4xUDapx",
        "pubs": [
            "065cc1b92bd99e5a3e626e8296a366b2d132688eb43aea19bc14fd8f43bf07fb",
            "5b95633a7dda34578b6985e6bfd85d83ec38b7ded892a9b74a3d899c85890562",
            "dc86d434b9a34495c8e845b969d51f80d19a8df03b400353ffe8036a0c22eb60",
            "06c8ffde238745b29ae8a97ae533e1f3edf214bba6ec58b5e7b9451d1d61ec19",
        ]
    },
    {
        "xprv": "tprv8ZgxMBicQKsPe6zLoU8MTTXgsdJVNBErrYGpoGwHf5VGvwUzdNc7NHeCSzkJkniCxBhZWujXjmD4HZmBBrnr3URgJjM6GxRgMmEhLdqNTWG",
        "xpub": "tpubD6NzVbkrYhZ4Xa28h7nwrsBoSepRXWRmRqsc5nyb5MHfmRjmFmRhYnG4d9dC7uxixN5AfsEv1Lz3mCAuWvERyvPgKozHUVjfo8EG6foJGy7",
        "pubs": [
            "d826a0a53abb6ffc60df25b9c152870578faef4b2eb5a09bdd672bbe32cdd79b",
            "939365e0359ff6bc6f6404ee220714c5d4a0d1e36838b9e2081ede217674e2ba",
            "4e8767edcf7d3d90258cfbbea01b784f4d2de813c4277b51279cf808bac410a2",
            "d42a2c280940bfc6ede971ae72cde2e1df96c6da7dab06a132900c6751ade208",
        ]
    },
    {
        "xprv": "tprv8ZgxMBicQKsPeB5o5oCsN2dVxM2mtJiYERQEBRc4JNwC1DFGYaEdNkmh8jJYVPU76YhkFoRoWTdh1p3yQGykG8TfDW34dKgrgSx28gswUyL",
        "xpub": "tpubD6NzVbkrYhZ4Xe7aySsTmSHcXNYi3duSoj11TweMiejaqhW3Ay4DZFPZJses4sfpk4b9VHRhn8v4cKTMjugMM3hqXcqSSmRdiW8QvASXjfY",
        "pubs": [
            "e360564b2e0e8d06681b6336a29d0750210e8f34afd9afb5e6fd5fe6dba26c81",
            "76b4900f00a1dcce463b6d8e02b768518fce4f9ecd6679a13ad78ea1e4815ad3",
            "5575556e263c8ed52e99ab02147cc05a738869afe0039911b5a60a780f4e43d2",
            "593b00e2c8d4bd6dda0fd9e238888acf427bb4e128887fd5a40e0e9da78cbc01",
        ]
    },
    {
        "xprv": "tprv8ZgxMBicQKsPfEH6jHemkGDjZRnAaKFJVGH8pQU638E6SdbX9hxit1tK2sfFPfL6KS7v8FfUKxstbfEpzSymbdfBM9Y5UkrxErF9fJaKLK3",
        "xpub": "tpubD6NzVbkrYhZ4YhJtcwKN9fsr8TJ6jeSD4Zsv6vWPTQ2VH7rHn6nK4WWBCzKK7FkdVVwm3iztCU1UmStY4hX6gRbBmp9UzK9C59dQEzeXS12",
        "pubs": [
            "7631cacec3343052d87ef4d0065f61dde82d7d2db0c1cc02ef61ef3c982ea763",
            "c05e44a9e735d1b1bef62e2c0d886e6fb4923b2649b67828290f5cacc51c71b7",
            "b33198b20701afe933226c92fd0e3d51d3f266f1113d864dbd026ae3166ef7f2",
            "f99643ac3f4072ee4a949301e86963a9ca0ad57f2ef29f6b84fda037d7cac85b",
        ]
    },
    {
        "xprv": "tprv8ZgxMBicQKsPdNWU38dT6aGxtqJR4oYS5kPpLVBcuKiiu7gqTYqMMqhUG6DP7pPahzPQu36sWSmeLCP1C4AwqcR5FX2RyRoZfd4B8pAnSdX",
        "xpub": "tpubD6NzVbkrYhZ4WqYFvnJ3Vyw5TrpME8jLf3zbd1DvKbX7jbwc5wewYLKLSFRzZWV6hZj7XhsXAy7fhE5jB25DiWyNM3ztXbsXHRVCrp5BiPY",
        "pubs": [
            "2258b1c3160be0864a541854eec9164a572f094f7562628281a8073bb89173a7",
            "83df59d0a5c951cdd62b7ab225a62079f48d2a333a86e66c35420d101446e92e",
            "2a654bf234d819055312f9ca03fad5836f9163b09cdd24d29678f694842b874a",
            "aa0334ab910047387c912a21ec0dab806a47ffa38365060dbc5d47c18c6e66e7",
        ]
    },
    {
        "xprv": "tprv8mGPkMVz5mZuJDnC2NjjAv7E9Zqa5LCgX4zawbZu5nzTtLb5kGhPwycX4H1gtW1f5ZdTKTNtQJ61hk71F2TdcQ93EFDTpUcPBr98QRji615",
        "xpub": "tpubDHxRtmYEE9FaBgoyv2QKaKmLibMWEfPb6NbNE7cCW4nripqrNfWz8UEPEPbHCrakwLvwFfsqoaf4pjX4gWStp4nECRf1QwBKPkLqnY8pHbj",
        "pubs": [
            "00a9da96087a72258f83b338ef7f0ea8cbbe05da5f18f091eb397d1ecbf7c3d3",
            "b2749b74d51a78f5fe3ebb3a7c0ff266a468cade143dfa265c57e325177edf00",
            "6b8747a6bbe4440d7386658476da51f6e49a220508a7ec77fe7bccc3e7baa916",
            "4674bf4d9ebbe01bf0aceaca2472f63198655ecf2df810f8d69b38421972318e",
        ]
    }
]

CHANGE_XPRV = "tprv8ZgxMBicQKsPcyDrWwiecVnTtFmfRwbfFqEfR4ZGWvq5aTTwLBWmAm5zrbMcYtb9gQNFfhRfqhhrBG37U3nhmXxEgeEPBJGHAPrHCrAd1WX"
CHANGE_XPUB = "tpubD6NzVbkrYhZ4WSFeQbPF1uSaTHHbbGnZq8qShabZwCdUQwihxaLMMFhs2kidGF2qrRKiQVqw8VoyuTHj1bZqmMXMeciaU1gBjWA1sim2zUB"

# Point with no known discrete log.
H_POINT = "50929b74c1a04954b78b4b6035e97a5e078a5a0f28ec96d547bfee9ace803ac0"


def key(hex_key):
    """Construct an x-only pubkey from its hex representation."""
    return bytes.fromhex(hex_key)

def pk(hex_key):
    """Construct a script expression for taproot_construct for pk(hex_key)."""
    return (None, CScript([bytes.fromhex(hex_key), OP_CHECKSIG]))

def compute_taproot_address(pubkey, scripts):
    """Compute the address for a taproot output with given inner key and scripts."""
    tap = taproot_construct(pubkey, scripts)
    assert tap.scriptPubKey[0] == 0x51
    assert tap.scriptPubKey[1] == 0x20
    return encode_segwit_address("bcrt", 1, tap.scriptPubKey[2:])

class WalletTaprootTest(BitcoinTestFramework):
    """Test generation and spending of P2TR address outputs."""

    def set_test_params(self):
        self.num_nodes = 3
        self.setup_clean_chain = True
        self.extra_args = [['-keypool=100'], ['-keypool=100'], ["-vbparams=taproot:1:1"]]
        self.supports_cli = False

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()
        self.skip_if_no_sqlite()

    def setup_network(self):
        self.setup_nodes()

    def init_wallet(self, i):
        pass

    @staticmethod
    def rand_keys(n):
        ret = []
        idxes = set()
        for _ in range(n):
            while True:
                i = random.randrange(len(KEYS))
                if not i in idxes:
                    break
            idxes.add(i)
            ret.append(KEYS[i])
        return ret

    @staticmethod
    def make_desc(pattern, privmap, keys, pub_only = False):
        pat = pattern.replace("$H", H_POINT)
        for i in range(len(privmap)):
            if privmap[i] and not pub_only:
                pat = pat.replace("$%i" % (i + 1), keys[i]['xprv'])
            else:
                pat = pat.replace("$%i" % (i + 1), keys[i]['xpub'])
        return descsum_create(pat)

    @staticmethod
    def make_addr(treefn, keys, i):
        args = []
        for j in range(len(keys)):
            args.append(keys[j]['pubs'][i])
        return compute_taproot_address(*treefn(*args))

    def do_test_addr(self, comment, pattern, privmap, treefn, keys):
        self.log.info("Testing %s address derivation" % comment)
        desc = self.make_desc(pattern, privmap, keys, False)
        desc_pub = self.make_desc(pattern, privmap, keys, True)
        assert_equal(self.nodes[0].getdescriptorinfo(desc)['descriptor'], desc_pub)
        result = self.addr_gen.importdescriptors([{"desc": desc_pub, "active": True, "timestamp": "now"}])
        assert(result[0]['success'])
        for i in range(4):
            addr_g = self.addr_gen.getnewaddress(address_type='bech32')
            if treefn is not None:
                addr_r = self.make_addr(treefn, keys, i)
                assert_equal(addr_g, addr_r)

        # tr descriptors cannot be imported when Taproot is not active
        result = self.privs_tr_enabled.importdescriptors([{"desc": desc, "timestamp": "now"}])
        assert(result[0]["success"])
        result = self.pubs_tr_enabled.importdescriptors([{"desc": desc_pub, "timestamp": "now"}])
        assert(result[0]["success"])
        if desc.startswith("tr"):
            result = self.privs_tr_disabled.importdescriptors([{"desc": desc, "timestamp": "now"}])
            assert(not result[0]["success"])
            assert_equal(result[0]["error"]["code"], -4)
            assert_equal(result[0]["error"]["message"], "Cannot import tr() descriptor when Taproot is not active")
            result = self.pubs_tr_disabled.importdescriptors([{"desc": desc_pub, "timestamp": "now"}])
            assert(not result[0]["success"])
            assert_equal(result[0]["error"]["code"], -4)
            assert_equal(result[0]["error"]["message"], "Cannot import tr() descriptor when Taproot is not active")

    def do_test_sendtoaddress(self, comment, pattern, privmap, treefn, keys_pay, keys_change):
        self.log.info("Testing %s through sendtoaddress" % comment)
        desc_pay = self.make_desc(pattern, privmap, keys_pay)
        desc_change = self.make_desc(pattern, privmap, keys_change)
        desc_pay_pub = self.make_desc(pattern, privmap, keys_pay, True)
        desc_change_pub = self.make_desc(pattern, privmap, keys_change, True)
        assert_equal(self.nodes[0].getdescriptorinfo(desc_pay)['descriptor'], desc_pay_pub)
        assert_equal(self.nodes[0].getdescriptorinfo(desc_change)['descriptor'], desc_change_pub)
        result = self.rpc_online.importdescriptors([{"desc": desc_pay, "active": True, "timestamp": "now"}])
        assert(result[0]['success'])
        result = self.rpc_online.importdescriptors([{"desc": desc_change, "active": True, "timestamp": "now", "internal": True}])
        assert(result[0]['success'])
        for i in range(4):
            addr_g = self.rpc_online.getnewaddress(address_type='bech32')
            if treefn is not None:
                addr_r = self.make_addr(treefn, keys_pay, i)
                assert_equal(addr_g, addr_r)
            boring_balance = int(self.boring.getbalance() * 100000000)
            to_amnt = random.randrange(1000000, boring_balance)
            self.boring.sendtoaddress(address=addr_g, amount=Decimal(to_amnt) / 100000000, subtractfeefromamount=True)
            self.nodes[0].generatetoaddress(1, self.boring.getnewaddress())
            test_balance = int(self.rpc_online.getbalance() * 100000000)
            ret_amnt = random.randrange(100000, test_balance)
            res = self.rpc_online.sendtoaddress(address=self.boring.getnewaddress(), amount=Decimal(ret_amnt) / 100000000, subtractfeefromamount=True)
            self.nodes[0].generatetoaddress(1, self.boring.getnewaddress())
            assert(self.rpc_online.gettransaction(res)["confirmations"] > 0)

    def do_test_psbt(self, comment, pattern, privmap, treefn, keys_pay, keys_change):
        self.log.info("Testing %s through PSBT" % comment)
        desc_pay = self.make_desc(pattern, privmap, keys_pay, False)
        desc_change = self.make_desc(pattern, privmap, keys_change, False)
        desc_pay_pub = self.make_desc(pattern, privmap, keys_pay, True)
        desc_change_pub = self.make_desc(pattern, privmap, keys_change, True)
        assert_equal(self.nodes[0].getdescriptorinfo(desc_pay)['descriptor'], desc_pay_pub)
        assert_equal(self.nodes[0].getdescriptorinfo(desc_change)['descriptor'], desc_change_pub)
        result = self.psbt_online.importdescriptors([{"desc": desc_pay_pub, "active": True, "timestamp": "now"}])
        assert(result[0]['success'])
        result = self.psbt_online.importdescriptors([{"desc": desc_change_pub, "active": True, "timestamp": "now", "internal": True}])
        assert(result[0]['success'])
        result = self.psbt_offline.importdescriptors([{"desc": desc_pay, "active": True, "timestamp": "now"}])
        assert(result[0]['success'])
        result = self.psbt_offline.importdescriptors([{"desc": desc_change, "active": True, "timestamp": "now", "internal": True}])
        assert(result[0]['success'])
        for i in range(4):
            addr_g = self.psbt_online.getnewaddress(address_type='bech32')
            if treefn is not None:
                addr_r = self.make_addr(treefn, keys_pay, i)
                assert_equal(addr_g, addr_r)
            boring_balance = int(self.boring.getbalance() * 100000000)
            to_amnt = random.randrange(1000000, boring_balance)
            self.boring.sendtoaddress(address=addr_g, amount=Decimal(to_amnt) / 100000000, subtractfeefromamount=True)
            self.nodes[0].generatetoaddress(1, self.boring.getnewaddress())
            test_balance = int(self.psbt_online.getbalance() * 100000000)
            ret_amnt = random.randrange(100000, test_balance)
            psbt = self.psbt_online.walletcreatefundedpsbt([], [{self.boring.getnewaddress(): Decimal(ret_amnt) / 100000000}], None, {"subtractFeeFromOutputs":[0]})['psbt']
            res = self.psbt_offline.walletprocesspsbt(psbt)
            assert(res['complete'])
            rawtx = self.nodes[0].finalizepsbt(res['psbt'])['hex']
            txid = self.nodes[0].sendrawtransaction(rawtx)
            self.nodes[0].generatetoaddress(1, self.boring.getnewaddress())
            assert(self.psbt_online.gettransaction(txid)['confirmations'] > 0)

    def do_test(self, comment, pattern, privmap, treefn, nkeys):
        keys = self.rand_keys(nkeys * 4)
        self.do_test_addr(comment, pattern, privmap, treefn, keys[0:nkeys])
        self.do_test_sendtoaddress(comment, pattern, privmap, treefn, keys[0:nkeys], keys[nkeys:2*nkeys])
        self.do_test_psbt(comment, pattern, privmap, treefn, keys[2*nkeys:3*nkeys], keys[3*nkeys:4*nkeys])

    def run_test(self):
        self.log.info("Creating wallets...")
        self.nodes[0].createwallet(wallet_name="privs_tr_enabled", descriptors=True, blank=True)
        self.privs_tr_enabled = self.nodes[0].get_wallet_rpc("privs_tr_enabled")
        self.nodes[2].createwallet(wallet_name="privs_tr_disabled", descriptors=True, blank=True)
        self.privs_tr_disabled=self.nodes[2].get_wallet_rpc("privs_tr_disabled")
        self.nodes[0].createwallet(wallet_name="pubs_tr_enabled", descriptors=True, blank=True, disable_private_keys=True)
        self.pubs_tr_enabled = self.nodes[0].get_wallet_rpc("pubs_tr_enabled")
        self.nodes[2].createwallet(wallet_name="pubs_tr_disabled", descriptors=True, blank=True, disable_private_keys=True)
        self.pubs_tr_disabled=self.nodes[2].get_wallet_rpc("pubs_tr_disabled")
        self.nodes[0].createwallet(wallet_name="boring")
        self.nodes[0].createwallet(wallet_name="addr_gen", descriptors=True, disable_private_keys=True, blank=True)
        self.nodes[0].createwallet(wallet_name="rpc_online", descriptors=True, blank=True)
        self.nodes[0].createwallet(wallet_name="psbt_online", descriptors=True, disable_private_keys=True, blank=True)
        self.nodes[1].createwallet(wallet_name="psbt_offline", descriptors=True, blank=True)
        self.boring = self.nodes[0].get_wallet_rpc("boring")
        self.addr_gen = self.nodes[0].get_wallet_rpc("addr_gen")
        self.rpc_online = self.nodes[0].get_wallet_rpc("rpc_online")
        self.psbt_online = self.nodes[0].get_wallet_rpc("psbt_online")
        self.psbt_offline = self.nodes[1].get_wallet_rpc("psbt_offline")

        self.log.info("Mining blocks...")
        gen_addr = self.boring.getnewaddress()
        self.nodes[0].generatetoaddress(101, gen_addr)

        self.do_test(
            "tr(XPRV)",
            "tr($1/*)",
            [True],
            lambda k1: (key(k1), []),
            1
        )
        self.do_test(
            "tr(H,XPRV)",
            "tr($H,pk($1/*))",
            [True],
            lambda k1: (key(H_POINT), [pk(k1)]),
            1
        )
        self.do_test(
            "wpkh(XPRV)",
            "wpkh($1/*)",
            [True],
            None,
            1
        )
        self.do_test(
            "tr(XPRV,{H,{H,XPUB}})",
            "tr($1/*,{pk($H),{pk($H),pk($2/*)}})",
            [True, False],
            lambda k1, k2: (key(k1), [pk(H_POINT), [pk(H_POINT), pk(k2)]]),
            2
        )
        self.do_test(
            "wsh(multi(1,XPRV,XPUB))",
            "wsh(multi(1,$1/*,$2/*))",
            [True, False],
            None,
            2
        )
        self.do_test(
            "tr(XPUB,{{H,{H,XPUB}},{H,{H,{H,XPRV}}}})",
            "tr($1/*,{{pk($H),{pk($H),pk($2/*)}},{pk($H),{pk($H),{pk($H),pk($3/*)}}}})",
            [False, False, True],
            lambda k1, k2, k3: (key(k1), [[pk(H_POINT), [pk(H_POINT), pk(k2)]], [pk(H_POINT), [pk(H_POINT), [pk(H_POINT), pk(k3)]]]]),
            3
        )

        self.log.info("Sending everything back...")

        txid = self.rpc_online.sendtoaddress(address=self.boring.getnewaddress(), amount=self.rpc_online.getbalance(), subtractfeefromamount=True)
        self.nodes[0].generatetoaddress(1, self.boring.getnewaddress())
        assert(self.rpc_online.gettransaction(txid)["confirmations"] > 0)

        psbt = self.psbt_online.walletcreatefundedpsbt([], [{self.boring.getnewaddress(): self.psbt_online.getbalance()}], None, {"subtractFeeFromOutputs": [0]})['psbt']
        res = self.psbt_offline.walletprocesspsbt(psbt)
        assert(res['complete'])
        rawtx = self.nodes[0].finalizepsbt(res['psbt'])['hex']
        txid = self.nodes[0].sendrawtransaction(rawtx)
        self.nodes[0].generatetoaddress(1, self.boring.getnewaddress())
        assert(self.psbt_online.gettransaction(txid)['confirmations'] > 0)

if __name__ == '__main__':
    WalletTaprootTest().main()
