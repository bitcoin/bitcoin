#!/usr/bin/env python3
# Copyright (c) 2021-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test generation and spending of P2TR addresses."""

import random
import uuid

from decimal import Decimal
from test_framework.address import output_key_to_p2tr
from test_framework.key import H_POINT, compute_xonly_pubkey
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal
from test_framework.descriptors import descsum_create
from test_framework.extendedkey import ExtendedPrivateKey
from test_framework.script import (
    CScript,
    MAX_PUBKEYS_PER_MULTI_A,
    OP_CHECKSIG,
    OP_CHECKSIGADD,
    OP_NUMEQUAL,
    taproot_construct,
)
from test_framework.segwit_addr import encode_segwit_address

def key(hex_key):
    """Construct an x-only pubkey from its hex representation."""
    return bytes.fromhex(hex_key)

def pk(hex_key):
    """Construct a script expression for taproot_construct for pk(hex_key)."""
    return (None, CScript([bytes.fromhex(hex_key), OP_CHECKSIG]))

def multi_a(k, hex_keys, sort=False):
    """Construct a script expression for taproot_construct for a multi_a script."""
    xkeys = [bytes.fromhex(hex_key) for hex_key in hex_keys]
    if sort:
        xkeys.sort()
    ops = [xkeys[0], OP_CHECKSIG]
    for i in range(1, len(hex_keys)):
        ops += [xkeys[i], OP_CHECKSIGADD]
    ops += [k, OP_NUMEQUAL]
    return (None, CScript(ops))

def compute_taproot_address(pubkey, scripts):
    """Compute the address for a taproot output with given inner key and scripts."""
    return output_key_to_p2tr(taproot_construct(pubkey, scripts).output_pubkey)

def compute_raw_taproot_address(pubkey):
    return encode_segwit_address("bcrt", 1, pubkey)

class WalletTaprootTest(BitcoinTestFramework):
    """Test generation and spending of P2TR address outputs."""

    def set_test_params(self):
        self.num_nodes = 2
        self.setup_clean_chain = True
        self.extra_args = [['-keypool=100'], ['-keypool=100']]

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def setup_network(self):
        self.setup_nodes()

    def init_wallet(self, *, node):
        pass

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
        tree = treefn(*args)
        if isinstance(tree, tuple):
            return compute_taproot_address(*tree)
        if isinstance(tree, bytes):
            return compute_raw_taproot_address(tree)
        assert False

    def do_test_addr(self, comment, pattern, privmap, treefn, keys):
        self.log.info("Testing %s address derivation" % comment)

        # Create wallets
        wallet_uuid = uuid.uuid4().hex
        self.nodes[0].createwallet(wallet_name=f"privs_tr_enabled_{wallet_uuid}", blank=True)
        self.nodes[0].createwallet(wallet_name=f"pubs_tr_enabled_{wallet_uuid}", blank=True, disable_private_keys=True)
        self.nodes[0].createwallet(wallet_name=f"addr_gen_{wallet_uuid}", disable_private_keys=True, blank=True)
        privs_tr_enabled = self.nodes[0].get_wallet_rpc(f"privs_tr_enabled_{wallet_uuid}")
        pubs_tr_enabled = self.nodes[0].get_wallet_rpc(f"pubs_tr_enabled_{wallet_uuid}")
        addr_gen = self.nodes[0].get_wallet_rpc(f"addr_gen_{wallet_uuid}")

        desc = self.make_desc(pattern, privmap, keys, False)
        desc_pub = self.make_desc(pattern, privmap, keys, True)
        assert_equal(self.nodes[0].getdescriptorinfo(desc)['descriptor'], desc_pub)
        result = addr_gen.importdescriptors([{"desc": desc_pub, "active": True, "timestamp": "now"}])
        assert result[0]['success']
        address_type = "bech32m" if "tr" in pattern else "bech32"
        for i in range(4):
            addr_g = addr_gen.getnewaddress(address_type=address_type)
            if treefn is not None:
                addr_r = self.make_addr(treefn, keys, i)
                assert_equal(addr_g, addr_r)
            desc_a = addr_gen.getaddressinfo(addr_g)['desc']
            if desc.startswith("tr("):
                assert desc_a.startswith("tr(")
            rederive = self.nodes[1].deriveaddresses(desc_a)
            assert_equal(len(rederive), 1)
            assert_equal(rederive[0], addr_g)

        # tr descriptors can be imported
        result = privs_tr_enabled.importdescriptors([{"desc": desc, "timestamp": "now"}])
        assert result[0]['success']
        result = pubs_tr_enabled.importdescriptors([{"desc": desc_pub, "timestamp": "now"}])
        assert result[0]["success"]

        # Cleanup
        privs_tr_enabled.unloadwallet()
        pubs_tr_enabled.unloadwallet()
        addr_gen.unloadwallet()

    def do_test_sendtoaddress(self, comment, pattern, privmap, treefn, keys_pay, keys_change):
        self.log.info("Testing %s through sendtoaddress" % comment)

        # Create wallets
        wallet_uuid = uuid.uuid4().hex
        self.nodes[0].createwallet(wallet_name=f"rpc_online_{wallet_uuid}", blank=True)
        rpc_online = self.nodes[0].get_wallet_rpc(f"rpc_online_{wallet_uuid}")

        desc_pay = self.make_desc(pattern, privmap, keys_pay)
        desc_change = self.make_desc(pattern, privmap, keys_change)
        desc_pay_pub = self.make_desc(pattern, privmap, keys_pay, True)
        desc_change_pub = self.make_desc(pattern, privmap, keys_change, True)
        assert_equal(self.nodes[0].getdescriptorinfo(desc_pay)['descriptor'], desc_pay_pub)
        assert_equal(self.nodes[0].getdescriptorinfo(desc_change)['descriptor'], desc_change_pub)
        result = rpc_online.importdescriptors([{"desc": desc_pay, "active": True, "timestamp": "now"}])
        assert result[0]['success']
        result = rpc_online.importdescriptors([{"desc": desc_change, "active": True, "timestamp": "now", "internal": True}])
        assert result[0]['success']
        address_type = "bech32m" if "tr" in pattern else "bech32"
        for i in range(4):
            addr_g = rpc_online.getnewaddress(address_type=address_type)
            if treefn is not None:
                addr_r = self.make_addr(treefn, keys_pay, i)
                assert_equal(addr_g, addr_r)
            boring_balance = int(self.boring.getbalance() * 100000000)
            to_amnt = random.randrange(1000000, boring_balance)
            self.boring.sendtoaddress(address=addr_g, amount=Decimal(to_amnt) / 100000000, subtractfeefromamount=True)
            self.generatetoaddress(self.nodes[0], 1, self.boring.getnewaddress(), sync_fun=self.no_op)
            test_balance = int(rpc_online.getbalance() * 100000000)
            ret_amnt = random.randrange(100000, test_balance)
            # Increase fee_rate to compensate for the wallet's inability to estimate fees for script path spends.
            res = rpc_online.sendtoaddress(address=self.boring.getnewaddress(), amount=Decimal(ret_amnt) / 100000000, subtractfeefromamount=True, fee_rate=200)
            self.generatetoaddress(self.nodes[0], 1, self.boring.getnewaddress(), sync_fun=self.no_op)
            assert rpc_online.gettransaction(res)["confirmations"] > 0

        # Cleanup
        txid = rpc_online.sendall(recipients=[self.boring.getnewaddress()])["txid"]
        self.generatetoaddress(self.nodes[0], 1, self.boring.getnewaddress(), sync_fun=self.no_op)
        assert rpc_online.gettransaction(txid)["confirmations"] > 0
        rpc_online.unloadwallet()

    def do_test_psbt(self, comment, pattern, privmap, treefn, keys_pay, keys_change):
        self.log.info("Testing %s through PSBT" % comment)

        # Create wallets
        wallet_uuid = uuid.uuid4().hex
        self.nodes[0].createwallet(wallet_name=f"psbt_online_{wallet_uuid}", disable_private_keys=True, blank=True)
        self.nodes[1].createwallet(wallet_name=f"psbt_offline_{wallet_uuid}", blank=True)
        self.nodes[1].createwallet(f"key_only_wallet_{wallet_uuid}", blank=True)
        psbt_online = self.nodes[0].get_wallet_rpc(f"psbt_online_{wallet_uuid}")
        psbt_offline = self.nodes[1].get_wallet_rpc(f"psbt_offline_{wallet_uuid}")
        key_only_wallet = self.nodes[1].get_wallet_rpc(f"key_only_wallet_{wallet_uuid}")

        desc_pay = self.make_desc(pattern, privmap, keys_pay, False)
        desc_change = self.make_desc(pattern, privmap, keys_change, False)
        desc_pay_pub = self.make_desc(pattern, privmap, keys_pay, True)
        desc_change_pub = self.make_desc(pattern, privmap, keys_change, True)
        assert_equal(self.nodes[0].getdescriptorinfo(desc_pay)['descriptor'], desc_pay_pub)
        assert_equal(self.nodes[0].getdescriptorinfo(desc_change)['descriptor'], desc_change_pub)
        result = psbt_online.importdescriptors([{"desc": desc_pay_pub, "active": True, "timestamp": "now"}])
        assert result[0]['success']
        result = psbt_online.importdescriptors([{"desc": desc_change_pub, "active": True, "timestamp": "now", "internal": True}])
        assert result[0]['success']
        result = psbt_offline.importdescriptors([{"desc": desc_pay, "active": True, "timestamp": "now"}])
        assert result[0]['success']
        result = psbt_offline.importdescriptors([{"desc": desc_change, "active": True, "timestamp": "now", "internal": True}])
        assert result[0]['success']
        for key in keys_pay + keys_change:
            result = key_only_wallet.importdescriptors([{"desc": descsum_create(f"wpkh({key['xprv']}/*)"), "timestamp":"now"}])
            assert result[0]["success"]
        address_type = "bech32m" if "tr" in pattern else "bech32"
        for i in range(4):
            addr_g = psbt_online.getnewaddress(address_type=address_type)
            if treefn is not None:
                addr_r = self.make_addr(treefn, keys_pay, i)
                assert_equal(addr_g, addr_r)
            boring_balance = int(self.boring.getbalance() * 100000000)
            to_amnt = random.randrange(1000000, boring_balance)
            self.boring.sendtoaddress(address=addr_g, amount=Decimal(to_amnt) / 100000000, subtractfeefromamount=True)
            self.generatetoaddress(self.nodes[0], 1, self.boring.getnewaddress(), sync_fun=self.no_op)
            test_balance = int(psbt_online.getbalance() * 100000000)
            ret_amnt = random.randrange(100000, test_balance)
            # Increase fee_rate to compensate for the wallet's inability to estimate fees for script path spends.
            psbt = psbt_online.walletcreatefundedpsbt([], [{self.boring.getnewaddress(): Decimal(ret_amnt) / 100000000}], None, {"subtractFeeFromOutputs":[0], "fee_rate": 200, "change_type": address_type})['psbt']
            res = psbt_offline.walletprocesspsbt(psbt=psbt, finalize=False)
            for wallet in [psbt_offline, key_only_wallet]:
                res = wallet.walletprocesspsbt(psbt=psbt, finalize=False)

                decoded = wallet.decodepsbt(res["psbt"])
                if pattern.startswith("tr("):
                    for psbtin in decoded["inputs"]:
                        assert "non_witness_utxo" not in psbtin
                        assert "witness_utxo" in psbtin
                        assert "taproot_internal_key" in psbtin
                        assert "taproot_bip32_derivs" in psbtin
                        assert "taproot_key_path_sig" in psbtin or "taproot_script_path_sigs" in psbtin
                        if "taproot_script_path_sigs" in psbtin:
                            assert "taproot_merkle_root" in psbtin
                            assert "taproot_scripts" in psbtin

                rawtx = self.nodes[0].finalizepsbt(res['psbt'])['hex']
                res = self.nodes[0].testmempoolaccept([rawtx])
                assert res[0]["allowed"]

            txid = self.nodes[0].sendrawtransaction(rawtx)
            self.generatetoaddress(self.nodes[0], 1, self.boring.getnewaddress(), sync_fun=self.no_op)
            assert psbt_online.gettransaction(txid)['confirmations'] > 0

        # Cleanup
        psbt = psbt_online.sendall(recipients=[self.boring.getnewaddress()], psbt=True)["psbt"]
        res = psbt_offline.walletprocesspsbt(psbt=psbt, finalize=False)
        rawtx = self.nodes[0].finalizepsbt(res['psbt'])['hex']
        txid = self.nodes[0].sendrawtransaction(rawtx)
        self.generatetoaddress(self.nodes[0], 1, self.boring.getnewaddress(), sync_fun=self.no_op)
        assert psbt_online.gettransaction(txid)['confirmations'] > 0
        psbt_online.unloadwallet()
        psbt_offline.unloadwallet()

    def do_test(self, comment, pattern, privmap, treefn):
        nkeys = len(privmap)
        keys = random.sample(self.keys, nkeys * 4)
        self.do_test_addr(comment, pattern, privmap, treefn, keys[0:nkeys])
        self.do_test_sendtoaddress(comment, pattern, privmap, treefn, keys[0:nkeys], keys[nkeys:2*nkeys])
        self.do_test_psbt(comment, pattern, privmap, treefn, keys[2*nkeys:3*nkeys], keys[3*nkeys:4*nkeys])

    def generate_test_keys(self):
        xprvs = [ExtendedPrivateKey.generate() for _ in range(0, 13)]
        return [{
            "xprv": xprv.to_string(),
            "xpub": xprv.pubkey().to_string(),
            "pubs": [compute_xonly_pubkey(xprv.derive_path(f"m/{i}").key.get_bytes())[0].hex() for i in range(0, 4)]
        } for xprv in xprvs]

    def run_test(self):
        self.keys = self.generate_test_keys()
        self.nodes[0].createwallet(wallet_name="boring")
        self.boring = self.nodes[0].get_wallet_rpc("boring")

        self.log.info("Mining blocks...")
        gen_addr = self.boring.getnewaddress()
        self.generatetoaddress(self.nodes[0], 101, gen_addr, sync_fun=self.no_op)

        self.do_test(
            "tr(XPRV)",
            "tr($1/*)",
            [True],
            lambda k1: (key(k1), [])
        )
        self.do_test(
            "tr(H,XPRV)",
            "tr($H,pk($1/*))",
            [True],
            lambda k1: (key(H_POINT), [pk(k1)])
        )
        self.do_test(
            "wpkh(XPRV)",
            "wpkh($1/*)",
            [True],
            None
        )
        self.do_test(
            "tr(XPRV,{H,{H,XPUB}})",
            "tr($1/*,{pk($H),{pk($H),pk($2/*)}})",
            [True, False],
            lambda k1, k2: (key(k1), [pk(H_POINT), [pk(H_POINT), pk(k2)]])
        )
        self.do_test(
            "wsh(multi(1,XPRV,XPUB))",
            "wsh(multi(1,$1/*,$2/*))",
            [True, False],
            None
        )
        self.do_test(
            "tr(XPRV,{XPUB,XPUB})",
            "tr($1/*,{pk($2/*),pk($2/*)})",
            [True, False],
            lambda k1, k2: (key(k1), [pk(k2), pk(k2)])
        )
        self.do_test(
            "tr(XPRV,{{XPUB,H},{H,XPUB}})",
            "tr($1/*,{{pk($2/*),pk($H)},{pk($H),pk($2/*)}})",
            [True, False],
            lambda k1, k2: (key(k1), [[pk(k2), pk(H_POINT)], [pk(H_POINT), pk(k2)]])
        )
        self.do_test(
            "tr(XPUB,{{H,{H,XPUB}},{H,{H,{H,XPRV}}}})",
            "tr($1/*,{{pk($H),{pk($H),pk($2/*)}},{pk($H),{pk($H),{pk($H),pk($3/*)}}}})",
            [False, False, True],
            lambda k1, k2, k3: (key(k1), [[pk(H_POINT), [pk(H_POINT), pk(k2)]], [pk(H_POINT), [pk(H_POINT), [pk(H_POINT), pk(k3)]]]])
        )
        self.do_test(
            "tr(XPRV,{XPUB,{{XPUB,{H,H}},{{H,H},XPUB}}})",
            "tr($1/*,{pk($2/*),{{pk($2/*),{pk($H),pk($H)}},{{pk($H),pk($H)},pk($2/*)}}})",
            [True, False],
            lambda k1, k2: (key(k1), [pk(k2), [[pk(k2), [pk(H_POINT), pk(H_POINT)]], [[pk(H_POINT), pk(H_POINT)], pk(k2)]]])
        )
        self.do_test(
            "tr(H,multi_a(1,XPRV))",
            "tr($H,multi_a(1,$1/*))",
            [True],
            lambda k1: (key(H_POINT), [multi_a(1, [k1])])
        )
        self.do_test(
            "tr(H,sortedmulti_a(1,XPRV,XPUB))",
            "tr($H,sortedmulti_a(1,$1/*,$2/*))",
            [True, False],
            lambda k1, k2: (key(H_POINT), [multi_a(1, [k1, k2], True)])
        )
        self.do_test(
            "tr(H,{H,multi_a(1,XPUB,XPRV)})",
            "tr($H,{pk($H),multi_a(1,$1/*,$2/*)})",
            [False, True],
            lambda k1, k2: (key(H_POINT), [pk(H_POINT), [multi_a(1, [k1, k2])]])
        )
        self.do_test(
            "tr(H,sortedmulti_a(1,XPUB,XPRV,XPRV))",
            "tr($H,sortedmulti_a(1,$1/*,$2/*,$3/*))",
            [False, True, True],
            lambda k1, k2, k3: (key(H_POINT), [multi_a(1, [k1, k2, k3], True)])
        )
        self.do_test(
            "tr(H,multi_a(2,XPRV,XPUB,XPRV))",
            "tr($H,multi_a(2,$1/*,$2/*,$3/*))",
            [True, False, True],
            lambda k1, k2, k3: (key(H_POINT), [multi_a(2, [k1, k2, k3])])
        )
        self.do_test(
            "tr(XPUB,{{XPUB,{XPUB,sortedmulti_a(2,XPRV,XPUB,XPRV)}})",
            "tr($2/*,{pk($2/*),{pk($2/*),sortedmulti_a(2,$1/*,$2/*,$3/*)}})",
            [True, False, True],
            lambda k1, k2, k3: (key(k2), [pk(k2), [pk(k2), multi_a(2, [k1, k2, k3], True)]])
        )
        rnd_pos = random.randrange(MAX_PUBKEYS_PER_MULTI_A)
        self.do_test(
            "tr(XPUB,multi_a(1,H...,XPRV,H...))",
            "tr($2/*,multi_a(1" + (",$H" * rnd_pos) + ",$1/*" + (",$H" * (MAX_PUBKEYS_PER_MULTI_A - 1 - rnd_pos)) + "))",
            [True, False],
            lambda k1, k2: (key(k2), [multi_a(1, ([H_POINT] * rnd_pos) + [k1] + ([H_POINT] * (MAX_PUBKEYS_PER_MULTI_A - 1 - rnd_pos)))])
        )
        self.do_test(
            "rawtr(XPRV)",
            "rawtr($1/*)",
            [True],
            lambda k1: key(k1)
        )

if __name__ == '__main__':
    WalletTaprootTest(__file__).main()
